#include "rig/RigMath.hh"

#include <cmath>

namespace tk
{

void Mat4::SetTranslation(Vec3 t)
{
    m[3][0] = t.x;
    m[3][1] = t.y;
    m[3][2] = t.z;
}

Mat4 operator*(const Mat4& a, const Mat4& b)
{
    Mat4 out;

    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            out.m[r][c] = a.m[r][0] * b.m[0][c] + a.m[r][1] * b.m[1][c] + a.m[r][2] * b.m[2][c] + a.m[r][3] * b.m[3][c];
        }
    }

    return out;
}

Vec3 TransformVector(Vec3 v, const Mat4& m)
{
    return {v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
        v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
        v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2]};
}

Vec3 TransformPoint(Vec3 p, const Mat4& m)
{
    return TransformVector(p, m) + m.Translation();
}

Vec3 TransformByTranspose(Vec3 v, const Mat4& m)
{
    return {v.x * m.m[0][0] + v.y * m.m[0][1] + v.z * m.m[0][2],
        v.x * m.m[1][0] + v.y * m.m[1][1] + v.z * m.m[1][2],
        v.x * m.m[2][0] + v.y * m.m[2][1] + v.z * m.m[2][2]};
}

Mat4 RotationOnly(const Mat4& m)
{
    Mat4 out = m;
    out.SetTranslation({});
    return out;
}

Mat4 InverseRigid(const Mat4& m)
{
    Mat4 out;

    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 3; ++c)
        {
            out.m[r][c] = m.m[c][r];
        }
    }

    out.SetTranslation(TransformVector(m.Translation(), out) * -1.0f);
    return out;
}

Mat4 FromTrs(Vec3 translation, const float q[4], Vec3 scale)
{
    float x = q[0], y = q[1], z = q[2], w = q[3];
    float rows[3][3] = {
        {1 - 2 * (y * y + z * z), 2 * (x * y + z * w), 2 * (x * z - y * w)},
        {2 * (x * y - z * w), 1 - 2 * (x * x + z * z), 2 * (y * z + x * w)},
        {2 * (x * z + y * w), 2 * (y * z - x * w), 1 - 2 * (x * x + y * y)},
    };
    float s[3] = {scale.x, scale.y, scale.z};
    Mat4 out;

    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 3; ++c)
        {
            out.m[r][c] = rows[r][c] * s[r];
        }
    }

    out.SetTranslation(translation);
    return out;
}

Mat4 AxisAngle(Vec3 axis, float radians)
{
    Vec3 k = Normalize(axis);
    float c = std::cos(radians), s = std::sin(radians), t = 1.0f - c;
    Mat4 out;
    float cols[3][3] = {
        {t * k.x * k.x + c, t * k.x * k.y - s * k.z, t * k.x * k.z + s * k.y},
        {t * k.x * k.y + s * k.z, t * k.y * k.y + c, t * k.y * k.z - s * k.x},
        {t * k.x * k.z - s * k.y, t * k.y * k.z + s * k.x, t * k.z * k.z + c},
    };

    for (int r = 0; r < 3; ++r)
    {
        for (int c2 = 0; c2 < 3; ++c2)
        {
            out.m[r][c2] = cols[c2][r];
        }
    }

    return out;
}

Mat4 RotationBetween(Vec3 from, Vec3 to)
{
    Vec3 a = Normalize(from), b = Normalize(to);
    Vec3 axis = Cross(a, b);
    float sine = Length(axis), cosine = Dot(a, b);

    if (sine < 1e-6f)
    {
        return cosine > 0.0f ? Mat4() : AxisAngle(std::fabs(a.x) < 0.9f ? Vec3{1, 0, 0} : Vec3{0, 1, 0}, 3.14159265f);
    }

    return AxisAngle(axis, std::atan2(sine, cosine));
}

Mat4 RotateAbout(Vec3 pivot, const Mat4& rotation)
{
    Mat4 out = RotationOnly(rotation);
    out.SetTranslation(pivot - TransformVector(pivot, out));
    return out;
}

float Length(Vec3 v)
{
    return std::sqrt(Dot(v, v));
}

Vec3 Lerp(Vec3 a, Vec3 b, float t)
{
    return a + (b - a) * t;
}

float Radians(float degrees)
{
    return degrees * 3.14159265f / 180.0f;
}

}
