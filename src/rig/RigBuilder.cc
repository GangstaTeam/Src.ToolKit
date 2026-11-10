#include "rig/RigBuilder.hh"

#include "rig/BoneMapping.hh"
#include "rig/JointTargets.hh"

#include <algorithm>
#include <cmath>

namespace tk
{

namespace
{

const char* const kArmChain[] = {"Shoulder", "Elbow", "Forarm", "Wrist"};

struct ArmTurn
{
    bool active = false;
    std::string side;
    Vec3 shoulder;
    Vec3 axis;
    float angle = 0.0f;
    Mat4 rotation;
    Mat4 inverse;
};

ArmTurn ArmTurnFor(const JointTargets& targets, bool left)
{
    ArmTurn turn;
    turn.side = left ? "_L" : "_R";
    Vec3 shoulder = targets.at("Shoulder" + turn.side);
    Vec3 direction = Normalize(targets.at("Wrist" + turn.side) - shoulder);
    Vec3 goal{left ? 1.0f : -1.0f, 0.0f, 0.0f};

    if (Dot(direction, goal) > 0.9998f)
    {
        return turn;
    }

    turn.active = true;
    turn.shoulder = shoulder;
    turn.axis = Normalize(Cross(direction, goal));
    turn.angle = std::acos(std::clamp(Dot(direction, goal), -1.0f, 1.0f));
    turn.rotation = RotateAbout(shoulder, RotationBetween(direction, goal));
    turn.inverse = RotateAbout(shoulder, RotationBetween(goal, direction));
    return turn;
}

float ArmShare(float weight)
{
    const float floor = 0.05f;
    return std::clamp((weight - floor) / (1.0f - floor), 0.0f, 1.0f);
}

void StraightenMesh(RiggedCharacter& rig, const ArmTurn& turn)
{
    std::vector<int> chain;

    for (const char* joint : kArmChain)
    {
        chain.push_back(FindJoint(rig.skeleton, joint + turn.side));
    }

    for (size_t v = 0; v < rig.positions.size(); ++v)
    {
        float weight = 0.0f;

        for (size_t k = 0; k < 4; ++k)
        {
            for (int joint : chain)
            {
                weight += rig.binding.joints[v][k] == joint ? rig.binding.weights[v][k] : 0.0f;
            }
        }

        float share = ArmShare(weight);

        if (share <= 0.0f)
        {
            continue;
        }

        Mat4 partial = RotateAbout(turn.shoulder, AxisAngle(turn.axis, turn.angle * share));
        rig.positions[v] = TransformPoint(rig.positions[v], partial);
        rig.normals[v] = Normalize(TransformVector(rig.normals[v], partial));
    }
}

void TurnTargets(JointTargets& targets, const ArmTurn& turn)
{
    for (const char* joint : kArmChain)
    {
        targets[joint + turn.side] = TransformPoint(targets[joint + turn.side], turn.rotation);
    }
}

bool InSubtree(const GameSkeleton& skeleton, size_t index, int root)
{
    for (int guard = 0; guard < 64; ++guard)
    {
        if (static_cast<int>(index) == root)
        {
            return true;
        }

        int parent = skeleton[index].parent;

        if (index == 0 || parent < 0 || static_cast<size_t>(parent) >= index)
        {
            return false;
        }

        index = static_cast<size_t>(parent);
    }

    return false;
}

void TiltArm(GameSkeleton& skeleton, const Mat4& tilt, const std::string& side)
{
    int root = FindJoint(skeleton, "Shoulder" + side);

    if (root < 0)
    {
        return;
    }

    std::vector<Mat4> worlds = WorldMatrices(skeleton);

    for (size_t i = 0; i < skeleton.size(); ++i)
    {
        if (InSubtree(skeleton, i, root))
        {
            worlds[i] = worlds[i] * tilt;
        }
    }

    for (size_t i = 0; i < skeleton.size(); ++i)
    {
        if (!InSubtree(skeleton, i, root))
        {
            continue;
        }

        skeleton[i].local = worlds[i] * InverseRigid(worlds[static_cast<size_t>(skeleton[i].parent)]);
    }
}

GameSkeleton FinishSkeleton(const RiggedCharacter& rig, const GameSkeleton& base)
{
    GameSkeleton skeleton = FitSkeleton(base, rig.targets, rig.height);

    for (int s = 0; s < 2; ++s)
    {
        if (rig.armTilted[s])
        {
            TiltArm(skeleton, rig.armTilt[s], s == 0 ? "_L" : "_R");
        }
    }

    return skeleton;
}

}

std::optional<RiggedCharacter>
BuildRig(const RigModel& model, const RigRequest& request, const GameSkeleton& base, std::string& error)
{
    RiggedCharacter rig;
    rig.positions = model.positions;
    rig.normals = model.normals;
    rig.uvs = model.uvs;
    rig.indices = model.indices;
    rig.height = model.height;
    rig.skeleton = base;

    if (request.source == RigSource::ModelSkeleton)
    {
        std::string missing;
        rig.targets = TargetsFromBones(model, request.mapping, missing);

        if (!missing.empty())
        {
            error = "These game joints have no bone assigned: " + missing;
            return std::nullopt;
        }

        rig.binding = MapSourceWeights(model, ResolveMapping(model.bones, request.mapping), base);
    }
    else
    {
        rig.targets = TargetsFromMarkers(model, request.markers, request.placed);
        rig.binding = ComputeAutoWeights(model, rig.targets, base);
    }

    bool straightened = false;

    for (int s = 0; s < 2; ++s)
    {
        ArmTurn turn = ArmTurnFor(rig.targets, s == 0);

        if (!turn.active)
        {
            continue;
        }

        if (request.straightenArms)
        {
            StraightenMesh(rig, turn);
            straightened = true;
        }
        else
        {
            rig.armTilted[s] = true;
            rig.armTilt[s] = turn.inverse;
        }

        TurnTargets(rig.targets, turn);
    }

    if (straightened && request.source == RigSource::Markers)
    {
        RigModel straight = model;
        straight.positions = rig.positions;
        straight.normals = rig.normals;
        rig.binding = ComputeAutoWeights(straight, rig.targets, base);
    }

    rig.skeleton = FinishSkeleton(rig, base);
    return rig;
}

GameSkeleton StraightSkeleton(const RiggedCharacter& rig)
{
    GameSkeleton skeleton = rig.skeleton;

    for (int s = 0; s < 2; ++s)
    {
        if (rig.armTilted[s])
        {
            TiltArm(skeleton, InverseRigid(rig.armTilt[s]), s == 0 ? "_L" : "_R");
        }
    }

    return skeleton;
}

RiggedCharacter Rebind(const RiggedCharacter& rig, const GameSkeleton& base)
{
    RiggedCharacter out = rig;
    out.skeleton = FinishSkeleton(rig, base);

    for (Influences& joints : out.binding.joints)
    {
        for (int& joint : joints)
        {
            joint = std::max(0, FindJoint(base, rig.skeleton[static_cast<size_t>(joint)].name));
        }
    }

    return out;
}

}
