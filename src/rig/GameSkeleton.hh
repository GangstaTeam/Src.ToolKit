#pragma once

#include "p3d/Chunk.hh"
#include "rig/RigMath.hh"

#include <map>
#include <string>
#include <vector>

namespace tk
{

struct GameJoint
{
    std::string name;
    int parent = 0;
    Mat4 local;
};

using GameSkeleton = std::vector<GameJoint>;
using JointTargets = std::map<std::string, Vec3>;

GameSkeleton DefaultGameSkeleton();
GameSkeleton ReadGameSkeleton(const Chunk& skeleton);
std::vector<Mat4> WorldMatrices(const GameSkeleton& skeleton);
int FindJoint(const GameSkeleton& skeleton, const std::string& name);
GameSkeleton FitSkeleton(const GameSkeleton& base, const JointTargets& targets, float height);

}
