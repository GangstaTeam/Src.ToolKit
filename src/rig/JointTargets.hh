#pragma once

#include "rig/GameSkeleton.hh"
#include "rig/Markers.hh"
#include "rig/RigModel.hh"

#include <string>
#include <vector>

namespace tk
{

JointTargets TargetsFromMarkers(const RigModel& model, const MarkerSet& markers, const MarkerFlags& placed);
Vec3 EstimateShoulder(const RigModel& model, Vec3 elbow, Vec3 wrist, float chinY, bool left);
JointTargets TargetsFromBones(const RigModel& model, const std::vector<std::string>& mapping, std::string& missing);

}
