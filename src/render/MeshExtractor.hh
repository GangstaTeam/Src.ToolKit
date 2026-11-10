#pragma once

#include <cstddef>
#include <cstdint>

#include "p3d/Chunk.hh"
#include "render/MeshData.hh"

#include <vector>

namespace tk
{

enum EMeshExtractorValue : uint32_t
{
    kPrimTriangleStrip = 1,
};

MeshScene ExtractMeshes(const std::vector<const Chunk*>& sources);

}
