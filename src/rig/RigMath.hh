#pragma once

#include "render/Math3d.hh"

namespace tk
{

struct Mat4
{
    float m[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};

    Vec3 Translation() const { return {m[3][0], m[3][1], m[3][2]}; }
    void SetTranslation(Vec3 t);
};

Mat4 operator*(const Mat4& a, const Mat4& b);
Vec3 TransformPoint(Vec3 p, const Mat4& m);
Vec3 TransformVector(Vec3 v, const Mat4& m);
Vec3 TransformByTranspose(Vec3 v, const Mat4& m);
Mat4 RotationOnly(const Mat4& m);
Mat4 InverseRigid(const Mat4& m);

Mat4 FromTrs(Vec3 translation, const float rotation[4], Vec3 scale);
Mat4 AxisAngle(Vec3 axis, float radians);
Mat4 RotationBetween(Vec3 from, Vec3 to);
Mat4 RotateAbout(Vec3 pivot, const Mat4& rotation);

float Length(Vec3 v);
Vec3 Lerp(Vec3 a, Vec3 b, float t);
float Radians(float degrees);

}
