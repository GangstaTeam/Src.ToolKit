#pragma once

#include "rig/RigModel.hh"

#include <cstdint>
#include <vector>

namespace tk
{

std::vector<uint32_t> WeldVertices(const std::vector<Vec3>& positions, uint32_t& uniqueCount);
void SmoothOverMesh(const RigModel& model,
    std::vector<float>& values,
    const std::vector<uint8_t>& mask,
    size_t stride,
    int iterations);

}
