#pragma once

#include "p3d/Chunk.hh"

#include <string>

namespace tk
{

size_t RenamePackage(Chunk& root, const std::string& from, const std::string& to);
bool IsValidPackageName(const std::string& name);

}
