#pragma once

#include "rig/GameSkeleton.hh"
#include "rig/RigModel.hh"

#include <string>
#include <vector>

namespace tk
{

struct SkinBinding
{
    std::vector<Influences> joints;
    std::vector<Weights> weights;
};

SkinBinding ComputeDistanceWeights(const RigModel& model, const JointTargets& targets, const GameSkeleton& skeleton);
SkinBinding ComputeAutoWeights(const RigModel& model, const JointTargets& targets, const GameSkeleton& skeleton);
SkinBinding
MapSourceWeights(const RigModel& model, const std::vector<std::string>& resolved, const GameSkeleton& skeleton);

}
