#pragma once

#include "render/MeshData.hh"
#include "render/SkeletonPose.hh"
#include "rig/RigBuilder.hh"

#include <vector>

#define IMPORTPREVIEW_IMPORT_SHADER "import"

namespace tk
{

MeshScene BuildPreviewScene(const std::vector<Vec3>& positions,
    const std::vector<Vec3>& normals,
    const std::vector<float>& uvs,
    const std::vector<uint32_t>& indices,
    const std::vector<uint32_t>& colours);
void UpdatePreviewVertices(std::vector<MeshVertex>& vertices,
    const std::vector<Vec3>& positions,
    const std::vector<Vec3>& normals);
std::vector<uint32_t> BoneColours(const SkinBinding& binding);
std::vector<uint32_t> WeightColours(const SkinBinding& binding, int joint);
std::vector<JointPose> RigJoints(const GameSkeleton& skeleton);
std::vector<JointPose> SourceJoints(const RigModel& model);

}
