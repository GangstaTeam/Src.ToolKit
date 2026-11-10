#include "rig/BoneSegments.hh"

#include "rig/RigMath.hh"

#include <algorithm>

namespace tk
{

Vec3 ClosestOnSegment(Vec3 p, const Segment& s)
{
    Vec3 ab = s.b - s.a;
    float t = std::clamp(Dot(p - s.a, ab) / std::max(Dot(ab, ab), 1e-12f), 0.0f, 1.0f);
    return s.a + ab * t;
}

float SegmentDistance(Vec3 p, const Segment& s)
{
    return Length(p - ClosestOnSegment(p, s));
}

float ReachDistance(Vec3 p, const Segment& s)
{
    return SegmentDistance(p, s) / s.reach;
}

SegmentTuning& WeightTuning()
{
    static SegmentTuning tuning;
    return tuning;
}

bool AllowedInfluence(int vertexGroup, const Segment& segment)
{
    if (segment.group == vertexGroup)
    {
        return true;
    }

    const std::string& name = segment.joint;

    if (vertexGroup == kTorso)
    {
        return name.rfind("Clavicle", 0) == 0 || name.rfind("Shoulder", 0) == 0 || name.rfind("Hip", 0) == 0;
    }

    if (vertexGroup == kArmL || vertexGroup == kArmR)
    {
        return name == "Spine_2" || name.rfind("Clavicle", 0) == 0;
    }

    return name == "Pelvis";
}

std::vector<Segment> BuildSegments(const RigModel& model, JointTargets t)
{
    float h = model.height, top = 0.0f;

    for (Vec3 p : model.positions)
    {
        top = std::max(top, p.y);
    }

    Vec3 crotch{t["Pelvis"].x, t["Hip_L"].y - 0.05f * h, t["Pelvis"].z};
    Vec3 neckBase = Lerp(t["Spine_2"], t["Neck"], 0.75f);
    const SegmentTuning& tuning = WeightTuning();
    Vec3 waist = Lerp(t["Pelvis"], t["Spine_2"], tuning.pelvisUp);
    std::vector<Segment> segments = {
        {"Pelvis", waist, crotch, kTorso},
        {"Spine_1", waist, t["Spine_2"], kTorso},
        {"Spine_2", t["Spine_2"], neckBase, kTorso},
        {"Neck", neckBase, t["Neck"], kTorso},
        {"Head", t["Neck"], {t["Head"].x, top, t["Head"].z}, kTorso},
    };

    for (bool left : {true, false})
    {
        auto j = [&](const char* name)
        {
            return t[std::string(name) + (left ? "_L" : "_R")];
        };
        auto n = [&](const char* name)
        {
            return std::string(name) + (left ? "_L" : "_R");
        };
        int arm = left ? kArmL : kArmR, leg = left ? kLegL : kLegR;
        Vec3 tip = j("Wrist") + Normalize(j("Wrist") - j("Elbow")) * (0.11f * h);
        Vec3 toe = j("Ankle") + Vec3{0.0f, -j("Ankle").y, -0.08f * h};
        segments.push_back({n("Clavicle"), j("Clavicle"), j("Shoulder"), kTorso});
        segments.push_back({n("Shoulder"), j("Shoulder"), j("Elbow"), arm});
        segments.push_back({n("Elbow"), j("Elbow"), j("Forarm"), arm});
        segments.push_back({n("Forarm"), j("Forarm"), j("Wrist"), arm});
        segments.push_back({n("Wrist"), j("Wrist"), tip, arm});
        segments.push_back({n("Hip"), j("Hip"), j("Knee"), leg});
        segments.push_back({n("Knee"), j("Knee"), j("Ankle"), leg});
        segments.push_back({n("Ankle"), j("Ankle"), toe, leg});

        if (tuning.pelvisWings > 0.0f)
        {
            segments.push_back({"Pelvis", t["Pelvis"], Lerp(t["Pelvis"], j("Hip"), tuning.pelvisWings), kTorso});
        }
    }

    for (Segment& segment : segments)
    {
        const std::string& name = segment.joint;
        bool sided = name.size() > 2 &&
            (name.compare(name.size() - 2, 2, "_L") == 0 || name.compare(name.size() - 2, 2, "_R") == 0);
        std::string family = sided ? name.substr(0, name.size() - 2) : name;
        auto found = tuning.reach.find(family);

        if (found != tuning.reach.end())
        {
            segment.reach = found->second;
        }
    }

    return segments;
}

void KeepStrongest(const float* values,
    size_t count,
    const std::vector<int>& jointOf,
    Influences& joints,
    Weights& weights)
{
    std::vector<size_t> order(count);

    for (size_t i = 0; i < count; ++i)
    {
        order[i] = i;
    }

    size_t keep = std::min<size_t>(4, count);
    std::partial_sort(order.begin(),
        order.begin() + static_cast<std::ptrdiff_t>(keep),
        order.end(),
        [&](size_t a, size_t b)
        {
            return values[a] > values[b];
        });
    float total = 0.0f;

    for (size_t k = 0; k < keep; ++k)
    {
        total += values[order[k]];
    }

    joints = {0, 0, 0, 0};
    weights = {0, 0, 0, 0};

    for (size_t k = 0; k < keep; ++k)
    {
        joints[k] = jointOf[order[k]];
        weights[k] = total > 0.0f ? values[order[k]] / total : (k == 0 ? 1.0f : 0.0f);
    }
}

}
