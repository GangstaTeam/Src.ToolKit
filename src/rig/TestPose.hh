#pragma once

#include "rig/RigBuilder.hh"

#include <vector>

namespace tk
{

struct PosedCharacter
{
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec3> joints;
};

PosedCharacter PoseCharacter(const RiggedCharacter& rig, float phase);

}
