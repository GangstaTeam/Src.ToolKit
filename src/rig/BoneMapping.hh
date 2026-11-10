#pragma once

#include "rig/SourceModel.hh"

#include <string>
#include <vector>

namespace tk
{

const std::vector<std::string>& MappableJoints();
std::vector<std::string> GuessBoneMapping(const std::vector<SourceBone>& bones);
std::vector<std::string> ResolveMapping(const std::vector<SourceBone>& bones, const std::vector<std::string>& mapping);

}
