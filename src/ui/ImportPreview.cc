#include "ui/ImportPreview.hh"

#include "rig/WeightPaint.hh"

#include <d3d9.h>

#include <algorithm>

namespace tk
{

MeshScene BuildPreviewScene(const std::vector<Vec3>& positions,
    const std::vector<Vec3>& normals,
    const std::vector<float>& uvs,
    const std::vector<uint32_t>& indices,
    const std::vector<uint32_t>& colours)
{
    MeshScene scene;
    MeshPart part;
    part.shader = IMPORTPREVIEW_IMPORT_SHADER;
    part.indices = indices;
    part.vertices.resize(positions.size());

    for (size_t i = 0; i < positions.size(); ++i)
    {
        MeshVertex& v = part.vertices[i];
        Vec3 n = i < normals.size() ? normals[i] : Vec3{0, 1, 0};
        v = {positions[i].x,
            positions[i].y,
            positions[i].z,
            n.x,
            n.y,
            n.z,
            i < colours.size() ? colours[i] : 0xFFFFFFFFu,
            uvs.size() > i * 2 + 1 ? uvs[i * 2] : 0.0f,
            uvs.size() > i * 2 + 1 ? uvs[i * 2 + 1] : 0.0f};
        scene.bounds.Add(v.x, v.y, v.z);
    }

    scene.triangles = indices.size() / 3;
    scene.parts.push_back(std::move(part));
    return scene;
}

void UpdatePreviewVertices(std::vector<MeshVertex>& vertices,
    const std::vector<Vec3>& positions,
    const std::vector<Vec3>& normals)
{
    for (size_t i = 0; i < vertices.size() && i < positions.size(); ++i)
    {
        vertices[i].x = positions[i].x;
        vertices[i].y = positions[i].y;
        vertices[i].z = positions[i].z;
        vertices[i].nx = normals[i].x;
        vertices[i].ny = normals[i].y;
        vertices[i].nz = normals[i].z;
    }
}

std::vector<uint32_t> BoneColours(const SkinBinding& binding)
{
    std::vector<uint32_t> colours;

    for (const Influences& joints : binding.joints)
    {
        uint32_t i = static_cast<uint32_t>(joints[0]);
        colours.push_back(D3DCOLOR_XRGB((i * 97) % 255, (i * 57 + 80) % 255, (i * 151 + 40) % 255));
    }

    return colours;
}

std::vector<uint32_t> WeightColours(const SkinBinding& binding, int joint)
{
    std::vector<uint32_t> colours;

    for (size_t v = 0; v < binding.joints.size(); ++v)
    {
        float w = JointWeight(binding, v, joint);
        float r = std::min(1.0f, w * 2.0f), g = std::max(0.0f, w * 2.0f - 1.0f), b = 0.35f * (1.0f - w);
        colours.push_back(D3DCOLOR_XRGB(static_cast<int>(40 + 215 * r),
            static_cast<int>(30 + 200 * g),
            static_cast<int>(60 + 150 * b)));
    }

    return colours;
}

std::vector<JointPose> RigJoints(const GameSkeleton& skeleton)
{
    std::vector<Mat4> worlds = WorldMatrices(skeleton);
    std::vector<JointPose> joints;

    for (size_t i = 0; i < skeleton.size(); ++i)
    {
        JointPose joint;
        joint.name = skeleton[i].name;
        joint.parent = i == 0 || static_cast<size_t>(skeleton[i].parent) >= i ? -1 : skeleton[i].parent;
        joint.position = worlds[i].Translation();

        for (int a = 0; a < 3; ++a)
        {
            joint.axes[a] = Normalize({worlds[i].m[a][0], worlds[i].m[a][1], worlds[i].m[a][2]});
        }

        joints.push_back(joint);
    }

    return joints;
}

std::vector<JointPose> SourceJoints(const RigModel& model)
{
    std::vector<JointPose> joints;

    for (const SourceBone& bone : model.bones)
    {
        JointPose joint;
        joint.name = bone.name;
        joint.parent = bone.parent;
        joint.position = bone.position;
        joints.push_back(joint);
    }

    return joints;
}

}
