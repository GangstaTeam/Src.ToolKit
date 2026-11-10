#pragma once

#include "render/Math3d.hh"

#include <array>
#include <cstdint>
#include <vector>

namespace tk
{

enum ETriangleGrid : int
{
    kMaxCellsPerAxis = 96,
};

using Triangle = std::array<uint32_t, 3>;

class TriangleGrid
{
public:
    TriangleGrid(const std::vector<Vec3>& positions, const std::vector<Triangle>& triangles);
    bool SegmentBlocked(Vec3 from, Vec3 to, uint32_t ignoreVertex) const;

private:
    int CellIndex(int x, int y, int z) const { return (z * m_size[1] + y) * m_size[0] + x; }
    void CellOf(Vec3 p, int out[3]) const;
    bool HitsTriangle(Vec3 from, Vec3 direction, const Triangle& triangle) const;

    const std::vector<Vec3>& m_positions;
    const std::vector<Triangle>& m_triangles;
    std::vector<std::vector<uint32_t>> m_cells;
    Vec3 m_origin;
    float m_cellSize = 1.0f;
    int m_size[3] = {1, 1, 1};
};

}
