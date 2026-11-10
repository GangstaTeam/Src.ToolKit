#include "render/OrbitCamera.hh"

#include <algorithm>
#include <cmath>

namespace tk
{

void OrbitCamera::Frame(const Bounds& bounds)
{
    float centre[3];
    bounds.Centre(centre);
    m_target = {centre[0], centre[1], centre[2]};
    m_radius = bounds.Radius();
    m_distance = m_radius * 2.6f;
    m_yaw = 0.0f;
    m_pitch = 0.25f;
}

void OrbitCamera::Orbit(float dx, float dy)
{
    m_yaw += dx * 0.01f;
    m_pitch = std::clamp(m_pitch + dy * 0.01f, -1.5f, 1.5f);
}

void OrbitCamera::Pan(float dx, float dy)
{
    Vec3 forward = Normalize(m_target - Eye());
    Vec3 right = Normalize(Cross({0, 1, 0}, forward));
    Vec3 up = Cross(forward, right);
    float scale = m_distance * 0.0015f;
    m_target = m_target - right * (dx * scale) + up * (dy * scale);
}

void OrbitCamera::Zoom(float wheel)
{
    m_distance = std::clamp(m_distance * std::pow(0.88f, wheel), m_radius * 0.05f, m_radius * 50.0f);
}

Vec3 OrbitCamera::Eye() const
{
    Vec3 offset{std::sin(m_yaw) * std::cos(m_pitch), std::sin(m_pitch), -std::cos(m_yaw) * std::cos(m_pitch)};
    return m_target + offset * m_distance;
}

D3DMATRIX OrbitCamera::View() const
{
    return LookAtLH(Eye(), m_target, {0, 1, 0});
}

D3DMATRIX OrbitCamera::Projection(float aspect) const
{
    return PerspectiveFovLH(0.8f, aspect, m_distance * 0.01f, m_distance + m_radius * 20.0f);
}

bool OrbitCamera::Project(Vec3 point, float width, float height, float& outX, float& outY) const
{
    D3DMATRIX view = View(), projection = Projection(width / height);
    float viewSpace[3] = {
        point.x * view._11 + point.y * view._21 + point.z * view._31 + view._41,
        point.x * view._12 + point.y * view._22 + point.z * view._32 + view._42,
        point.x * view._13 + point.y * view._23 + point.z * view._33 + view._43,
    };
    float clipX = viewSpace[0] * projection._11;
    float clipY = viewSpace[1] * projection._22;
    float clipW = viewSpace[2] * projection._34;

    if (clipW <= 0.0001f)
    {
        return false;
    }

    outX = (clipX / clipW * 0.5f + 0.5f) * width;
    outY = (0.5f - clipY / clipW * 0.5f) * height;
    return true;
}

}
