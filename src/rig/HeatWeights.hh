#pragma once

#include <cstddef>
#include <cstdint>

#include "rig/BoneSegments.hh"
#include "rig/GameSkeleton.hh"
#include "rig/RigModel.hh"
#include "rig/SkinWeights.hh"

#include <optional>

namespace tk
{

enum EHeatWeights : int
{
    kMaxIterations = 3000,
    kGroupCount = kLegR + 1,
};

enum EHeatWeightsSize : size_t
{
    kVisibilityCandidates = 8,
};

std::optional<SkinBinding>
ComputeHeatWeights(const RigModel& model, const JointTargets& targets, const GameSkeleton& skeleton);

}
