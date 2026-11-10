#pragma once

#include "core/Bytes.hh"
#include "render/Math3d.hh"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace tk
{

using Influences = std::array<int, 4>;
using Weights = std::array<float, 4>;

struct SourceBone
{
    std::string name;
    int parent = -1;
    Vec3 position;
};

struct SourceModel
{
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<float> uvs;
    std::vector<uint32_t> indices;
    std::vector<SourceBone> bones;
    std::vector<Influences> influences;
    std::vector<Weights> weights;
    Bytes texture;

    bool HasSkin() const { return !bones.empty(); }
    size_t VertexCount() const { return positions.size(); }
};

}
