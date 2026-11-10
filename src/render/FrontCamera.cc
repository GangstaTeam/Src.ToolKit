#include "render/FrontCamera.hh"

#include <algorithm>
#include <cmath>

namespace tk
{

void FrontCamera::Frame(const Bounds& bounds)
{
    float centre[3];
    bounds.Centre(centre);
    m_centre = {centre[0], centre[1], centre[2]};
    m_halfHeight = std::max(0.01f, (bounds.max[1] - bounds.min[1]) * 0.55f);
    m_depth = bounds.Radius() * 4.0f + 1.0f;
}

void FrontCamera::Pan(float dx, float dy, float viewHeight)
{
    float scale = 2.0f * m_halfHeight / std::max(viewHeight, 1.0f);
    m_centre = m_centre + Vec3{-dx * scale, dy * scale, 0.0f};
}

void FrontCamera::Zoom(float wheel)
{
    m_halfHeight = std::clamp(m_halfHeight * std::pow(0.88f, wheel), 0.01f, 100.0f);
}

D3DMATRIX FrontCamera::View() const
{
    return LookAtLH(m_centre - Vec3{0.0f, 0.0f, m_depth * 0.5f}, m_centre, {0, 1, 0});
}

D3DMATRIX FrontCamera::Projection(float aspect) const
{
    return OrthoLH(2.0f * m_halfHeight * aspect, 2.0f * m_halfHeight, 0.0f, m_depth);
}

void FrontCamera::Project(Vec3 point, float width, float height, float& outX, float& outY) const
{
    float halfWidth = m_halfHeight * width / height;
    outX = ((point.x - m_centre.x) / halfWidth * 0.5f + 0.5f) * width;
    outY = (0.5f - (point.y - m_centre.y) / m_halfHeight * 0.5f) * height;
}

void FrontCamera::Unproject(float x, float y, float width, float height, float& outX, float& outY) const
{
    float halfWidth = m_halfHeight * width / height;
    outX = m_centre.x + (x / width - 0.5f) * 2.0f * halfWidth;
    outY = m_centre.y + (0.5f - y / height) * 2.0f * m_halfHeight;
}

}
