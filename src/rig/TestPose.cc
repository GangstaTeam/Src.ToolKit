#include "rig/TestPose.hh"

#include <algorithm>
#include <cmath>

namespace tk
{

namespace
{

struct JointMove
{
    const char* joint;
    Vec3 axis;
    float degrees;
};

std::vector<JointMove> Moves(float phase)
{
    float swing = std::sin(phase), forward = std::max(swing, 0.0f), back = std::max(-swing, 0.0f);
    return {
        {"Shoulder_L", {0, 0, 1}, -65.0f},
        {"Shoulder_R", {0, 0, 1}, 65.0f},
        {"Elbow_L", {0, 1, 0}, 25.0f},
        {"Elbow_R", {0, 1, 0}, -25.0f},
        {"Hip_L", {1, 0, 0}, 35.0f * swing},
        {"Hip_R", {1, 0, 0}, -35.0f * swing},
        {"Knee_L", {1, 0, 0}, -50.0f * back},
        {"Knee_R", {1, 0, 0}, -50.0f * forward},
        {"Spine_2", {0, 1, 0}, 12.0f * swing},
        {"Head", {1, 0, 0}, -12.0f * swing},
    };
}

}

PosedCharacter PoseCharacter(const RiggedCharacter& rig, float phase)
{
    std::vector<Mat4> bind = WorldMatrices(rig.skeleton);
    std::vector<Mat4> straight = WorldMatrices(StraightSkeleton(rig));
    std::vector<Mat4> rotation(rig.skeleton.size());

    for (const JointMove& move : Moves(phase))
    {
        int joint = FindJoint(rig.skeleton, move.joint);

        if (joint >= 0)
        {
            rotation[static_cast<size_t>(joint)] = AxisAngle(move.axis, Radians(move.degrees));
        }
    }

    std::vector<Mat4> posed(rig.skeleton.size());
    PosedCharacter out;

    for (size_t i = 0; i < rig.skeleton.size(); ++i)
    {
        Mat4 local = RotateAbout(straight[i].Translation(), rotation[i]);
        int parent = rig.skeleton[i].parent;
        posed[i] = i == 0 || parent < 0 || static_cast<size_t>(parent) >= i
            ? local
            : local * posed[static_cast<size_t>(parent)];
        out.joints.push_back(TransformPoint(straight[i].Translation(), posed[i]));
    }

    for (size_t i = 0; i < posed.size(); ++i)
    {
        posed[i] = InverseRigid(bind[i]) * straight[i] * posed[i];
    }

    out.positions.resize(rig.positions.size());
    out.normals.resize(rig.normals.size());

    for (size_t v = 0; v < rig.positions.size(); ++v)
    {
        Vec3 p, n;

        for (size_t k = 0; k < 4; ++k)
        {
            float w = rig.binding.weights[v][k];

            if (w <= 0.0f)
            {
                continue;
            }

            const Mat4& m = posed[static_cast<size_t>(rig.binding.joints[v][k])];
            p = p + TransformPoint(rig.positions[v], m) * w;
            n = n + TransformVector(rig.normals[v], m) * w;
        }

        out.positions[v] = p;
        out.normals[v] = Normalize(n);
    }

    return out;
}

}
