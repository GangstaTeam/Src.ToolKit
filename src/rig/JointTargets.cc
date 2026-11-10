#include "rig/JointTargets.hh"

#include "rig/MeshProbe.hh"

#include <algorithm>
#include <cmath>
#include <map>

namespace tk
{

namespace
{

const char* const kRequiredJoints[] = {"Pelvis",
    "Head",
    "Shoulder_L",
    "Elbow_L",
    "Wrist_L",
    "Shoulder_R",
    "Elbow_R",
    "Wrist_R",
    "Hip_L",
    "Knee_L",
    "Ankle_L",
    "Hip_R",
    "Knee_R",
    "Ankle_R"};

std::string Side(const char* joint, bool left)
{
    return std::string(joint) + (left ? "_L" : "_R");
}

Vec3 OnSurface(const RigModel& model, MarkerPoint point, float radius)
{
    return {point.x, point.y, DepthAt(model, point.x, point.y, radius)};
}

Vec3 Ankle(const RigModel& model, Vec3 knee, bool left)
{
    float h = model.height, y = 0.045f * h;
    ProbeBox box{left ? 0.005f * h : -h, left ? h : -0.005f * h, y - 0.02f * h, y + 0.02f * h};
    auto centre = BoxCentre(model, box);
    return centre ? Vec3{centre->x, y, centre->z} : Vec3{knee.x, y, knee.z};
}

void AddRoots(JointTargets& targets)
{
    Vec3 pelvis = targets["Pelvis"];
    targets["Motion_Root"] = {};
    targets["Balance_Root"] = {0.0f, 0.737f * pelvis.y, 0.0f};
    targets["Character_Root"] = pelvis;
    targets["Spine_1"] = pelvis;
}

void AddArmDetails(JointTargets& targets, const RigModel& model, bool left)
{
    float sign = left ? 1.0f : -1.0f, h = model.height;
    Vec3 neck = targets["Neck"];

    if (!targets.count(Side("Clavicle", left)))
    {
        Vec3 shoulder = targets[Side("Shoulder", left)];
        float x = sign * std::max(0.3f * std::fabs(shoulder.x), 0.01f * h);
        float y = shoulder.y + 0.35f * (neck.y - shoulder.y);
        targets[Side("Clavicle", left)] = {x, y, DepthAt(model, x, y, 0.04f * h)};
    }

    targets[Side("Forarm", left)] = Lerp(targets[Side("Elbow", left)], targets[Side("Wrist", left)], 0.63f);
}

int Depth(const std::vector<SourceBone>& bones, size_t index)
{
    int depth = 0;

    for (int parent = bones[index].parent; parent >= 0 && depth < 1000;
        parent = bones[static_cast<size_t>(parent)].parent)
    {
        ++depth;
    }

    return depth;
}

}

Vec3 EstimateShoulder(const RigModel& model, Vec3 elbow, Vec3 wrist, float chinY, bool left)
{
    float sign = left ? 1.0f : -1.0f, height = model.height;
    float y = std::min(chinY - 0.04f * height, elbow.y + Length(wrist - elbow) * 1.06f);
    float upper = Length(wrist - elbow) * 1.06f, dy = y - elbow.y;
    float dx = std::sqrt(std::max(upper * upper - dy * dy, 0.0f));
    float x = std::clamp(std::fabs(elbow.x) - dx, 0.06f * height, std::fabs(elbow.x));
    auto top = ShoulderTop(model, chinY, elbow.y, left);

    if (top && elbow.y < *top - 0.08f * height)
    {
        y = *top - 0.03f * height;
        x = std::max(OuterExtent(model, y, 0.01f * height, left) - 0.03f * height, 0.06f * height);
    }

    return {sign * x, y, 0.0f};
}

JointTargets TargetsFromMarkers(const RigModel& model, const MarkerSet& markers, const MarkerFlags& placed)
{
    float h = model.height;
    MarkerPoint chin = markers[static_cast<size_t>(Marker::Chin)], groin = markers[static_cast<size_t>(Marker::Groin)];
    float hipY = groin.y + 0.05f * h, pelvisY = hipY + 0.026f * h;
    JointTargets t;
    t["Pelvis"] = {groin.x, pelvisY, DepthAt(model, groin.x, pelvisY, 0.08f * h)};
    float neckY = chin.y + 0.012f * h;
    t["Neck"] = {chin.x, neckY, DepthAt(model, chin.x, neckY, 0.04f * h)};
    t["Head"] = {chin.x, chin.y + 0.07f * h, t["Neck"].z};
    float spineY = pelvisY + 0.416f * (neckY - pelvisY);
    t["Spine_2"] = {groin.x, spineY, DepthAt(model, groin.x, spineY, 0.08f * h)};

    for (bool left : {true, false})
    {
        auto marker = [&](Marker l, Marker r)
        {
            return markers[static_cast<size_t>(left ? l : r)];
        };
        Vec3 wrist = OnSurface(model, marker(Marker::WristL, Marker::WristR), 0.04f * h);
        Vec3 elbow = OnSurface(model, marker(Marker::ElbowL, Marker::ElbowR), 0.04f * h);
        Marker shoulderMarker = left ? Marker::ShoulderL : Marker::ShoulderR;
        MarkerPoint manual = markers[static_cast<size_t>(shoulderMarker)];
        Vec3 shoulder = placed[static_cast<size_t>(shoulderMarker)]
            ? Vec3{manual.x, manual.y, 0.0f}
            : EstimateShoulder(model, elbow, wrist, chin.y, left);
        shoulder.z = DepthAt(model, shoulder.x, shoulder.y, 0.05f * h);
        Vec3 knee = OnSurface(model, marker(Marker::KneeL, Marker::KneeR), 0.04f * h);
        t[Side("Wrist", left)] = wrist;
        t[Side("Elbow", left)] = elbow;
        t[Side("Shoulder", left)] = shoulder;
        t[Side("Knee", left)] = knee;
        t[Side("Hip", left)] = {knee.x, hipY, t["Pelvis"].z};
        t[Side("Ankle", left)] = Ankle(model, knee, left);
        AddArmDetails(t, model, left);
    }

    AddRoots(t);
    return t;
}

JointTargets TargetsFromBones(const RigModel& model, const std::vector<std::string>& mapping, std::string& missing)
{
    std::map<std::string, std::pair<int, Vec3>> chosen;

    for (size_t i = 0; i < model.bones.size() && i < mapping.size(); ++i)
    {
        if (mapping[i].empty())
        {
            continue;
        }

        int depth = Depth(model.bones, i);
        auto found = chosen.find(mapping[i]);

        if (found == chosen.end() || depth < found->second.first)
        {
            chosen[mapping[i]] = {depth, model.bones[i].position};
        }
    }

    JointTargets t;

    for (const auto& [name, entry] : chosen)
    {
        t[name] = entry.second;
    }

    missing.clear();

    for (const char* name : kRequiredJoints)
    {
        if (!t.count(name))
        {
            missing += (missing.empty() ? "" : ", ") + std::string(name);
        }
    }

    if (!missing.empty())
    {
        return {};
    }

    float h = model.height;

    if (!t.count("Neck"))
    {
        t["Neck"] = t["Head"] - Vec3{0.0f, 0.057f * h, 0.0f};
    }

    if (!t.count("Spine_2"))
    {
        t["Spine_2"] = Lerp(t["Pelvis"], t["Neck"], 0.416f);
    }

    for (bool left : {true, false})
    {
        AddArmDetails(t, model, left);
    }

    AddRoots(t);
    return t;
}

}
