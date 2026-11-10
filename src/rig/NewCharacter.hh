#pragma once

#include "p3d/Chunk.hh"
#include "rig/LoadGroup.hh"

#include <string>

namespace tk
{

bool IsOriginalCharacter(const GameFolders& folders, const std::string& name);
bool SaveNewCharacter(const Chunk& package, const std::string& name, const GameFolders& folders, std::string& report);

}
