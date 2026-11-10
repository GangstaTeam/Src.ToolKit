#include "rig/BoneSegments.hh"
#include "rig/GameSkeleton.hh"
#include "rig/FbxConvert.hh"
#include "rig/ImportSession.hh"
#include "rig/MeshSmoother.hh"
#include "rig/RigMath.hh"
#include "rig/SkinWeights.hh"
#include "rig/TestPose.hh"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

using namespace tk;

namespace
{

const char* const kGroupNames[] = {"torso", "arm_L", "arm_R", "leg_L", "leg_R"};

std::filesystem::path ConvertedModel(const std::filesystem::path& model)
{
    if (model.extension() != ".fbx" && model.extension() != ".FBX")
    {
        return model;
    }

    std::filesystem::path glb = std::filesystem::temp_directory_path() / L"weightcheck_import.glb";
    std::string error;

    if (!ConvertFbx(model, glb, error))
    {
        std::printf("fbx: %s\n", error.c_str());
    }

    return glb;
}

std::vector<int> JointGroups(const RiggedCharacter& rig, const std::vector<Segment>& segments)
{
    std::vector<int> groups(rig.skeleton.size(), -1);

    for (const Segment& segment : segments)
    {
        int joint = FindJoint(rig.skeleton, segment.joint);

        if (joint >= 0)
        {
            groups[static_cast<size_t>(joint)] = segment.group;
        }
    }

    return groups;
}

void ReportLeakage(const RiggedCharacter& rig)
{
    RigModel model;
    model.positions = rig.positions;
    model.height = rig.height;
    std::vector<Segment> segments = BuildSegments(model, rig.targets);
    std::vector<int> groups = JointGroups(rig, segments);
    std::vector<const Segment*> segmentOf(rig.skeleton.size(), nullptr);

    for (const Segment& segment : segments)
    {
        int joint = FindJoint(rig.skeleton, segment.joint);

        if (joint >= 0)
        {
            segmentOf[static_cast<size_t>(joint)] = &segment;
        }
    }

    size_t leaking = 0, severe = 0;
    float worst = 0.0f;
    int matrix[5][5] = {};

    for (size_t v = 0; v < rig.positions.size(); ++v)
    {
        int home = groups[static_cast<size_t>(rig.binding.joints[v][0])];

        if (home < 0)
        {
            continue;
        }

        float leak = 0.0f;

        for (size_t k = 0; k < 4; ++k)
        {
            int joint = rig.binding.joints[v][k];
            float w = rig.binding.weights[v][k];
            const Segment* segment = segmentOf[static_cast<size_t>(joint)];

            if (w <= 0.0f || !segment || AllowedInfluence(home, *segment))
            {
                continue;
            }

            leak += w;

            if (w > 0.05f)
            {
                ++matrix[home][segment->group];
            }
        }

        worst = std::max(worst, leak);
        leaking += leak > 0.05f;
        severe += leak > 0.25f;
    }

    std::vector<size_t> primary(rig.skeleton.size(), 0);

    for (const Influences& joints : rig.binding.joints)
    {
        ++primary[static_cast<size_t>(joints[0])];
    }

    std::printf("primary:");

    for (size_t j = 0; j < primary.size(); ++j)
    {
        if (primary[j])
        {
            std::printf(" %s=%zu", rig.skeleton[j].name.c_str(), primary[j]);
        }
    }

    std::printf("\n");
    std::printf("leakage: %zu vertices >5%%, %zu vertices >25%%, worst %.2f\n", leaking, severe, worst);

    for (int a = 0; a < 5; ++a)
    {
        for (int b = 0; b < 5; ++b)
        {
            if (matrix[a][b])
            {
                std::printf("  %-6s <- %-6s %d\n", kGroupNames[a], kGroupNames[b], matrix[a][b]);
            }
        }
    }
}

void ReportDifference(const RiggedCharacter& rig, const SkinBinding& automatic)
{
    size_t n = rig.positions.size(), joints = rig.skeleton.size();
    std::vector<float> a(joints), b(joints), jointError(joints, 0.0f), jointMass(joints, 0.0f);
    double total = 0.0;
    size_t bad = 0;

    for (size_t v = 0; v < n; ++v)
    {
        std::fill(a.begin(), a.end(), 0.0f);
        std::fill(b.begin(), b.end(), 0.0f);

        for (size_t k = 0; k < 4; ++k)
        {
            a[static_cast<size_t>(rig.binding.joints[v][k])] += rig.binding.weights[v][k];
            b[static_cast<size_t>(automatic.joints[v][k])] += automatic.weights[v][k];
        }

        float error = 0.0f;

        for (size_t j = 0; j < joints; ++j)
        {
            float d = std::fabs(a[j] - b[j]);
            error += d;
            jointError[j] += d;
            jointMass[j] += a[j];
        }

        error *= 0.5f;
        total += error;
        bad += error > 0.5f;
    }

    std::printf("difference: mean %.3f, %zu vertices (%.1f%%) differ by more than half\n",
        total / static_cast<double>(n),
        bad,
        100.0 * static_cast<double>(bad) / static_cast<double>(n));
    for (size_t j = 0; j < joints; ++j)
    {
        if (jointMass[j] > 1.0f)
        {
            std::printf("  %-14s source mass %7.0f  error %7.0f\n",
                rig.skeleton[j].name.c_str(),
                jointMass[j],
                jointError[j]);
        }
    }
}

bool ApplyTuning(const std::string& setting)
{
    size_t equals = setting.find('=');

    if (equals == std::string::npos)
    {
        return false;
    }

    std::string key = setting.substr(0, equals);
    float value = static_cast<float>(std::atof(setting.c_str() + equals + 1));
    SegmentTuning& tuning = WeightTuning();

    if (key == "pelvisUp")
    {
        tuning.pelvisUp = value;
    }
    else if (key == "pelvisWings")
    {
        tuning.pelvisWings = value;
    }
    else if (key.rfind("reach.", 0) == 0)
    {
        tuning.reach[key.substr(6)] = value;
    }
    else
    {
        std::printf("unknown tuning %s\n", key.c_str());
        return false;
    }

    return true;
}

void Dump(const RiggedCharacter& rig, const char* path)
{
    FILE* file = std::fopen(path, "wb");

    if (!file)
    {
        return;
    }

    uint32_t sizes[2] = {static_cast<uint32_t>(rig.positions.size()), static_cast<uint32_t>(rig.indices.size())};
    std::fwrite(sizes, sizeof(sizes), 1, file);
    std::fwrite(rig.positions.data(), sizeof(Vec3), rig.positions.size(), file);

    for (float phase : {0.5f * 3.14159265f, 1.5f * 3.14159265f})
    {
        PosedCharacter posed = PoseCharacter(rig, phase);
        std::fwrite(posed.positions.data(), sizeof(Vec3), posed.positions.size(), file);
    }

    std::fwrite(rig.indices.data(), sizeof(uint32_t), rig.indices.size(), file);
    std::fwrite(rig.binding.joints.data(), sizeof(Influences), rig.binding.joints.size(), file);
    std::fwrite(rig.binding.weights.data(), sizeof(Weights), rig.binding.weights.size(), file);
    std::fclose(file);
}

void ReportStretch(const RiggedCharacter& rig)
{
    float worst = 0.0f, spike = 0.05f * rig.height;
    size_t spikes = 0, edges = 0;
    Vec3 worstAt;

    for (float phase : {0.0f, 0.5f * 3.14159265f, 3.14159265f, 1.5f * 3.14159265f})
    {
        PosedCharacter posed = PoseCharacter(rig, phase);

        for (size_t t = 0; t + 2 < rig.indices.size(); t += 3)
        {
            for (int e = 0; e < 3; ++e)
            {
                uint32_t a = rig.indices[t + e], b = rig.indices[t + (e + 1) % 3];
                float growth =
                    Length(posed.positions[a] - posed.positions[b]) - Length(rig.positions[a] - rig.positions[b]);
                ++edges;
                spikes += growth > spike;

                if (growth > worst)
                {
                    worst = growth;
                    worstAt = rig.positions[a];
                }
            }
        }
    }

    std::printf("stretch: worst +%.1f cm at (%.3f %.3f %.3f), %zu edges grow >%.0f cm (of %zu)\n",
        worst * 100.0f,
        worstAt.x,
        worstAt.y,
        worstAt.z,
        spikes,
        spike * 100.0f,
        edges);
}

}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::printf("usage: weightcheck <model.glb|fbx> [--height 1.8] [--distance] [--keep-pose] [--rotate x y z] "
                    "[--dump file] [--reference] [--tune key=value]\n");
        return 1;
    }

    float height = 1.8f;
    bool distance = false, keepPose = false, reference = false;
    const char* dump = nullptr;
    int turns[3] = {0, 0, 0};

    for (int i = 2; i < argc; ++i)
    {
        if (!std::strcmp(argv[i], "--height") && i + 1 < argc)
        {
            height = static_cast<float>(std::atof(argv[++i]));
        }
        else if (!std::strcmp(argv[i], "--distance"))
        {
            distance = true;
        }
        else if (!std::strcmp(argv[i], "--reference"))
        {
            reference = true;
        }
        else if (!std::strcmp(argv[i], "--tune") && i + 1 < argc && !ApplyTuning(argv[++i]))
        {
            return 1;
        }
        else if (!std::strcmp(argv[i], "--keep-pose"))
        {
            keepPose = true;
        }
        else if (!std::strcmp(argv[i], "--dump") && i + 1 < argc)
        {
            dump = argv[++i];
        }
        else if (!std::strcmp(argv[i], "--rotate") && i + 3 < argc)
        {
            for (int& turn : turns)
            {
                turn = std::atoi(argv[++i]);
            }
        }
    }

    ImportSession session;
    std::string error;

    if (!session.Load(ConvertedModel(std::filesystem::u8path(argv[1])), error))
    {
        std::printf("load: %s\n", error.c_str());
        return 1;
    }

    for (int axis = 0; axis < 3; ++axis)
    {
        if (turns[axis])
        {
            session.Rotate(axis, turns[axis]);
        }
    }

    session.SetHeight(height);

    if (reference)
    {
        if (session.Model().bones.empty())
        {
            std::printf("reference: the model has no skin\n");
            return 1;
        }

        session.Request().source = RigSource::ModelSkeleton;
        session.Request().straightenArms = !keepPose;

        if (!session.Build(DefaultGameSkeleton(), error))
        {
            std::printf("rig: %s\n", error.c_str());
            return 1;
        }

        RiggedCharacter rig = *session.Rig();
        RigModel model = session.Model();
        model.positions = rig.positions;
        model.normals = rig.normals;
        auto start = std::chrono::steady_clock::now();
        SkinBinding automatic = distance ? ComputeDistanceWeights(model, rig.targets, DefaultGameSkeleton())
                                         : ComputeAutoWeights(model, rig.targets, DefaultGameSkeleton());
        std::printf("model: %zu vertices, weights in %.2fs\n",
            model.positions.size(),
            std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count());
        std::printf("-- source weights\n");
        ReportLeakage(rig);
        ReportStretch(rig);

        if (dump)
        {
            Dump(rig, (std::string(dump) + ".ref").c_str());
        }

        ReportDifference(rig, automatic);
        rig.binding = automatic;
        std::printf("-- automatic weights\n");
        ReportLeakage(rig);
        ReportStretch(rig);

        if (dump)
        {
            Dump(rig, dump);
        }

        return 0;
    }

    session.Request().source = RigSource::Markers;
    session.Request().straightenArms = !keepPose;
    session.GuessMarkers();
    std::printf("model: %zu vertices, %zu triangles\n",
        session.Model().positions.size(),
        session.Model().indices.size() / 3);
    auto start = std::chrono::steady_clock::now();

    if (!session.Build(DefaultGameSkeleton(), error))
    {
        std::printf("rig: %s\n", error.c_str());
        return 1;
    }

    std::printf("build: %.2fs\n", std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count());
    RiggedCharacter rig = *session.Rig();

    if (distance)
    {
        RigModel model = session.Model();
        model.positions = rig.positions;
        model.normals = rig.normals;
        rig.binding = ComputeDistanceWeights(model, rig.targets, DefaultGameSkeleton());
        std::printf("weights: distance\n");
    }

    ReportLeakage(rig);
    ReportStretch(rig);

    if (dump)
    {
        Dump(rig, dump);
    }

    return 0;
}
