#pragma once

#include "rig/SourceModel.hh"

namespace tk
{

struct Orientation
{
    int quarterTurns[3] = {0, 0, 0};
};

struct RigModel
{
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<float> uvs;
    std::vector<uint32_t> indices;
    std::vector<SourceBone> bones;
    std::vector<Influences> influences;
    std::vector<Weights> weights;
    float height = 1.8f;
};

RigModel PrepareModel(const SourceModel& source, const Orientation& orientation, float height);

}
