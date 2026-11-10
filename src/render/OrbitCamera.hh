#pragma once

#include "render/MeshData.hh"
#include "render/Math3d.hh"

namespace tk
{

class OrbitCamera
{
public:
    void Frame(const Bounds& bounds);
    void Orbit(float dx, float dy);
    void Pan(float dx, float dy);
    void Zoom(float wheel);

    Vec3 Eye() const;
    D3DMATRIX View() const;
    D3DMATRIX Projection(float aspect) const;
    bool Project(Vec3 point, float width, float height, float& outX, float& outY) const;
    float Radius() const { return m_radius; }

private:
    Vec3 m_target;
    float m_yaw = 0.0f;
    float m_pitch = 0.25f;
    float m_distance = 3.0f;
    float m_radius = 1.0f;
};

}
