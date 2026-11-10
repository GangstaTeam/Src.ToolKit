#pragma once

#include "doc/Document.hh"
#include "link/GameLink.hh"
#include "p3d/Schema.hh"

#include <imgui.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace tk
{

enum EGameWorkspaceColour : uint32_t
{
    kGameSelection = 0xFF40C4FF,
    kHoverColour = 0xC8FFFFFF,
    kPageBackground = 0xFF1C1818,
    kPageBorder = 0x3CFFFFFF,
};

class GameWorkspace
{
public:
    GameWorkspace(Document& document, const Schema& schema);

    static const char* TreeWindow() { return "Live Tree"; }
    static const char* ViewWindow() { return "Live View"; }
    static const char* ElementWindow() { return "Live Element"; }

    void Draw();
    bool Connected() const { return m_link.Connected(); }
    GameLink& Link() { return m_link; }

private:
    struct View
    {
        ImVec2 origin;
        float zoom = 1.0f;
        ImVec2 ToScreen(float x, float y) const;
        ImVec2 ToPage(ImVec2 screen) const;
    };

    void Poll();
    void Rebuild();
    void Select(uint32_t id, bool reveal);
    uint32_t Pick(float x, float y) const;

    void DrawTree();
    void DrawTreeNode(size_t index);
    void DrawStatus();
    void DrawView();
    void DrawCanvas();
    void DrawElement();
    void DrawEdits(const link::NodeRecord& node);
    void DrawApply(const link::NodeRecord& node);

    const link::NodeRecord* Find(uint32_t id) const;
    std::string NameOf(const link::NodeRecord& node) const;
    bool MatchesFilter(size_t index) const;
    bool ApplyToDocument(const link::NodeRecord& node, std::string& message);

    Document& m_document;
    const Schema& m_schema;
    GameLink m_link;

    uint32_t m_frame = 0;
    uint32_t m_connection = 0;
    std::vector<link::NodeRecord> m_nodes;
    std::unordered_map<uint32_t, size_t> m_index;
    std::vector<std::vector<size_t>> m_children;
    std::vector<size_t> m_roots;

    uint32_t m_selected = 0;
    uint32_t m_hover = 0;
    bool m_revealPending = false;
    bool m_highlight = true;
    bool m_picking = false;
    bool m_onlyShown = true;
    char m_filter[64] = {};

    float m_zoom = 1.0f;
    bool m_fit = true;
    bool m_showHidden = false;
    bool m_dragging = false;
    ImVec2 m_dragStart;
    float m_dragX = 0.0f, m_dragY = 0.0f;
    float m_sentX = 0.0f, m_sentY = 0.0f;
    std::string m_applyMessage;
};

}
