#pragma once

#include "core/ByteReader.hh"
#include "p3d/Chunk.hh"

#include <string>

namespace tk
{

std::string ReadName(ByteReader& reader);
std::string LeadingName(const Chunk& chunk);

}
