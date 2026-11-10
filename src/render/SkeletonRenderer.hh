#pragma once

#include "render/OrbitCamera.hh"
#include "render/SkeletonPose.hh"

#include <d3d9.h>

#include <vector>

namespace tk
{

void DrawSkeletonLines(IDirect3DDevice9* device, const std::vector<JointPose>& joints, float axisLength);

}
