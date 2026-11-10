#include "rig/WeightPaint.hh"

#include "rig/MeshSmoother.hh"

#include <algorithm>

namespace tk
{

namespace
{

int SlotOf(const Influences& joints, const Weights& weights, int joint)
{
    for (int k = 0; k < 4; ++k)
    {
        if (joints[static_cast<size_t>(k)] == joint && weights[static_cast<size_t>(k)] > 0.0f)
        {
            return k;
        }
    }

    return -1;
}

int WeakestSlot(const Weights& weights)
{
    int weakest = 0;

    for (int k = 1; k < 4; ++k)
    {
        if (weights[static_cast<size_t>(k)] < weights[static_cast<size_t>(weakest)])
        {
            weakest = k;
        }
    }

    return weakest;
}

void SortByWeight(Influences& joints, Weights& weights)
{
    for (size_t a = 0; a < 4; ++a)
    {
        for (size_t b = a + 1; b < 4; ++b)
        {
            if (weights[b] > weights[a])
            {
                std::swap(weights[a], weights[b]);
                std::swap(joints[a], joints[b]);
            }
        }
    }
}

}

float JointWeight(const SkinBinding& binding, size_t vertex, int joint)
{
    int slot = SlotOf(binding.joints[vertex], binding.weights[vertex], joint);
    return slot < 0 ? 0.0f : binding.weights[vertex][static_cast<size_t>(slot)];
}

void SetJointWeight(RiggedCharacter& rig, size_t vertex, int joint, float weight)
{
    Influences& joints = rig.binding.joints[vertex];
    Weights& weights = rig.binding.weights[vertex];
    weight = std::clamp(weight, 0.0f, 1.0f);
    int slot = SlotOf(joints, weights, joint);

    if (slot < 0)
    {
        if (weight <= 0.0f)
        {
            return;
        }

        slot = WeakestSlot(weights);
        joints[static_cast<size_t>(slot)] = joint;
        weights[static_cast<size_t>(slot)] = 0.0f;
    }

    float others = 0.0f;

    for (size_t k = 0; k < 4; ++k)
    {
        others += k == static_cast<size_t>(slot) ? 0.0f : weights[k];
    }

    if (others <= 1e-6f && weight < 1.0f)
    {
        int parent = rig.skeleton[static_cast<size_t>(joint)].parent;
        size_t free = static_cast<size_t>((slot + 1) % 4);
        joints[free] = parent >= 0 && parent != joint ? parent : 0;
        weights[free] = 1.0f - weight;
    }
    else
    {
        float scale = others > 0.0f ? (1.0f - weight) / others : 0.0f;

        for (size_t k = 0; k < 4; ++k)
        {
            if (k != static_cast<size_t>(slot))
            {
                weights[k] *= scale;
            }
        }
    }

    weights[static_cast<size_t>(slot)] = weight;
    SortByWeight(joints, weights);
}

void WeightPainter::Prepare(const RiggedCharacter& rig)
{
    uint32_t unique = 0;
    m_weld = WeldVertices(rig.positions, unique);
    m_representative.assign(unique, 0);

    for (uint32_t v = static_cast<uint32_t>(m_weld.size()); v-- > 0;)
    {
        m_representative[m_weld[v]] = v;
    }

    m_neighbours.assign(unique, {});

    for (size_t t = 0; t + 2 < rig.indices.size(); t += 3)
    {
        for (int e = 0; e < 3; ++e)
        {
            uint32_t a = m_weld[rig.indices[t + e]], b = m_weld[rig.indices[t + (e + 1) % 3]];
            m_neighbours[a].push_back(b);
            m_neighbours[b].push_back(a);
        }
    }
}

float WeightPainter::NeighbourAverage(const SkinBinding& binding, uint32_t vertex, int joint) const
{
    const std::vector<uint32_t>& around = m_neighbours[m_weld[vertex]];

    if (around.empty())
    {
        return JointWeight(binding, vertex, joint);
    }

    float total = 0.0f;

    for (uint32_t id : around)
    {
        total += JointWeight(binding, m_representative[id], joint);
    }

    return total / static_cast<float>(around.size());
}

void WeightPainter::Paint(RiggedCharacter& rig,
    const std::vector<BrushHit>& hits,
    int joint,
    BrushMode mode,
    float amount) const
{
    if (m_weld.size() != rig.positions.size())
    {
        return;
    }

    std::vector<float> targets;

    for (const BrushHit& hit : hits)
    {
        float current = JointWeight(rig.binding, hit.vertex, joint);
        float goal = mode == BrushMode::Add ? 1.0f
            : mode == BrushMode::Remove     ? 0.0f
                                            : NeighbourAverage(rig.binding, hit.vertex, joint);
        targets.push_back(current + (goal - current) * std::clamp(amount * hit.falloff, 0.0f, 1.0f));
    }

    for (size_t i = 0; i < hits.size(); ++i)
    {
        SetJointWeight(rig, hits[i].vertex, joint, targets[i]);
    }
}

}
