#pragma once

#include "rig/GameSkeleton.hh"
#include "rig/RigModel.hh"

#include <map>
#include <string>
#include <vector>

namespace tk
{

enum Group
{
    kTorso,
    kArmL,
    kArmR,
    kLegL,
    kLegR
};

struct Segment
{
    std::string joint;
    Vec3 a, b;
    int group;
    float reach = 1.0f;
};

struct SegmentTuning
{
    float pelvisUp = 0.2f;
    float pelvisWings = 0.75f;
    std::map<std::string, float> reach = {{"Pelvis", 1.375f},
        {"Spine_1", 1.4f},
        {"Spine_2", 1.72f},
        {"Neck", 1.1f},
        {"Hip", 1.21f}};
};

SegmentTuning& WeightTuning();

std::vector<Segment> BuildSegments(const RigModel& model, JointTargets targets);
Vec3 ClosestOnSegment(Vec3 p, const Segment& segment);
float SegmentDistance(Vec3 p, const Segment& segment);
float ReachDistance(Vec3 p, const Segment& segment);
bool AllowedInfluence(int vertexGroup, const Segment& segment);
void KeepStrongest(const float* values,
    size_t count,
    const std::vector<int>& jointOf,
    Influences& joints,
    Weights& weights);

}
