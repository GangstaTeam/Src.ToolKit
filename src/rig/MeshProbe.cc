#include "rig/MeshProbe.hh"

#include <algorithm>
#include <cmath>

namespace tk
{

std::optional<Vec3> BoxCentre(const RigModel& model, const ProbeBox& box)
{
    Vec3 lo{1e30f, 1e30f, 1e30f}, hi{-1e30f, -1e30f, -1e30f};
    bool found = false;

    for (Vec3 p : model.positions)
    {
        if (p.x < box.minX || p.x > box.maxX || p.y < box.minY || p.y > box.maxY)
        {
            continue;
        }

        lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
        hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
        found = true;
    }

    if (!found)
    {
        return std::nullopt;
    }

    return (lo + hi) * 0.5f;
}

float DepthAt(const RigModel& model, float x, float y, float radius)
{
    for (float r = radius; r < model.height; r *= 2.0f)
    {
        if (auto centre = BoxCentre(model, {x - r, x + r, y - r, y + r}))
        {
            return centre->z;
        }
    }

    return 0.0f;
}

std::optional<Vec3> SnapToMesh(const RigModel& model, float x, float y, float radius)
{
    std::optional<Vec3> centre;

    for (int pass = 0; pass < 2; ++pass)
    {
        float cx = centre ? centre->x : x, cy = centre ? centre->y : y;
        Vec3 lo{1e30f, 1e30f, 0.0f}, hi{-1e30f, -1e30f, 0.0f};
        bool found = false;

        for (Vec3 p : model.positions)
        {
            float dx = p.x - cx, dy = p.y - cy;

            if (dx * dx + dy * dy > radius * radius)
            {
                continue;
            }

            lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), 0.0f};
            hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), 0.0f};
            found = true;
        }

        if (!found)
        {
            return centre;
        }

        centre = (lo + hi) * 0.5f;
    }

    return centre;
}

float OuterExtent(const RigModel& model, float y, float band, bool left)
{
    float outer = 0.0f;

    for (Vec3 p : model.positions)
    {
        if (std::fabs(p.y - y) > band || (p.x > 0.0f) != left)
        {
            continue;
        }

        outer = std::max(outer, std::fabs(p.x));
    }

    return outer;
}

std::optional<float> ShoulderTop(const RigModel& model, float fromY, float toY, bool left)
{
    float h = model.height, step = 0.005f * h, neck = 1e30f;

    for (float y = fromY; y > toY; y -= step)
    {
        float outer = OuterExtent(model, y, step, left);

        if (outer <= 0.0f)
        {
            continue;
        }

        neck = std::min(neck, outer);

        if (outer > neck + 0.04f * h)
        {
            return y;
        }
    }

    return std::nullopt;
}

float CrotchHeight(const RigModel& model)
{
    float h = model.height, step = 0.005f * h, gap = 0.03f * h;
    std::vector<float> depths;

    for (Vec3 p : model.positions)
    {
        depths.push_back(p.z);
    }

    std::nth_element(depths.begin(), depths.begin() + static_cast<std::ptrdiff_t>(depths.size() / 2), depths.end());
    float median = depths.empty() ? 0.0f : depths[depths.size() / 2];
    size_t bands = static_cast<size_t>(std::ceil(0.48f * h / step));
    std::vector<int> hits(bands, 0);

    for (Vec3 p : model.positions)
    {
        if (p.z > median || std::fabs(p.x) >= gap || p.y < 0.12f * h)
        {
            continue;
        }

        size_t band = static_cast<size_t>((p.y - 0.12f * h) / step);

        if (band < bands)
        {
            ++hits[band];
        }
    }

    int run = 0;

    for (size_t band = 0; band < bands; ++band)
    {
        run = hits[band] >= 2 ? run + 1 : 0;

        if (run == 4)
        {
            return 0.12f * h + static_cast<float>(band - 3) * step;
        }
    }

    return 0.47f * h;
}

}
