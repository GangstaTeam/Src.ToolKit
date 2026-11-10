#include "rig/SkinWeights.hh"

#include "rig/BoneSegments.hh"
#include "rig/HeatWeights.hh"
#include "rig/MeshSmoother.hh"
#include "rig/RigMath.hh"

#include <algorithm>
#include <cmath>

namespace tk
{

SkinBinding ComputeDistanceWeights(const RigModel& model, const JointTargets& targets, const GameSkeleton& skeleton)
{
    std::vector<Segment> segments = BuildSegments(model, targets);
    size_t n = model.positions.size(), s = segments.size();
    float floor = 0.01f * model.height;
    std::vector<float> influence(n * s, 0.0f);
    std::vector<uint8_t> mask(n * s, 0);
    std::vector<float> distance(s);

    for (size_t v = 0; v < n; ++v)
    {
        size_t nearest = 0;

        for (size_t k = 0; k < s; ++k)
        {
            distance[k] = ReachDistance(model.positions[v], segments[k]);

            if (distance[k] < distance[nearest])
            {
                nearest = k;
            }
        }

        float total = 0.0f;

        for (size_t k = 0; k < s; ++k)
        {
            if (!AllowedInfluence(segments[nearest].group, segments[k]))
            {
                continue;
            }

            mask[v * s + k] = 1;
            influence[v * s + k] = 1.0f / std::pow(std::max(distance[k], floor), 4.0f);
            total += influence[v * s + k];
        }

        for (size_t k = 0; k < s; ++k)
        {
            influence[v * s + k] /= total;
        }
    }

    SmoothOverMesh(model, influence, mask, s, 4);
    std::vector<int> jointOf;

    for (const Segment& segment : segments)
    {
        jointOf.push_back(std::max(0, FindJoint(skeleton, segment.joint)));
    }

    SkinBinding binding;
    binding.joints.resize(n);
    binding.weights.resize(n);

    for (size_t v = 0; v < n; ++v)
    {
        KeepStrongest(&influence[v * s], s, jointOf, binding.joints[v], binding.weights[v]);
    }

    return binding;
}

SkinBinding ComputeAutoWeights(const RigModel& model, const JointTargets& targets, const GameSkeleton& skeleton)
{
    if (auto heat = ComputeHeatWeights(model, targets, skeleton))
    {
        return *heat;
    }

    return ComputeDistanceWeights(model, targets, skeleton);
}

SkinBinding
MapSourceWeights(const RigModel& model, const std::vector<std::string>& resolved, const GameSkeleton& skeleton)
{
    std::vector<int> jointOf;

    for (const GameJoint& joint : skeleton)
    {
        jointOf.push_back(FindJoint(skeleton, joint.name));
    }

    std::vector<int> boneJoint;

    for (const std::string& name : resolved)
    {
        boneJoint.push_back(std::max(0, FindJoint(skeleton, name)));
    }

    SkinBinding binding;
    binding.joints.resize(model.positions.size());
    binding.weights.resize(model.positions.size());
    std::vector<float> accum(skeleton.size());

    for (size_t v = 0; v < model.positions.size(); ++v)
    {
        std::fill(accum.begin(), accum.end(), 0.0f);

        for (size_t k = 0; k < 4 && v < model.influences.size(); ++k)
        {
            size_t bone = static_cast<size_t>(model.influences[v][k]);

            if (bone < boneJoint.size())
            {
                accum[static_cast<size_t>(boneJoint[bone])] += model.weights[v][k];
            }
        }

        KeepStrongest(accum.data(), accum.size(), jointOf, binding.joints[v], binding.weights[v]);
    }

    return binding;
}

}
