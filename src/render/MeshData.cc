#include "render/MeshData.hh"

#include <algorithm>
#include <cmath>

namespace tk
{

void Bounds::Add(float x, float y, float z)
{
    const float point[3] = {x, y, z};

    for (int i = 0; i < 3; ++i)
    {
        min[i] = std::min(min[i], point[i]);
        max[i] = std::max(max[i], point[i]);
    }
}

float Bounds::Radius() const
{
    if (!Valid())
    {
        return 1.0f;
    }

    float dx = max[0] - min[0], dy = max[1] - min[1], dz = max[2] - min[2];
    return std::max(0.5f * std::sqrt(dx * dx + dy * dy + dz * dz), 0.01f);
}

void Bounds::Centre(float out[3]) const
{
    for (int i = 0; i < 3; ++i)
    {
        out[i] = Valid() ? 0.5f * (min[i] + max[i]) : 0.0f;
    }
}

}
