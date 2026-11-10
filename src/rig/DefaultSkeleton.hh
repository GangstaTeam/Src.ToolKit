#pragma once

#include <vector>

namespace tk
{

struct DefaultJoint
{
    const char* name;
    int parent;
    float local[16];
};

const std::vector<DefaultJoint>& DefaultSkeletonJoints();

}
