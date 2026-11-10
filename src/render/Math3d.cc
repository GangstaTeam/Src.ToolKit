#include "render/Math3d.hh"

#include <cmath>

namespace tk
{

Vec3 operator+(Vec3 a, Vec3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vec3 operator-(Vec3 a, Vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3 operator*(Vec3 a, float s)
{
    return {a.x * s, a.y * s, a.z * s};
}
Vec3 Cross(Vec3 a, Vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
float Dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Normalize(Vec3 v)
{
    float length = std::sqrt(Dot(v, v));
    return length > 0.0f ? v * (1.0f / length) : v;
}

D3DMATRIX Identity()
{
    D3DMATRIX m = {};
    m._11 = m._22 = m._33 = m._44 = 1.0f;
    return m;
}

D3DMATRIX LookAtLH(Vec3 eye, Vec3 target, Vec3 up)
{
    Vec3 z = Normalize(target - eye);
    Vec3 x = Normalize(Cross(up, z));
    Vec3 y = Cross(z, x);
    D3DMATRIX m = Identity();
    m._11 = x.x;
    m._12 = y.x;
    m._13 = z.x;
    m._21 = x.y;
    m._22 = y.y;
    m._23 = z.y;
    m._31 = x.z;
    m._32 = y.z;
    m._33 = z.z;
    m._41 = -Dot(x, eye);
    m._42 = -Dot(y, eye);
    m._43 = -Dot(z, eye);
    return m;
}

D3DMATRIX OrthoLH(float width, float height, float zNear, float zFar)
{
    D3DMATRIX m = Identity();
    m._11 = 2.0f / width;
    m._22 = 2.0f / height;
    m._33 = 1.0f / (zFar - zNear);
    m._43 = -zNear / (zFar - zNear);
    return m;
}

D3DMATRIX PerspectiveFovLH(float fovY, float aspect, float zNear, float zFar)
{
    float yScale = 1.0f / std::tan(fovY * 0.5f);
    D3DMATRIX m = {};
    m._11 = yScale / aspect;
    m._22 = yScale;
    m._33 = zFar / (zFar - zNear);
    m._34 = 1.0f;
    m._43 = -zNear * zFar / (zFar - zNear);
    return m;
}

}
