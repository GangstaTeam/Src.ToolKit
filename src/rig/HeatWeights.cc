#include "rig/HeatWeights.hh"

#include "rig/BoneSegments.hh"
#include "rig/MeshSmoother.hh"
#include "rig/RigMath.hh"
#include "rig/SparseSystem.hh"
#include "rig/TriangleGrid.hh"

#include <algorithm>
#include <cmath>
#include <future>

namespace tk
{

namespace
{

struct WeldedMesh
{
    std::vector<uint32_t> weld;
    std::vector<Vec3> positions;
    std::vector<Triangle> triangles;
};

WeldedMesh Weld(const RigModel& model)
{
    WeldedMesh mesh;
    uint32_t unique = 0;
    mesh.weld = WeldVertices(model.positions, unique);
    mesh.positions.resize(unique);

    for (size_t v = 0; v < mesh.weld.size(); ++v)
    {
        mesh.positions[mesh.weld[v]] = model.positions[v];
    }

    for (size_t t = 0; t + 2 < model.indices.size(); t += 3)
    {
        Triangle tri = {mesh.weld[model.indices[t]], mesh.weld[model.indices[t + 1]], mesh.weld[model.indices[t + 2]]};

        if (tri[0] != tri[1] && tri[1] != tri[2] && tri[0] != tri[2])
        {
            mesh.triangles.push_back(tri);
        }
    }

    return mesh;
}

struct Attachment
{
    std::vector<double> heat;
    std::vector<std::vector<uint32_t>> nearest;
};

Attachment AttachToBones(const WeldedMesh& mesh, const std::vector<Segment>& segments, float height)
{
    TriangleGrid grid(mesh.positions, mesh.triangles);
    Attachment attach;
    attach.heat.assign(mesh.positions.size(), 0.0);
    attach.nearest.resize(mesh.positions.size());
    float floor = 0.005f * height;
    std::vector<std::pair<float, uint32_t>> order(segments.size());

    for (uint32_t i = 0; i < mesh.positions.size(); ++i)
    {
        Vec3 p = mesh.positions[i];

        for (uint32_t s = 0; s < segments.size(); ++s)
        {
            order[s] = {ReachDistance(p, segments[s]), s};
        }

        std::sort(order.begin(), order.end());
        float best = order[0].first;

        for (size_t c = 0; c < std::min<size_t>(kVisibilityCandidates, order.size()); ++c)
        {
            uint32_t s = order[c].second;

            if (!grid.SegmentBlocked(p, ClosestOnSegment(p, segments[s]), i))
            {
                best = order[c].first;
                break;
            }
        }

        for (const auto& [distance, s] : order)
        {
            if (distance > best * 1.0001 + 1e-6f)
            {
                break;
            }

            if (distance >= best || !grid.SegmentBlocked(p, ClosestOnSegment(p, segments[s]), i))
            {
                attach.nearest[i].push_back(s);
            }
        }

        if (attach.nearest[i].empty())
        {
            attach.nearest[i].push_back(order[0].second);
        }

        double d = std::max(best, floor);
        attach.heat[i] = 1.0 / (d * d);
    }

    return attach;
}

float LimitWeight(double weight)
{
    float w = static_cast<float>(std::max(weight, 0.0));

    if (w >= 0.05f)
    {
        return w;
    }

    if (w <= 0.025f)
    {
        return 0.0f;
    }

    return 0.05f * (w - 0.025f) / (0.05f - 0.025f);
}

double Cotangent(Vec3 a, Vec3 b, Vec3 c)
{
    Vec3 u = b - a, v = c - a;
    double sine = Length(Cross(u, v));
    return sine > 1e-12 ? std::max(static_cast<double>(Dot(u, v)) / sine, 1e-4) : 1e-4;
}

SparseMatrix BuildSystem(const WeldedMesh& mesh, const Attachment& attach, std::vector<double>& areaHeat)
{
    size_t n = mesh.positions.size();
    std::vector<double> area(n, 0.0);
    SparseMatrix matrix(n);

    for (const Triangle& t : mesh.triangles)
    {
        Vec3 p[3] = {mesh.positions[t[0]], mesh.positions[t[1]], mesh.positions[t[2]]};
        double third = Length(Cross(p[1] - p[0], p[2] - p[0])) / 6.0;

        for (int k = 0; k < 3; ++k)
        {
            area[t[k]] += third;
            uint32_t a = t[(k + 1) % 3], b = t[(k + 2) % 3];
            double w = 0.5 * Cotangent(p[k], p[(k + 1) % 3], p[(k + 2) % 3]);
            matrix.Add(a, b, -w);
            matrix.Add(b, a, -w);
            matrix.Add(a, a, w);
            matrix.Add(b, b, w);
        }
    }

    areaHeat.resize(n);

    for (uint32_t i = 0; i < n; ++i)
    {
        areaHeat[i] = std::max(area[i], 1e-12) * attach.heat[i];
        matrix.Add(i, i, areaHeat[i]);
    }

    matrix.Finish();
    return matrix;
}

std::vector<double>
SolveBone(const SparseMatrix& matrix, const Attachment& attach, const std::vector<double>& areaHeat, uint32_t bone)
{
    size_t n = matrix.Size();
    std::vector<double> rhs(n, 0.0), x(n, 0.0);

    for (uint32_t i = 0; i < n; ++i)
    {
        const auto& nearest = attach.nearest[i];

        if (std::find(nearest.begin(), nearest.end(), bone) == nearest.end())
        {
            continue;
        }

        double share = 1.0 / static_cast<double>(nearest.size());
        rhs[i] = areaHeat[i] * share;
        x[i] = share;
    }

    if (!SolveConjugateGradient(matrix, rhs, x, kMaxIterations, 1e-6))
    {
        return {};
    }

    return x;
}

bool BothLegs(int a, int b)
{
    return (a == kLegL || a == kLegR) && (b == kLegL || b == kLegR);
}

void KeepToRegions(const WeldedMesh& mesh, const std::vector<Segment>& segments, float height, std::vector<float>& heat)
{
    size_t count = segments.size();
    float blend = 0.04f * height;
    std::vector<float> share(count);

    for (size_t i = 0; i < mesh.positions.size(); ++i)
    {
        float distance[kGroupCount];
        std::fill(distance, distance + kGroupCount, 1e30f);

        for (const Segment& segment : segments)
        {
            distance[segment.group] = std::min(distance[segment.group], SegmentDistance(mesh.positions[i], segment));
        }

        float nearest = *std::min_element(distance, distance + kGroupCount), membership[kGroupCount], members = 0.0f;

        for (int g = 0; g < kGroupCount; ++g)
        {
            members += membership[g] = std::exp(-(distance[g] - nearest) / blend);
        }

        float* values = &heat[i * count];
        float total = 0.0f;

        for (size_t s = 0; s < count; ++s)
        {
            float allowed = 0.0f;

            for (int g = 0; g < kGroupCount; ++g)
            {
                allowed += BothLegs(g, segments[s].group) || AllowedInfluence(g, segments[s]) ? membership[g] : 0.0f;
            }

            share[s] = values[s] * allowed / members;
            total += share[s];
        }

        if (total <= 1e-6f)
        {
            continue;
        }

        for (size_t s = 0; s < count; ++s)
        {
            values[s] = share[s] / total;
        }
    }
}

}

std::optional<SkinBinding>
ComputeHeatWeights(const RigModel& model, const JointTargets& targets, const GameSkeleton& skeleton)
{
    if (model.indices.empty())
    {
        return std::nullopt;
    }

    std::vector<Segment> segments = BuildSegments(model, targets);
    WeldedMesh mesh = Weld(model);
    Attachment attach = AttachToBones(mesh, segments, model.height);
    std::vector<double> areaHeat;
    SparseMatrix matrix = BuildSystem(mesh, attach, areaHeat);
    std::vector<std::future<std::vector<double>>> jobs;

    for (uint32_t s = 0; s < segments.size(); ++s)
    {
        jobs.push_back(std::async(std::launch::async,
            [&, s]
            {
                return SolveBone(matrix, attach, areaHeat, s);
            }));
    }

    size_t n = mesh.positions.size(), count = segments.size();
    std::vector<float> heat(n * count, 0.0f);

    for (size_t s = 0; s < count; ++s)
    {
        std::vector<double> solution = jobs[s].get();

        if (solution.size() != n)
        {
            return std::nullopt;
        }

        for (size_t i = 0; i < n; ++i)
        {
            heat[i * count + s] = LimitWeight(solution[i]);
        }
    }

    KeepToRegions(mesh, segments, model.height, heat);
    std::vector<int> jointOf, identity(skeleton.size());

    for (const Segment& segment : segments)
    {
        jointOf.push_back(std::max(0, FindJoint(skeleton, segment.joint)));
    }

    for (size_t j = 0; j < identity.size(); ++j)
    {
        identity[j] = static_cast<int>(j);
    }

    std::vector<float> perJoint(n * skeleton.size(), 0.0f);

    for (size_t i = 0; i < n; ++i)
    {
        for (size_t s = 0; s < count; ++s)
        {
            perJoint[i * skeleton.size() + static_cast<size_t>(jointOf[s])] += heat[i * count + s];
        }
    }

    SkinBinding binding;
    binding.joints.resize(model.positions.size());
    binding.weights.resize(model.positions.size());

    for (size_t v = 0; v < model.positions.size(); ++v)
    {
        KeepStrongest(&perJoint[mesh.weld[v] * skeleton.size()],
            skeleton.size(),
            identity,
            binding.joints[v],
            binding.weights[v]);
    }

    return binding;
}

}
