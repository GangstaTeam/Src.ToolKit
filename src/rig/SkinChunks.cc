#include "rig/SkinChunks.hh"

#include "core/ByteReader.hh"
#include "core/ByteWriter.hh"
#include "p3d/ChunkIds.hh"
#include "rig/ChunkText.hh"
#include "rig/NormalTable.hh"

#include <algorithm>
#include <map>
#include <memory>
#include <set>

namespace tk
{

namespace
{

struct Palette
{
    std::vector<int> joints;
    std::map<int, uint8_t> slot;
};

Palette BuildPalette(const SkinBinding& binding)
{
    std::set<int> used;

    for (size_t v = 0; v < binding.joints.size(); ++v)
    {
        for (size_t k = 0; k < 4; ++k)
        {
            if (binding.weights[v][k] > 1e-6f)
            {
                used.insert(binding.joints[v][k]);
            }
        }
    }

    Palette palette;

    for (int joint : used)
    {
        palette.slot[joint] = static_cast<uint8_t>(palette.joints.size());
        palette.joints.push_back(joint);
    }

    return palette;
}

void WriteVec3s(ByteWriter& w, const std::vector<Vec3>& values)
{
    w.U32(static_cast<uint32_t>(values.size()));

    for (Vec3 v : values)
    {
        w.F32(v.x);
        w.F32(v.y);
        w.F32(v.z);
    }
}

uint8_t PackNormal(Vec3 n)
{
    uint8_t best = 0;
    float bestDot = -2.0f;

    for (int i = 0; i < 256; ++i)
    {
        float d = n.x * kPackedNormals[i][0] + n.y * kPackedNormals[i][1] + n.z * kPackedNormals[i][2];

        if (d > bestDot)
        {
            bestDot = d;
            best = static_cast<uint8_t>(i);
        }
    }

    return best;
}

void Bounds(const std::vector<Vec3>& points, Vec3& lo, Vec3& hi, Vec3& centre, float& radius)
{
    lo = {1e30f, 1e30f, 1e30f};
    hi = {-1e30f, -1e30f, -1e30f};

    for (Vec3 p : points)
    {
        lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
        hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
    }

    centre = (lo + hi) * 0.5f;
    radius = 0.0f;

    for (Vec3 p : points)
    {
        radius = std::max(radius, Length(p - centre));
    }
}

Bytes BoundsPayload(uint32_t id, const std::vector<Vec3>& points)
{
    Vec3 lo, hi, centre;
    float radius;
    Bounds(points, lo, hi, centre, radius);
    ByteWriter w;

    if (id == chunk::Box)
    {
        for (float f : {lo.x, lo.y, lo.z, hi.x, hi.y, hi.z})
        {
            w.F32(f);
        }
    }
    else
    {
        for (float f : {centre.x, centre.y, centre.z, radius})
        {
            w.F32(f);
        }
    }

    return w.Take();
}

Bytes ListPayload(uint32_t id, const RiggedCharacter& rig, const Palette& palette)
{
    ByteWriter w;
    uint32_t count = static_cast<uint32_t>(rig.positions.size());

    switch (id)
    {
    case chunk::Positions:
        WriteVec3s(w, rig.positions);
        break;
    case chunk::Normals:
        WriteVec3s(w, rig.normals);
        break;
    case chunk::PackedNormals:
        w.U32(count);

        for (Vec3 n : rig.normals)
        {
            w.U8(PackNormal(n));
        }
        break;
    case chunk::Uvs:
        w.U32(count);
        w.U32(0);

        for (size_t i = 0; i < count; ++i)
        {
            w.F32(rig.uvs[i * 2]);
            w.F32(1.0f - rig.uvs[i * 2 + 1]);
        }
        break;
    case chunk::Colours:
        w.U32(count);

        for (size_t i = 0; i < count; ++i)
        {
            w.U32(0xFFFFFFFFu);
        }
        break;
    case chunk::MatrixList:
        w.U32(count);

        for (size_t v = 0; v < count; ++v)
        {
            uint8_t slots[4] = {0, 0, 0, 0};

            for (size_t k = 0; k < 4; ++k)
            {
                if (rig.binding.weights[v][k] > 1e-6f)
                {
                    slots[3 - k] = palette.slot.at(rig.binding.joints[v][k]);
                }
            }

            w.Append(slots, 4);
        }
        break;
    case chunk::WeightList:
        w.U32(count);

        for (const Weights& weights : rig.binding.weights)
        {
            for (size_t k = 0; k < 3; ++k)
            {
                w.F32(weights[k]);
            }
        }
        break;
    case chunk::MatrixPalette:
        w.U32(static_cast<uint32_t>(palette.joints.size()));

        for (int joint : palette.joints)
        {
            w.U32(static_cast<uint32_t>(joint));
        }
        break;
    case chunk::Indices:
        w.U32(static_cast<uint32_t>(rig.indices.size()));

        for (uint32_t index : rig.indices)
        {
            w.U32(index);
        }
        break;
    default:
        return {};
    }

    return w.Take();
}

Bytes PrimGroupHeader(const Chunk& original, const RiggedCharacter& rig, const Palette& palette)
{
    const Bytes& payload = original.Payload();
    size_t nameEnd = 5 + payload[4];
    ByteReader reader(payload.data() + nameEnd, payload.size() - nameEnd);
    uint32_t fields[9];

    for (uint32_t& field : fields)
    {
        field = reader.U32();
    }

    fields[0] = 0;
    fields[2] = static_cast<uint32_t>(rig.positions.size());
    fields[3] = static_cast<uint32_t>(rig.indices.size());
    fields[4] = static_cast<uint32_t>(palette.joints.size());
    ByteWriter w;
    w.U32(0);
    w.Append(payload.data() + 4, nameEnd - 4);

    for (uint32_t field : fields)
    {
        w.U32(field);
    }

    w.Append(payload.data() + nameEnd + 36, payload.size() - nameEnd - 36);
    return w.Take();
}

std::unique_ptr<Chunk> BuildPrimGroup(const Chunk& original, const RiggedCharacter& rig)
{
    Palette palette = BuildPalette(rig.binding);
    auto group = std::make_unique<Chunk>(chunk::PrimGroup, PrimGroupHeader(original, rig, palette));

    for (size_t i = 0; i < original.ChildCount(); ++i)
    {
        const Chunk& child = *original.Child(i);

        if (child.Id() == chunk::Box || child.Id() == chunk::Sphere)
        {
            group->AppendChild(std::make_unique<Chunk>(child.Id(), BoundsPayload(child.Id(), rig.positions)));
            continue;
        }

        Bytes list = ListPayload(child.Id(), rig, palette);
        group->AppendChild(list.empty() ? child.Clone() : std::make_unique<Chunk>(child.Id(), std::move(list)));
    }

    return group;
}

Bytes NameBytes(const Chunk& chunk)
{
    const Bytes& payload = chunk.Payload();
    return Bytes(payload.begin(), payload.begin() + 1 + payload[0]);
}

}

void RebuildPolySkin(Chunk& skin, const RiggedCharacter& rig)
{
    std::vector<std::unique_ptr<Chunk>> children;

    while (skin.ChildCount() > 0)
    {
        children.push_back(skin.RemoveChild(0));
    }

    bool groupWritten = false;

    for (auto& child : children)
    {
        if (child->Id() == chunk::PrimGroup)
        {
            if (!groupWritten)
            {
                skin.AppendChild(BuildPrimGroup(*child, rig));
            }

            groupWritten = true;
        }
        else if (child->Id() == chunk::Box || child->Id() == chunk::Sphere)
        {
            skin.AppendChild(std::make_unique<Chunk>(child->Id(), BoundsPayload(child->Id(), rig.positions)));
        }
        else
        {
            skin.AppendChild(std::move(child));
        }
    }

    Bytes payload = skin.Payload();
    size_t countOffset = 1 + payload[0] + 4;
    countOffset += 1 + payload[countOffset];
    ByteWriter count;
    count.U32(1);
    std::copy(count.Data().begin(), count.Data().end(), payload.begin() + static_cast<std::ptrdiff_t>(countOffset));
    skin.SetPayload(std::move(payload));
}

void RewriteSkeleton(Chunk& skeleton, const GameSkeleton& fitted)
{
    for (size_t i = 0; i < skeleton.ChildCount(); ++i)
    {
        Chunk& joint = *skeleton.Child(i);

        if (joint.Id() != chunk::SkeletonJoint)
        {
            continue;
        }

        ByteReader reader(joint.Payload());
        int index = FindJoint(fitted, ReadName(reader));

        if (index < 0)
        {
            continue;
        }

        Bytes payload = joint.Payload();
        size_t matrixOffset = 1 + payload[0] + 4;
        ByteWriter matrix;

        for (const auto& row : fitted[static_cast<size_t>(index)].local.m)
        {
            for (float value : row)
            {
                matrix.F32(value);
            }
        }

        std::copy(matrix.Data().begin(),
            matrix.Data().end(),
            payload.begin() + static_cast<std::ptrdiff_t>(matrixOffset));
        joint.SetPayload(std::move(payload));
    }
}

void RewriteTexture(Chunk& texture, const Bytes& png, uint32_t size)
{
    Bytes name = NameBytes(texture);
    ByteWriter header;
    header.Append(name);

    for (uint32_t value : {static_cast<uint32_t>(kTextureVersion), size, size, 24u, 0u, 1u, 0u, 0u, 0u})
    {
        header.U32(value);
    }

    ByteWriter imageHeader;
    imageHeader.Append(name);

    for (uint32_t value : {static_cast<uint32_t>(kTextureVersion), size, size, 24u, 0u, 0u, 1u})
    {
        imageHeader.U32(value);
    }

    ByteWriter data;
    data.U32(static_cast<uint32_t>(png.size()));
    data.Append(png);
    auto image = std::make_unique<Chunk>(chunk::Image, imageHeader.Take());
    image->AppendChild(std::make_unique<Chunk>(chunk::ImageData, data.Take()));

    while (texture.ChildCount() > 0)
    {
        texture.RemoveChild(0);
    }

    texture.SetPayload(header.Take());
    texture.AppendChild(std::move(image));
}

}
