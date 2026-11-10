#pragma once

#include "p3d/Chunk.hh"
#include "p3d/Schema.hh"

#include <memory>

namespace tk
{

std::unique_ptr<Chunk> CreateChunk(const Schema& schema, uint32_t id);

}
