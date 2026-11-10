#pragma once

#include "rig/RigModel.hh"

#include <optional>

namespace tk
{

struct ProbeBox
{
    float minX = -1e30f, maxX = 1e30f;
    float minY = -1e30f, maxY = 1e30f;
};

std::optional<Vec3> BoxCentre(const RigModel& model, const ProbeBox& box);
float DepthAt(const RigModel& model, float x, float y, float radius);
float CrotchHeight(const RigModel& model);
float OuterExtent(const RigModel& model, float y, float band, bool left);
std::optional<float> ShoulderTop(const RigModel& model, float fromY, float toY, bool left);
std::optional<Vec3> SnapToMesh(const RigModel& model, float x, float y, float radius);

}
