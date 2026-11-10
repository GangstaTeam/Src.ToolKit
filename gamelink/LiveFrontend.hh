#pragma once

#include "link/LinkProtocol.hh"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace pure3d::frontend
{
class Child;
}

enum ELiveFrontend : int
{
    kMaxDepth = 32,
};

enum ELiveFrontendByte : uint8_t
{
    kPulseLow = 40,
};

enum ELiveFrontendSize : size_t
{
    kMaxNodes = 20000,
};

enum ELiveFrontendValue : uint32_t
{
    kMaxChildren = 4096,
};

class LiveFrontend
{
public:
    bool Walk();
    const std::vector<tk::link::NodeRecord>& Nodes() const { return m_nodes; }

    void Select(uint32_t id) { m_selected = id; }
    void SetHighlight(bool on) { m_highlight = on; }
    void SetField(uint32_t id, tk::link::Field field, uint32_t value);
    void Revert(uint32_t id);
    void ClearHighlight();
    void UpdateHighlight(double seconds);

private:
    struct Original
    {
        uint32_t key = 0;
        uint8_t mask = 0;
        uint8_t visible = 0;
        uint8_t alpha = 0;
        float x = 0, y = 0;
        uint32_t colour = 0;
    };
    struct Pulse
    {
        uint32_t id = 0;
        uint32_t key = 0;
        uint8_t alpha = 0;
    };

    void WalkProjects();
    void WalkGuarded(pure3d::frontend::Child* child, uint32_t parent, uint8_t project, bool shown, int depth);
    void WalkObject(pure3d::frontend::Child* child, uint32_t parent, uint8_t project, bool shown, int depth);
    const tk::link::NodeRecord* Find(uint32_t id) const;
    void Write(const tk::link::NodeRecord& node, tk::link::Field field, uint32_t value);
    void Remember(const tk::link::NodeRecord& node, tk::link::Field field);

    std::vector<tk::link::NodeRecord> m_nodes;
    std::unordered_map<uint32_t, size_t> m_index;
    std::unordered_map<uint32_t, Original> m_originals;
    uint32_t m_selected = 0;
    bool m_highlight = true;
    Pulse m_pulse;
};
