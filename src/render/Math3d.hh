#pragma once

#include <d3d9.h>

namespace tk
{

struct Vec3
{
    float x = 0, y = 0, z = 0;
};

Vec3 operator+(Vec3 a, Vec3 b);
Vec3 operator-(Vec3 a, Vec3 b);
Vec3 operator*(Vec3 a, float s);
Vec3 Cross(Vec3 a, Vec3 b);
float Dot(Vec3 a, Vec3 b);
Vec3 Normalize(Vec3 v);

D3DMATRIX Identity();
D3DMATRIX LookAtLH(Vec3 eye, Vec3 target, Vec3 up);
D3DMATRIX PerspectiveFovLH(float fovY, float aspect, float zNear, float zFar);
D3DMATRIX OrthoLH(float width, float height, float zNear, float zFar);

}
