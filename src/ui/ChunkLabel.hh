#pragma once

#include "p3d/Chunk.hh"
#include "p3d/Schema.hh"

#include <string>

namespace tk
{

std::string ObjectName(const Schema& schema, const Chunk& chunk);
std::string ChunkLabel(const Schema& schema, const Chunk& chunk);

}
