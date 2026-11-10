#include "rig/GltfImport.hh"

#include "rig/Gltf.hh"
#include "rig/RigMath.hh"

#include <functional>

namespace tk
{

namespace
{

Mat4 LocalMatrix(const Json& node)
{
    Mat4 out;

    if (node.Has("matrix"))
    {
        for (int i = 0; i < 16; ++i)
        {
            out.m[i / 4][i % 4] = static_cast<float>(node["matrix"][static_cast<size_t>(i)].Number());
        }

        return out;
    }

    auto vec = [&](const char* key, Vec3 fallback)
    {
        const Json& v = node[key];
        return v.IsNull() ? fallback
                          : Vec3{static_cast<float>(v[0].Number()),
                                static_cast<float>(v[1].Number()),
                                static_cast<float>(v[2].Number())};
    };
    float rotation[4] = {0, 0, 0, 1};

    for (size_t i = 0; i < 4 && node.Has("rotation"); ++i)
    {
        rotation[i] = static_cast<float>(node["rotation"][i].Number());
    }

    return FromTrs(vec("translation", {}), rotation, vec("scale", {1, 1, 1}));
}

float Determinant3(const Mat4& m)
{
    return m.m[0][0] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1]) -
        m.m[0][1] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0]) +
        m.m[0][2] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0]);
}

struct SkinData
{
    std::vector<int> bones;
    std::vector<Mat4> skinning;
};

Vec3 VecAt(const Accessor& a, size_t i)
{
    return {a.values[i * a.components], a.values[i * a.components + 1], a.values[i * a.components + 2]};
}

class GltfImporter
{
public:
    explicit GltfImporter(const GltfFile& file) : m_file(file), m_nodes(file.json["nodes"]) {}

    SourceModel Run()
    {
        BuildHierarchy();
        BuildBones();

        for (size_t i = 0; i < m_nodes.Size(); ++i)
        {
            if (!m_nodes[i].Has("mesh"))
            {
                continue;
            }

            const Json& primitives = m_file.json["meshes"][static_cast<size_t>(m_nodes[i]["mesh"].Int())]["primitives"];

            for (size_t p = 0; p < primitives.Size(); ++p)
            {
                AddPrimitive(static_cast<int>(i), primitives[p]);
            }
        }

        ReadTexture();
        return std::move(m_model);
    }

private:
    void BuildHierarchy()
    {
        m_parent.assign(m_nodes.Size(), -1);

        for (size_t i = 0; i < m_nodes.Size(); ++i)
        {
            const Json& children = m_nodes[i]["children"];

            for (size_t c = 0; c < children.Size(); ++c)
            {
                size_t child = static_cast<size_t>(children[c].Int());

                if (child < m_parent.size())
                {
                    m_parent[child] = static_cast<int>(i);
                }
            }
        }

        m_world.resize(m_nodes.Size());
        std::vector<bool> done(m_nodes.Size(), false);
        std::function<const Mat4&(size_t)> world = [&](size_t i) -> const Mat4&
        {
            if (!done[i])
            {
                done[i] = true;
                m_world[i] = LocalMatrix(m_nodes[i]);

                if (m_parent[i] >= 0)
                {
                    m_world[i] = m_world[i] * world(static_cast<size_t>(m_parent[i]));
                }
            }

            return m_world[i];
        };

        for (size_t i = 0; i < m_nodes.Size(); ++i)
        {
            world(i);
        }
    }

    void BuildBones()
    {
        m_boneOfNode.assign(m_nodes.Size(), -1);
        const Json& skins = m_file.json["skins"];
        std::vector<size_t> boneNodes;

        for (size_t s = 0; s < skins.Size(); ++s)
        {
            const Json& joints = skins[s]["joints"];

            for (size_t j = 0; j < joints.Size(); ++j)
            {
                size_t node = static_cast<size_t>(joints[j].Int());

                if (node >= m_boneOfNode.size() || m_boneOfNode[node] >= 0)
                {
                    continue;
                }

                m_boneOfNode[node] = static_cast<int>(boneNodes.size());
                boneNodes.push_back(node);
            }
        }

        for (size_t node : boneNodes)
        {
            SourceBone bone;
            bone.name = m_nodes[node]["name"].String();
            bone.parent = NearestBone(m_parent[node]);
            bone.position = m_world[node].Translation();
            m_model.bones.push_back(bone);
        }

        for (size_t s = 0; s < skins.Size(); ++s)
        {
            m_skins.push_back(ReadSkin(skins[s]));
        }
    }

    SkinData ReadSkin(const Json& skin) const
    {
        const Json& joints = skin["joints"];
        Accessor ibm =
            skin.Has("inverseBindMatrices") ? ReadAccessor(m_file, skin["inverseBindMatrices"].Int()) : Accessor();
        SkinData data;

        for (size_t j = 0; j < joints.Size(); ++j)
        {
            size_t node = static_cast<size_t>(joints[j].Int());
            Mat4 inverseBind;

            for (int k = 0; k < 16 && ibm.values.size() >= (j + 1) * 16; ++k)
            {
                inverseBind.m[k / 4][k % 4] = ibm.values[j * 16 + k];
            }

            data.bones.push_back(node < m_boneOfNode.size() ? m_boneOfNode[node] : 0);
            data.skinning.push_back(inverseBind * m_world[node]);
        }

        return data;
    }

    int NearestBone(int node) const
    {
        while (node >= 0 && m_boneOfNode[static_cast<size_t>(node)] < 0)
        {
            node = m_parent[static_cast<size_t>(node)];
        }

        return node >= 0 ? m_boneOfNode[static_cast<size_t>(node)] : -1;
    }

    void AddPrimitive(int node, const Json& primitive)
    {
        if (primitive["mode"].Int(kTriangles) != kTriangles || !primitive["attributes"].Has("POSITION"))
        {
            return;
        }

        const Json& attributes = primitive["attributes"];
        Accessor positions = ReadAccessor(m_file, attributes["POSITION"].Int());
        Accessor normals = attributes.Has("NORMAL") ? ReadAccessor(m_file, attributes["NORMAL"].Int()) : Accessor();
        Accessor uvs = attributes.Has("TEXCOORD_0") ? ReadAccessor(m_file, attributes["TEXCOORD_0"].Int()) : Accessor();
        Accessor joints = attributes.Has("JOINTS_0") ? ReadAccessor(m_file, attributes["JOINTS_0"].Int()) : Accessor();
        Accessor weights =
            attributes.Has("WEIGHTS_0") ? ReadAccessor(m_file, attributes["WEIGHTS_0"].Int()) : Accessor();
        int skin = m_nodes[static_cast<size_t>(node)]["skin"].Int();
        bool skinned = skin >= 0 && static_cast<size_t>(skin) < m_skins.size() && joints.count == positions.count &&
            weights.count == positions.count;
        size_t base = m_model.positions.size();

        for (size_t i = 0; i < positions.count; ++i)
        {
            if (skinned)
            {
                AddSkinnedVertex(m_skins[static_cast<size_t>(skin)],
                    VecAt(positions, i),
                    normals.count ? VecAt(normals, i) : Vec3{0, 1, 0},
                    joints,
                    weights,
                    i);
            }
            else
            {
                AddRigidVertex(node, VecAt(positions, i), normals.count ? VecAt(normals, i) : Vec3{0, 1, 0});
            }

            m_model.uvs.push_back(uvs.count ? uvs.values[i * 2] : 0.0f);
            m_model.uvs.push_back(uvs.count ? uvs.values[i * 2 + 1] : 0.0f);
        }

        AddIndices(primitive,
            positions.count,
            base,
            !skinned && Determinant3(m_world[static_cast<size_t>(node)]) < 0.0f);
        if (!normals.count)
        {
            ComputeNormals(base);
        }

        if (m_material < 0 && primitive.Has("material"))
        {
            m_material = primitive["material"].Int();
        }
    }

    void
    AddSkinnedVertex(const SkinData& skin, Vec3 p, Vec3 n, const Accessor& joints, const Accessor& weights, size_t i)
    {
        Vec3 position, normal;
        Influences influence = {0, 0, 0, 0};
        Weights weight = {0, 0, 0, 0};
        float total = 0.0f;

        for (size_t k = 0; k < 4 && k < joints.components; ++k)
        {
            size_t joint = static_cast<size_t>(joints.values[i * joints.components + k]);
            float w = weights.values[i * weights.components + k];

            if (joint >= skin.skinning.size() || w <= 0.0f)
            {
                continue;
            }

            position = position + TransformPoint(p, skin.skinning[joint]) * w;
            normal = normal + TransformVector(n, skin.skinning[joint]) * w;
            influence[k] = skin.bones[joint];
            weight[k] = w;
            total += w;
        }

        if (total <= 0.0f)
        {
            position = p;
            normal = n;
            weight[0] = total = 1.0f;
        }

        for (float& w : weight)
        {
            w /= total;
        }

        m_model.positions.push_back(position * (1.0f / total));
        m_model.normals.push_back(Normalize(normal));
        m_model.influences.push_back(influence);
        m_model.weights.push_back(weight);
    }

    void AddRigidVertex(int node, Vec3 p, Vec3 n)
    {
        const Mat4& world = m_world[static_cast<size_t>(node)];
        m_model.positions.push_back(TransformPoint(p, world));
        m_model.normals.push_back(Normalize(TransformVector(n, world)));

        if (m_skins.empty())
        {
            return;
        }

        int bone = NearestBone(node);
        m_model.influences.push_back({bone < 0 ? 0 : bone, 0, 0, 0});
        m_model.weights.push_back({1, 0, 0, 0});
    }

    void AddIndices(const Json& primitive, size_t count, size_t base, bool flip)
    {
        std::vector<uint32_t> indices;

        if (primitive.Has("indices"))
        {
            Accessor a = ReadAccessor(m_file, primitive["indices"].Int());

            for (float v : a.values)
            {
                indices.push_back(static_cast<uint32_t>(v));
            }
        }
        else
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                indices.push_back(i);
            }
        }

        for (size_t t = 0; t + 2 < indices.size(); t += 3)
        {
            if (indices[t] >= count || indices[t + 1] >= count || indices[t + 2] >= count)
            {
                continue;
            }

            uint32_t b = static_cast<uint32_t>(base);
            m_model.indices.insert(m_model.indices.end(),
                {indices[t] + b, indices[t + (flip ? 2 : 1)] + b, indices[t + (flip ? 1 : 2)] + b});
        }
    }

    void ComputeNormals(size_t base)
    {
        for (size_t i = base; i < m_model.normals.size(); ++i)
        {
            m_model.normals[i] = {};
        }

        for (size_t t = 0; t + 2 < m_model.indices.size(); t += 3)
        {
            uint32_t a = m_model.indices[t], b = m_model.indices[t + 1], c = m_model.indices[t + 2];

            if (a < base)
            {
                continue;
            }

            Vec3 face = Cross(m_model.positions[b] - m_model.positions[a], m_model.positions[c] - m_model.positions[a]);

            for (uint32_t v : {a, b, c})
            {
                m_model.normals[v] = m_model.normals[v] + face;
            }
        }

        for (size_t i = base; i < m_model.normals.size(); ++i)
        {
            m_model.normals[i] = Normalize(m_model.normals[i]);
        }
    }

    void ReadTexture()
    {
        if (m_material < 0)
        {
            return;
        }

        const Json& texture =
            m_file.json["materials"][static_cast<size_t>(m_material)]["pbrMetallicRoughness"]["baseColorTexture"];
        if (texture.IsNull())
        {
            return;
        }

        int source = m_file.json["textures"][static_cast<size_t>(texture["index"].Int(0))]["source"].Int();

        if (source >= 0)
        {
            m_model.texture = ReadImage(m_file, source);
        }
    }

    const GltfFile& m_file;
    const Json& m_nodes;
    std::vector<int> m_parent;
    std::vector<Mat4> m_world;
    std::vector<int> m_boneOfNode;
    std::vector<SkinData> m_skins;
    SourceModel m_model;
    int m_material = -1;
};

}

std::optional<SourceModel> ImportGltf(const std::filesystem::path& path, std::string& error)
{
    auto file = LoadGltf(path, error);

    if (!file)
    {
        return std::nullopt;
    }

    SourceModel model = GltfImporter(*file).Run();

    if (model.indices.empty())
    {
        error = path.filename().u8string() + ": no triangle meshes found";
        return std::nullopt;
    }

    return model;
}

}
