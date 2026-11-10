#include "rig/MeshSmoother.hh"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace tk
{

namespace
{

uint64_t QuantizedKey(Vec3 p)
{
    auto q = [](float v)
    {
        return static_cast<uint64_t>(static_cast<int64_t>(std::llround(v * 1e5f)) + (1 << 20)) & 0x1FFFFF;
    };
    return q(p.x) | (q(p.y) << 21) | (q(p.z) << 42);
}

}

std::vector<uint32_t> WeldVertices(const std::vector<Vec3>& positions, uint32_t& uniqueCount)
{
    std::unordered_map<uint64_t, uint32_t> ids;
    std::vector<uint32_t> weld;
    weld.reserve(positions.size());

    for (Vec3 p : positions)
    {
        weld.push_back(ids.emplace(QuantizedKey(p), static_cast<uint32_t>(ids.size())).first->second);
    }

    uniqueCount = static_cast<uint32_t>(ids.size());
    return weld;
}

void SmoothOverMesh(const RigModel& model,
    std::vector<float>& values,
    const std::vector<uint8_t>& mask,
    size_t stride,
    int iterations)
{
    uint32_t unique = 0;
    std::vector<uint32_t> weld = WeldVertices(model.positions, unique);
    std::vector<float> counts(unique, 0.0f), degree(unique, 0.0f);

    for (uint32_t w : weld)
    {
        counts[w] += 1.0f;
    }

    for (size_t t = 0; t + 2 < model.indices.size(); t += 3)
    {
        for (int e = 0; e < 3; ++e)
        {
            degree[weld[model.indices[t + e]]] += 1.0f;
            degree[weld[model.indices[t + (e + 1) % 3]]] += 1.0f;
        }
    }

    std::vector<float> shared(unique * stride), neighbours(unique * stride);

    for (int iteration = 0; iteration < iterations; ++iteration)
    {
        std::fill(shared.begin(), shared.end(), 0.0f);
        std::fill(neighbours.begin(), neighbours.end(), 0.0f);

        for (size_t v = 0; v < weld.size(); ++v)
        {
            for (size_t k = 0; k < stride; ++k)
            {
                shared[weld[v] * stride + k] += values[v * stride + k] / counts[weld[v]];
            }
        }

        for (size_t t = 0; t + 2 < model.indices.size(); t += 3)
        {
            for (int e = 0; e < 3; ++e)
            {
                uint32_t a = weld[model.indices[t + e]], b = weld[model.indices[t + (e + 1) % 3]];

                for (size_t k = 0; k < stride; ++k)
                {
                    neighbours[a * stride + k] += shared[b * stride + k];
                    neighbours[b * stride + k] += shared[a * stride + k];
                }
            }
        }

        for (size_t v = 0; v < weld.size(); ++v)
        {
            uint32_t w = weld[v];
            float total = 0.0f;

            for (size_t k = 0; k < stride; ++k)
            {
                float smoothed =
                    0.5f * shared[w * stride + k] + 0.5f * neighbours[w * stride + k] / std::max(degree[w], 1.0f);
                values[v * stride + k] = mask[v * stride + k] ? smoothed : 0.0f;
                total += values[v * stride + k];
            }

            for (size_t k = 0; k < stride && total > 0.0f; ++k)
            {
                values[v * stride + k] /= total;
            }
        }
    }
}

}
