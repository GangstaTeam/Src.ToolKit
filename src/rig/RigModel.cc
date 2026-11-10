#include "rig/RigModel.hh"

#include "rig/RigMath.hh"

#include <algorithm>

namespace tk
{

namespace
{

Mat4 OrientationMatrix(const Orientation& orientation)
{
    Mat4 flipZ;
    flipZ.m[2][2] = -1.0f;
    const Vec3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    Mat4 out = flipZ;

    for (int a = 0; a < 3; ++a)
    {
        out = out * AxisAngle(axes[a], Radians(90.0f * static_cast<float>(orientation.quarterTurns[a])));
    }

    return out;
}

}

RigModel PrepareModel(const SourceModel& source, const Orientation& orientation, float height)
{
    Mat4 rotation = OrientationMatrix(orientation);
    RigModel model;
    model.uvs = source.uvs;
    model.indices = source.indices;
    model.influences = source.influences;
    model.weights = source.weights;
    model.bones = source.bones;
    model.height = height;
    Vec3 lo{1e30f, 1e30f, 1e30f}, hi{-1e30f, -1e30f, -1e30f};

    for (Vec3 p : source.positions)
    {
        Vec3 q = TransformVector(p, rotation);
        model.positions.push_back(q);
        lo = {std::min(lo.x, q.x), std::min(lo.y, q.y), std::min(lo.z, q.z)};
        hi = {std::max(hi.x, q.x), std::max(hi.y, q.y), std::max(hi.z, q.z)};
    }

    for (Vec3 n : source.normals)
    {
        model.normals.push_back(Normalize(TransformVector(n, rotation)));
    }

    float scale = hi.y > lo.y ? height / (hi.y - lo.y) : 1.0f;
    Vec3 offset{(lo.x + hi.x) * 0.5f, lo.y, (lo.z + hi.z) * 0.5f};

    for (Vec3& p : model.positions)
    {
        p = (p - offset) * scale;
    }

    for (SourceBone& bone : model.bones)
    {
        bone.position = (TransformVector(bone.position, rotation) - offset) * scale;
    }

    return model;
}

}
