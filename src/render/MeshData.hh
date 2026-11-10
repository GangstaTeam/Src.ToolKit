#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tk
{

struct MeshVertex
{
    float x, y, z;
    float nx, ny, nz;
    uint32_t colour;
    float u, v;
};

struct MeshPart
{
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::string shader;
};

struct Bounds
{
    float min[3] = {1e30f, 1e30f, 1e30f};
    float max[3] = {-1e30f, -1e30f, -1e30f};

    void Add(float x, float y, float z);
    bool Valid() const { return min[0] <= max[0]; }
    float Radius() const;
    void Centre(float out[3]) const;
};

struct MeshScene
{
    std::vector<MeshPart> parts;
    Bounds bounds;
    size_t triangles = 0;
};

}
