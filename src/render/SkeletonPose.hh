#pragma once

#include "p3d/Chunk.hh"
#include "render/Math3d.hh"

#include <string>
#include <vector>

namespace tk
{

struct JointPose
{
    std::string name;
    int parent = -1;
    Chunk* chunk = nullptr;
    Vec3 position;
    Vec3 axes[3];
};

std::vector<JointPose> ExtractSkeleton(Chunk& skeleton);

}
