#pragma once

#include "render/MeshData.hh"
#include "render/Math3d.hh"

namespace tk
{

class FrontCamera
{
public:
    void Frame(const Bounds& bounds);
    void Pan(float dx, float dy, float viewHeight);
    void Zoom(float wheel);

    D3DMATRIX View() const;
    D3DMATRIX Projection(float aspect) const;
    void Project(Vec3 point, float width, float height, float& outX, float& outY) const;
    void Unproject(float x, float y, float width, float height, float& outX, float& outY) const;

private:
    Vec3 m_centre;
    float m_halfHeight = 1.0f;
    float m_depth = 10.0f;
};

}
