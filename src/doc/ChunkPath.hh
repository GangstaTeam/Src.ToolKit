#pragma once

#include "p3d/Chunk.hh"

#include <vector>

namespace tk
{

using ChunkPath = std::vector<size_t>;

ChunkPath PathOf(const Chunk* chunk);
Chunk* Resolve(Chunk* root, const ChunkPath& path);

}
