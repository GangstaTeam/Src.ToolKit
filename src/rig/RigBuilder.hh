#pragma once

#include "rig/GameSkeleton.hh"
#include "rig/Markers.hh"
#include "rig/RigModel.hh"
#include "rig/SkinWeights.hh"

#include <optional>
#include <string>
#include <vector>

namespace tk
{

enum class RigSource
{
    Markers,
    ModelSkeleton
};

struct RigRequest
{
    RigSource source = RigSource::Markers;
    MarkerSet markers;
    MarkerFlags placed = {};
    std::vector<std::string> mapping;
    bool straightenArms = true;
};

struct RiggedCharacter
{
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<float> uvs;
    std::vector<uint32_t> indices;
    GameSkeleton skeleton;
    JointTargets targets;
    SkinBinding binding;
    float height = 1.8f;
    bool armTilted[2] = {false, false};
    Mat4 armTilt[2];
};

std::optional<RiggedCharacter>
BuildRig(const RigModel& model, const RigRequest& request, const GameSkeleton& base, std::string& error);
RiggedCharacter Rebind(const RiggedCharacter& rig, const GameSkeleton& base);
GameSkeleton StraightSkeleton(const RiggedCharacter& rig);

}
