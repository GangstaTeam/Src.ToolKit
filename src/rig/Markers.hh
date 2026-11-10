#pragma once

#include <cstddef>
#include <cstdint>

#include "rig/RigModel.hh"

#include <array>

namespace tk
{

enum class Marker
{
    Chin,
    Groin,
    WristL,
    WristR,
    ElbowL,
    ElbowR,
    KneeL,
    KneeR,
    ShoulderL,
    ShoulderR,
    Count
};

enum EMarkersSize : size_t
{
    kMarkerCount = static_cast<size_t>(Marker::Count),
};

struct MarkerPoint
{
    float x = 0.0f;
    float y = 0.0f;
};

using MarkerSet = std::array<MarkerPoint, kMarkerCount>;
using MarkerFlags = std::array<bool, kMarkerCount>;

const char* MarkerLabel(Marker marker);
Marker MirrorOf(Marker marker);
bool IsOptional(Marker marker);
MarkerSet GuessMarkers(const RigModel& model);

}
