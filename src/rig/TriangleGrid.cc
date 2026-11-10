#include "rig/TriangleGrid.hh"

#include <algorithm>
#include <cmath>

namespace tk
{

TriangleGrid::TriangleGrid(const std::vector<Vec3>& positions, const std::vector<Triangle>& triangles)
    : m_positions(positions), m_triangles(triangles)
{
    Vec3 lo{1e30f, 1e30f, 1e30f}, hi{-1e30f, -1e30f, -1e30f};

    for (Vec3 p : positions)
    {
        lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
        hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
    }

    Vec3 extent = hi - lo;
    float volume = std::max(extent.x, 1e-3f) * std::max(extent.y, 1e-3f) * std::max(extent.z, 1e-3f);
    m_cellSize = std::cbrt(volume * 4.0f / std::max<float>(1.0f, static_cast<float>(triangles.size())));
    float axes[3] = {extent.x, extent.y, extent.z};

    for (int a = 0; a < 3; ++a)
    {
        m_size[a] = std::clamp<int>(static_cast<int>(std::ceil(axes[a] / m_cellSize)), 1, kMaxCellsPerAxis);
    }

    m_cellSize = std::max({extent.x / m_size[0], extent.y / m_size[1], extent.z / m_size[2], 1e-4f});
    m_origin = lo;
    m_cells.resize(static_cast<size_t>(m_size[0]) * m_size[1] * m_size[2]);

    for (uint32_t t = 0; t < triangles.size(); ++t)
    {
        int a[3], b[3];
        Vec3 p0 = positions[triangles[t][0]], p1 = positions[triangles[t][1]], p2 = positions[triangles[t][2]];
        CellOf({std::min({p0.x, p1.x, p2.x}), std::min({p0.y, p1.y, p2.y}), std::min({p0.z, p1.z, p2.z})}, a);
        CellOf({std::max({p0.x, p1.x, p2.x}), std::max({p0.y, p1.y, p2.y}), std::max({p0.z, p1.z, p2.z})}, b);

        for (int z = a[2]; z <= b[2]; ++z)
        {
            for (int y = a[1]; y <= b[1]; ++y)
            {
                for (int x = a[0]; x <= b[0]; ++x)
                {
                    m_cells[static_cast<size_t>(CellIndex(x, y, z))].push_back(t);
                }
            }
        }
    }
}

void TriangleGrid::CellOf(Vec3 p, int out[3]) const
{
    float c[3] = {(p.x - m_origin.x) / m_cellSize, (p.y - m_origin.y) / m_cellSize, (p.z - m_origin.z) / m_cellSize};

    for (int a = 0; a < 3; ++a)
    {
        out[a] = std::clamp(static_cast<int>(std::floor(c[a])), 0, m_size[a] - 1);
    }
}

bool TriangleGrid::HitsTriangle(Vec3 from, Vec3 direction, const Triangle& triangle) const
{
    Vec3 p0 = m_positions[triangle[0]], e1 = m_positions[triangle[1]] - p0, e2 = m_positions[triangle[2]] - p0;
    Vec3 h = Cross(direction, e2);
    float det = Dot(e1, h);

    if (std::fabs(det) < 1e-12f)
    {
        return false;
    }

    float inv = 1.0f / det;
    Vec3 s = from - p0;
    float u = Dot(s, h) * inv;

    if (u < -1e-4f || u > 1.0f + 1e-4f)
    {
        return false;
    }

    Vec3 q = Cross(s, e1);
    float v = Dot(direction, q) * inv;

    if (v < -1e-4f || u + v > 1.0f + 1e-4f)
    {
        return false;
    }

    float t = Dot(e2, q) * inv;
    return t > 1e-3f && t < 0.999f;
}

bool TriangleGrid::SegmentBlocked(Vec3 from, Vec3 to, uint32_t ignoreVertex) const
{
    Vec3 direction = to - from;
    float length = std::sqrt(Dot(direction, direction));
    int steps = std::max(1, static_cast<int>(std::ceil(length / (m_cellSize * 0.5f))));
    int last = -1;

    for (int i = 0; i <= steps; ++i)
    {
        int cell[3];
        CellOf(from + direction * (static_cast<float>(i) / static_cast<float>(steps)), cell);
        int index = CellIndex(cell[0], cell[1], cell[2]);

        if (index == last)
        {
            continue;
        }

        last = index;

        for (uint32_t t : m_cells[static_cast<size_t>(index)])
        {
            const Triangle& triangle = m_triangles[t];

            if (triangle[0] == ignoreVertex || triangle[1] == ignoreVertex || triangle[2] == ignoreVertex)
            {
                continue;
            }

            if (HitsTriangle(from, direction, triangle))
            {
                return true;
            }
        }
    }

    return false;
}

}
