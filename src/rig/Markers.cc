#include "rig/Markers.hh"

#include "rig/JointTargets.hh"
#include "rig/MeshProbe.hh"
#include "rig/RigMath.hh"

namespace tk
{

namespace
{

MarkerPoint& At(MarkerSet& set, Marker marker)
{
    return set[static_cast<size_t>(marker)];
}

Vec3 OuterTip(const RigModel& model, float side)
{
    Vec3 best{side * 0.3f * model.height, 0.8f * model.height, 0.0f};

    for (Vec3 p : model.positions)
    {
        if (p.x * side > best.x * side && p.y > 0.3f * model.height)
        {
            best = p;
        }
    }

    return best;
}

float LegCentreX(const RigModel& model, float y, float side)
{
    float h = model.height;
    ProbeBox box{side > 0 ? 0.01f * h : -h, side > 0 ? h : -0.01f * h, y - 0.02f * h, y + 0.02f * h};
    auto centre = BoxCentre(model, box);
    return centre ? centre->x : side * 0.06f * h;
}

}

const char* MarkerLabel(Marker marker)
{
    static const char* labels[kMarkerCount] =
        {"Chin", "Groin", "Wrist L", "Wrist R", "Elbow L", "Elbow R", "Knee L", "Knee R", "Shoulder L", "Shoulder R"};
    return labels[static_cast<size_t>(marker)];
}

Marker MirrorOf(Marker marker)
{
    switch (marker)
    {
    case Marker::WristL:
        return Marker::WristR;
    case Marker::WristR:
        return Marker::WristL;
    case Marker::ElbowL:
        return Marker::ElbowR;
    case Marker::ElbowR:
        return Marker::ElbowL;
    case Marker::KneeL:
        return Marker::KneeR;
    case Marker::KneeR:
        return Marker::KneeL;
    case Marker::ShoulderL:
        return Marker::ShoulderR;
    case Marker::ShoulderR:
        return Marker::ShoulderL;
    default:
        return marker;
    }
}

bool IsOptional(Marker marker)
{
    return marker == Marker::ShoulderL || marker == Marker::ShoulderR;
}

MarkerSet GuessMarkers(const RigModel& model)
{
    float h = model.height;
    MarkerSet set;
    float crotch = CrotchHeight(model);
    At(set, Marker::Chin) = {0.0f, 0.875f * h};
    At(set, Marker::Groin) = {0.0f, crotch};
    float kneeY = 0.045f * h + 0.507f * (crotch + 0.005f * h);

    for (float side : {1.0f, -1.0f})
    {
        bool left = side > 0;
        Vec3 shoulder{side * 0.1f * h, 0.82f * h, 0.0f};
        Vec3 tip = OuterTip(model, side);
        Vec3 wrist = tip + Normalize(shoulder - tip) * (0.1f * h);
        Vec3 elbow = Lerp(shoulder, wrist, 0.5f);
        At(set, left ? Marker::WristL : Marker::WristR) = {wrist.x, wrist.y};
        At(set, left ? Marker::ElbowL : Marker::ElbowR) = {elbow.x, elbow.y};
        At(set, left ? Marker::KneeL : Marker::KneeR) = {LegCentreX(model, kneeY, side), kneeY};
        Vec3 estimate = EstimateShoulder(model, elbow, wrist, At(set, Marker::Chin).y, left);
        At(set, left ? Marker::ShoulderL : Marker::ShoulderR) = {estimate.x, estimate.y};
    }

    return set;
}

}
