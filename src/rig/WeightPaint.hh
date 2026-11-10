#pragma once

#include "rig/RigBuilder.hh"

#include <cstdint>
#include <vector>

namespace tk
{

enum class BrushMode
{
    Add,
    Remove,
    Smooth
};

struct BrushHit
{
    uint32_t vertex;
    float falloff;
};

float JointWeight(const SkinBinding& binding, size_t vertex, int joint);
void SetJointWeight(RiggedCharacter& rig, size_t vertex, int joint, float weight);

class WeightPainter
{
public:
    void Prepare(const RiggedCharacter& rig);
    void Paint(RiggedCharacter& rig, const std::vector<BrushHit>& hits, int joint, BrushMode mode, float amount) const;

private:
    float NeighbourAverage(const SkinBinding& binding, uint32_t vertex, int joint) const;

    std::vector<uint32_t> m_weld;
    std::vector<uint32_t> m_representative;
    std::vector<std::vector<uint32_t>> m_neighbours;
};

}
