#include "ui/GameWorkspace.hh"

#include "fe/FeModel.hh"
#include "fe/FeTextBible.hh"
#include "p3d/PayloadCodec.hh"
#include "ui/Icons.hh"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <unordered_set>

namespace tk
{

namespace
{

using LiveField = link::Field;
using link::NodeRecord;
using link::NodeType;

const ImVec4 kConnected(0.45f, 0.85f, 0.45f, 1.0f);
const ImVec4 kWaiting(0.85f, 0.65f, 0.35f, 1.0f);

NodeType TypeOf(const NodeRecord& node)
{
    return static_cast<NodeType>(node.type);
}

bool IsParent(NodeType type)
{
    return type == NodeType::Screen || type == NodeType::Page || type == NodeType::Layer || type == NodeType::Group;
}

bool IsDrawable(NodeType type)
{
    return type >= NodeType::Sprite && type <= NodeType::RoundedRectangle;
}

const char* TypeName(NodeType type)
{
    switch (type)
    {
    case NodeType::Project:
        return "Project";
    case NodeType::Screen:
        return "Screen";
    case NodeType::Page:
        return "Page";
    case NodeType::Layer:
        return "Layer";
    case NodeType::Group:
        return "Group";
    case NodeType::Sprite:
        return "Sprite";
    case NodeType::Text:
        return "Text";
    case NodeType::Pure3dObject:
        return "Pure3D object";
    case NodeType::Polygon:
        return "Polygon";
    case NodeType::RoundedRectangle:
        return "Rounded rectangle";
    case NodeType::Unknown:
        break;
    }

    return "Unknown";
}

const char* IconFor(NodeType type)
{
    switch (type)
    {
    case NodeType::Project:
        return icon::Archive;
    case NodeType::Screen:
        return icon::Screen;
    case NodeType::Page:
        return icon::Page;
    case NodeType::Layer:
        return icon::Layers;
    case NodeType::Group:
        return icon::Group;
    case NodeType::Sprite:
        return icon::Image;
    case NodeType::Text:
        return icon::Text;
    case NodeType::Pure3dObject:
        return icon::Model;
    case NodeType::Polygon:
    case NodeType::RoundedRectangle:
        return icon::Shape;
    case NodeType::Unknown:
        break;
    }

    return icon::Document;
}

ImU32 TypeColour(NodeType type, int alpha)
{
    switch (type)
    {
    case NodeType::Sprite:
        return IM_COL32(90, 170, 255, alpha);
    case NodeType::Text:
        return IM_COL32(255, 230, 120, alpha);
    case NodeType::Pure3dObject:
        return IM_COL32(200, 120, 255, alpha);
    case NodeType::Polygon:
        return IM_COL32(120, 230, 160, alpha);
    case NodeType::RoundedRectangle:
        return IM_COL32(255, 140, 110, alpha);
    default:
        return IM_COL32(200, 200, 200, alpha);
    }
}

const char* JustificationName(int value)
{
    static const char* names[] = {"left", "right", "top", "bottom", "centre"};
    return value >= 0 && value < 5 ? names[value] : "?";
}

void Corners(const NodeRecord& node, float& x0, float& y0, float& x1, float& y1)
{
    x0 = static_cast<float>(std::min(node.bounds[0], node.bounds[2]));
    x1 = static_cast<float>(std::max(node.bounds[0], node.bounds[2]));
    y0 = static_cast<float>(std::min(node.bounds[1], node.bounds[3]));
    y1 = static_cast<float>(std::max(node.bounds[1], node.bounds[3]));
}

ImVec4 ToFloat4(uint32_t argb)
{
    return ImVec4(((argb >> 16) & 0xFF) / 255.0f,
        ((argb >> 8) & 0xFF) / 255.0f,
        (argb & 0xFF) / 255.0f,
        (argb >> 24) / 255.0f);
}

uint32_t ToArgb(const float c[4])
{
    auto byte = [](float v)
    {
        return static_cast<uint32_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
    };
    return (byte(c[3]) << 24) | (byte(c[0]) << 16) | (byte(c[1]) << 8) | byte(c[2]);
}

std::string BaseName(const std::string& name)
{
    size_t slash = name.find_last_of("\\/");
    return slash == std::string::npos ? name : name.substr(slash + 1);
}

fe::Node* FindByKey(fe::Node& node, uint32_t key)
{
    if (fe::MakeKey(BaseName(node.name)) == key)
    {
        return &node;
    }

    for (auto& child : node.children)
    {
        if (fe::Node* found = FindByKey(*child, key))
        {
            return found;
        }
    }

    return nullptr;
}

}

GameWorkspace::GameWorkspace(Document& document, const Schema& schema) : m_document(document), m_schema(schema)
{
}

ImVec2 GameWorkspace::View::ToScreen(float x, float y) const
{
    return ImVec2(origin.x + x * zoom, origin.y + (448.0f - y) * zoom);
}

ImVec2 GameWorkspace::View::ToPage(ImVec2 screen) const
{
    return ImVec2((screen.x - origin.x) / zoom, 448.0f - (screen.y - origin.y) / zoom);
}

void GameWorkspace::Draw()
{
    Poll();
    DrawTree();
    DrawView();
    DrawElement();
}

void GameWorkspace::Poll()
{
    if (m_link.Connection() != m_connection && m_link.Connected())
    {
        m_connection = m_link.Connection();
        m_link.SetHighlight(m_highlight);
        m_link.Select(m_selected);
        m_picking = false;
    }

    if (m_link.LatestSnapshot(m_frame, m_nodes))
    {
        Rebuild();
    }

    std::vector<link::PickRecord> picks = m_link.TakePicks();

    if (!picks.empty() && m_picking)
    {
        m_picking = false;
        m_link.SetPickMode(false);

        if (uint32_t id = Pick(picks.back().x, picks.back().y))
        {
            Select(id, true);
        }
    }
}

void GameWorkspace::Rebuild()
{
    m_index.clear();
    m_roots.clear();
    m_children.assign(m_nodes.size(), {});

    for (size_t i = 0; i < m_nodes.size(); ++i)
    {
        m_index[m_nodes[i].id] = i;
        auto parent = m_index.find(m_nodes[i].parent);

        if (m_nodes[i].parent && parent != m_index.end())
        {
            m_children[parent->second].push_back(i);
        }
        else
        {
            m_roots.push_back(i);
        }
    }

    if (m_hover && !m_index.count(m_hover))
    {
        m_hover = 0;
    }
}

const NodeRecord* GameWorkspace::Find(uint32_t id) const
{
    auto found = m_index.find(id);
    return found == m_index.end() ? nullptr : &m_nodes[found->second];
}

std::string GameWorkspace::NameOf(const NodeRecord& node) const
{
    std::string name = m_link.NameOf(node.key);

    if (!name.empty())
    {
        return name;
    }

    char fallback[32];
    std::snprintf(fallback, sizeof(fallback), "%s %08X", TypeName(TypeOf(node)), node.key);
    return fallback;
}

void GameWorkspace::Select(uint32_t id, bool reveal)
{
    m_selected = id;
    m_revealPending = reveal && id;
    m_link.Select(id);
    m_applyMessage.clear();
}

uint32_t GameWorkspace::Pick(float x, float y) const
{
    uint32_t best = 0;
    float bestArea = 0.0f;

    for (const NodeRecord& node : m_nodes)
    {
        if (!(node.flags & link::kNodeHasBounds))
        {
            continue;
        }

        if (!(node.flags & link::kNodeShown) && !m_showHidden)
        {
            continue;
        }

        float x0, y0, x1, y1;
        Corners(node, x0, y0, x1, y1);

        if (x < x0 || x > x1 || y < y0 || y > y1)
        {
            continue;
        }

        float area = (x1 - x0) * (y1 - y0);

        if (!best || area <= bestArea)
        {
            best = node.id, bestArea = area;
        }
    }

    return best;
}

bool GameWorkspace::MatchesFilter(size_t index) const
{
    if (!m_filter[0])
    {
        return true;
    }

    std::string name = NameOf(m_nodes[index]), filter = m_filter;
    auto lower = [](std::string s)
    {
        std::transform(s.begin(),
            s.end(),
            s.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
        return s;
    };

    if (lower(name).find(lower(filter)) != std::string::npos)
    {
        return true;
    }

    for (size_t child : m_children[index])
    {
        if (MatchesFilter(child))
        {
            return true;
        }
    }

    return false;
}

void GameWorkspace::DrawStatus()
{
    if (m_link.Connected())
    {
        ImGui::TextColored(kConnected, "%s  Connected to scarface.exe (pid %u)", icon::Screen, m_link.Pid());
        return;
    }

    ImGui::TextColored(kWaiting, "%s  Waiting for the game...", icon::Screen);
    ImGui::TextDisabled("Build gamelink/ (Win32) and copy ToolKitLink.asi");
    ImGui::TextDisabled("into the game's plugins folder, then start the game.");
    ImGui::TextDisabled("It connects on its own; US 1.00.2 only.");
}

void GameWorkspace::DrawTree()
{
    if (!ImGui::Begin(TreeWindow()))
    {
        return ImGui::End();
    }

    DrawStatus();
    ImGui::Separator();
    ImGui::BeginDisabled(!m_link.Connected());

    if (m_picking)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }

    if (ImGui::Button(m_picking ? "Click in the game..." : "Pick in game"))
    {
        m_picking = !m_picking;
        m_link.SetPickMode(m_picking);
    }

    if (m_picking)
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Then click an element in the game window to select it here.");
    }

    ImGui::SameLine();

    if (ImGui::Checkbox("Highlight", &m_highlight))
    {
        m_link.SetHighlight(m_highlight);
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("The selected element blinks in the game.");
    }

    ImGui::SameLine();
    ImGui::Checkbox("Only on screen", &m_onlyShown);
    ImGui::SameLine();

    if (ImGui::Button("Revert all"))
    {
        m_link.Revert(0);
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Undo every live edit in the game.");
    }

    ImGui::EndDisabled();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##filter", "filter by name", m_filter, sizeof(m_filter));
    ImGui::BeginChild("tree");

    if (m_link.Connected() && m_nodes.empty())
    {
        ImGui::TextDisabled("No frontend project is loaded yet.");
    }

    if (m_revealPending && !Find(m_selected))
    {
        m_revealPending = false;
    }

    for (size_t root : m_roots)
    {
        DrawTreeNode(root);
    }

    ImGui::EndChild();
    ImGui::End();
}

void GameWorkspace::DrawTreeNode(size_t index)
{
    const NodeRecord& node = m_nodes[index];
    NodeType type = TypeOf(node);
    bool shown = (node.flags & link::kNodeShown) != 0;

    if (m_onlyShown && !shown && node.id != m_selected)
    {
        return;
    }

    if (!MatchesFilter(index))
    {
        return;
    }

    ImGui::PushID(static_cast<int>(node.id));

    if (type != NodeType::Project)
    {
        bool visible = (node.flags & link::kNodeVisible) != 0;
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(visible ? ImGuiCol_Text : ImGuiCol_TextDisabled));

        if (ImGui::SmallButton(icon::Eye))
        {
            m_link.SetField(node.id, LiveField::Visible, static_cast<uint32_t>(!visible));
        }

        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(visible ? "Hide in the game" : "Show in the game");
        }

        ImGui::SameLine();
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (m_children[index].empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (node.id == m_selected)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (type == NodeType::Project || type == NodeType::Screen)
    {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (m_revealPending && node.id != m_selected)
    {
        for (const NodeRecord* at = Find(m_selected); at; at = Find(at->parent))
        {
            if (at->parent == node.id)
            {
                ImGui::SetNextItemOpen(true);
                break;
            }
        }
    }

    std::string label = std::string(IconFor(type)) + "  " + NameOf(node);

    if (!shown)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    }

    bool open = ImGui::TreeNodeEx("node", flags, "%s", label.c_str());

    if (!shown)
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        Select(node.id, false);
    }

    if (ImGui::IsItemHovered())
    {
        m_hover = node.id;
    }

    if (m_revealPending && node.id == m_selected)
    {
        ImGui::SetScrollHereY();
        m_revealPending = false;
    }

    if (node.flags & link::kNodeEdited)
    {
        ImGui::SameLine();
        ImGui::TextColored(kWaiting, "(edited)");
    }

    if (open)
    {
        for (size_t child : m_children[index])
        {
            DrawTreeNode(child);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void GameWorkspace::DrawView()
{
    if (!ImGui::Begin(ViewWindow()))
    {
        return ImGui::End();
    }

    ImGui::Checkbox("Fit", &m_fit);
    ImGui::SameLine();
    ImGui::BeginDisabled(m_fit);
    ImGui::SetNextItemWidth(140);
    ImGui::SliderFloat("##zoom", &m_zoom, 0.25f, 4.0f, "%.2fx");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::Checkbox("Show hidden", &m_showHidden);
    ImGui::SameLine();
    ImGui::TextDisabled("Click selects, drag moves (live in the game), arrows nudge.");
    DrawCanvas();
    ImGui::End();
}

void GameWorkspace::DrawCanvas()
{
    ImVec2 available = ImGui::GetContentRegionAvail();

    if (available.x < 32 || available.y < 32)
    {
        return;
    }

    float zoom = m_fit ? std::max(0.1f, std::min((available.x - 16) / 640.0f, (available.y - 16) / 448.0f)) : m_zoom;
    ImGui::BeginChild("canvas", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImVec2 size(640.0f * zoom, 448.0f * zoom);
    ImVec2 start = ImGui::GetCursorScreenPos();
    ImVec2 region = ImGui::GetContentRegionAvail();
    View view;
    view.zoom = zoom;
    view.origin = ImVec2(start.x + std::max(0.0f, (region.x - size.x) * 0.5f),
        start.y + std::max(0.0f, (region.y - size.y) * 0.5f));
    ImGui::InvisibleButton("page", ImVec2(std::max(size.x, region.x), std::max(size.y, region.y)));
    bool hovered = ImGui::IsItemHovered();
    ImDrawList* list = ImGui::GetWindowDrawList();
    ImVec2 pageMin = view.ToScreen(0, 448.0f), pageMax = view.ToScreen(640.0f, 0);
    list->AddRectFilled(pageMin, pageMax, kPageBackground);
    list->AddRect(pageMin, pageMax, kPageBorder);
    list->PushClipRect(pageMin, pageMax, true);

    std::unordered_set<uint32_t> selectedSet;

    for (const NodeRecord& node : m_nodes)
    {
        for (const NodeRecord* at = &node; at; at = Find(at->parent))
        {
            if (at->id == m_selected)
            {
                selectedSet.insert(node.id);
                break;
            }
        }
    }

    for (const NodeRecord& node : m_nodes)
    {
        if (!(node.flags & link::kNodeHasBounds))
        {
            continue;
        }

        bool shown = (node.flags & link::kNodeShown) != 0;

        if (!shown && !m_showHidden)
        {
            continue;
        }

        float x0, y0, x1, y1;
        Corners(node, x0, y0, x1, y1);
        ImVec2 a = view.ToScreen(x0, y1), b = view.ToScreen(x1, y0);
        int alpha = shown ? node.alpha : 40;
        list->AddRectFilled(a, b, TypeColour(TypeOf(node), std::max(12, alpha / 6)));
        list->AddRect(a, b, TypeColour(TypeOf(node), shown ? 170 : 60));
    }

    for (const NodeRecord& node : m_nodes)
    {
        if (!(node.flags & link::kNodeHasBounds) || !selectedSet.count(node.id))
        {
            continue;
        }

        float x0, y0, x1, y1;
        Corners(node, x0, y0, x1, y1);
        list->AddRect(view.ToScreen(x0, y1), view.ToScreen(x1, y0), kGameSelection, 0.0f, 0, 2.0f);
    }

    ImVec2 mouse = view.ToPage(ImGui::GetIO().MousePos);
    uint32_t under = hovered ? Pick(mouse.x, mouse.y) : 0;

    if (const NodeRecord* node = Find(under))
    {
        float x0, y0, x1, y1;
        Corners(*node, x0, y0, x1, y1);
        list->AddRect(view.ToScreen(x0, y1), view.ToScreen(x1, y0), kHoverColour);

        if (!m_dragging)
        {
            ImGui::SetTooltip("%s  %s\n%.0f, %.0f", IconFor(TypeOf(*node)), NameOf(*node).c_str(), mouse.x, mouse.y);
        }
    }

    list->PopClipRect();

    const NodeRecord* selected = Find(m_selected);
    bool movable = selected && IsDrawable(TypeOf(*selected)) && m_link.Connected();

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        if (under)
        {
            Select(under, true);
        }

        selected = Find(m_selected);
        movable = selected && under == m_selected && IsDrawable(TypeOf(*selected));

        if (movable)
        {
            m_dragging = true;
            m_dragStart = ImGui::GetIO().MousePos;
            m_dragX = m_sentX = selected->x;
            m_dragY = m_sentY = selected->y;
        }
    }

    if (m_dragging)
    {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || !selected)
        {
            m_dragging = false;
        }
        else
        {
            ImVec2 delta = ImGui::GetIO().MousePos;
            float dx = std::round((delta.x - m_dragStart.x) / zoom), dy = std::round((m_dragStart.y - delta.y) / zoom);

            if (m_dragX + dx != m_sentX)
            {
                m_link.SetField(selected->id, LiveField::X, m_sentX = m_dragX + dx);
            }

            if (m_dragY + dy != m_sentY)
            {
                m_link.SetField(selected->id, LiveField::Y, m_sentY = m_dragY + dy);
            }
        }
    }

    if (movable && !m_dragging && ImGui::IsWindowFocused())
    {
        float step = ImGui::GetIO().KeyShift ? 10.0f : 1.0f, dx = 0, dy = 0;

        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
        {
            dx -= step;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        {
            dx += step;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
        {
            dy += step;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        {
            dy -= step;
        }

        if (dx)
        {
            m_link.SetField(selected->id, LiveField::X, selected->x + dx);
        }

        if (dy)
        {
            m_link.SetField(selected->id, LiveField::Y, selected->y + dy);
        }
    }

    ImGui::EndChild();
}

void GameWorkspace::DrawElement()
{
    if (!ImGui::Begin(ElementWindow()))
    {
        return ImGui::End();
    }

    const NodeRecord* node = Find(m_selected);

    if (!node)
    {
        ImGui::TextDisabled(m_selected ? "The selected element is no longer on screen."
                                       : "Select an element in the tree, the view or the game.");
        return ImGui::End();
    }

    NodeType type = TypeOf(*node);
    ImGui::Text("%s  %s", IconFor(type), NameOf(*node).c_str());
    ImGui::TextDisabled("%s   key %08X   project %u   at %08X", TypeName(type), node->key, node->project, node->id);

    if (const NodeRecord* parent = Find(node->parent))
    {
        ImGui::TextDisabled("in");
        ImGui::SameLine();

        if (ImGui::SmallButton((std::string(IconFor(TypeOf(*parent))) + "  " + NameOf(*parent)).c_str()))
        {
            Select(parent->id, true);
        }
    }

    if (!(node->flags & link::kNodeShown) && (node->flags & link::kNodeVisible))
    {
        ImGui::TextColored(kWaiting, "Hidden by a parent.");
    }

    ImGui::Separator();
    ImGui::BeginDisabled(!m_link.Connected());
    DrawEdits(*node);
    ImGui::EndDisabled();
    ImGui::Separator();
    DrawApply(*node);
    ImGui::End();
}

void GameWorkspace::DrawEdits(const NodeRecord& node)
{
    NodeType type = TypeOf(node);

    if (type == NodeType::Project || type == NodeType::Unknown)
    {
        ImGui::TextDisabled("Nothing to edit on a %s.", TypeName(type));
        return;
    }

    bool visible = (node.flags & link::kNodeVisible) != 0;

    if (ImGui::Checkbox("Visible", &visible))
    {
        m_link.SetField(node.id, LiveField::Visible, static_cast<uint32_t>(visible));
    }

    int alpha = node.alpha;

    if (ImGui::SliderInt("Alpha", &alpha, 0, 255))
    {
        m_link.SetField(node.id, LiveField::Alpha, static_cast<uint32_t>(alpha));
    }

    if (IsDrawable(type))
    {
        float position[2] = {node.x, node.y};

        if (ImGui::DragFloat2("Position", position, 1.0f, -2000.0f, 2000.0f, "%.0f"))
        {
            if (position[0] != node.x)
            {
                m_link.SetField(node.id, LiveField::X, position[0]);
            }

            if (position[1] != node.y)
            {
                m_link.SetField(node.id, LiveField::Y, position[1]);
            }
        }

        if (type != NodeType::Polygon)
        {
            ImVec4 c = ToFloat4(node.colour);
            float colour[4] = {c.x, c.y, c.z, c.w};

            if (ImGui::ColorEdit4("Colour", colour, ImGuiColorEditFlags_AlphaBar))
            {
                m_link.SetField(node.id, LiveField::Colour, ToArgb(colour));
            }
        }
        else
        {
            ImGui::TextDisabled("Colour: per point (edit in the file)");
        }

        ImGui::TextDisabled("Size %d x %d   justification %s / %s",
            node.width,
            node.height,
            JustificationName(node.justification & 7),
            JustificationName((node.justification >> 3) & 7));
        if (node.offsetX != 0.0f || node.offsetY != 0.0f)
        {
            ImGui::TextDisabled("Animated offset %.1f, %.1f", node.offsetX, node.offsetY);
        }

        if (node.flags & link::kNodeHasBounds)
        {
            ImGui::TextDisabled("Bounds %d, %d  to  %d, %d",
                node.bounds[0],
                node.bounds[1],
                node.bounds[2],
                node.bounds[3]);
        }
    }

    if (node.flags & link::kNodeEdited)
    {
        if (ImGui::Button("Revert"))
        {
            m_link.Revert(node.id);
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Edits live in the game's memory only.");
    }
}

void GameWorkspace::DrawApply(const NodeRecord& node)
{
    NodeType type = TypeOf(node);

    if (type == NodeType::Project || type == NodeType::Screen || type == NodeType::Page || type == NodeType::Unknown)
    {
        return;
    }

    bool hasProject = fe::ContainsProject(*m_document.Root());
    ImGui::BeginDisabled(!hasProject);

    if (ImGui::Button("Copy to the open file"))
    {
        std::string message;
        ApplyToDocument(node, message);
        m_applyMessage = message;
    }

    ImGui::EndDisabled();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip(
            "Writes the live values into the matching element of the open frontend .p3d (one undo step).");
    }

    if (!hasProject)
    {
        ImGui::TextDisabled("Open the frontend .p3d of this page to keep the changes.");
    }

    if (!m_applyMessage.empty())
    {
        ImGui::TextWrapped("%s", m_applyMessage.c_str());
    }
}

bool GameWorkspace::ApplyToDocument(const NodeRecord& node, std::string& message)
{
    const NodeRecord* page = &node;

    while (page && TypeOf(*page) != NodeType::Page)
    {
        page = Find(page->parent);
    }

    if (!page)
    {
        message = "Could not find the page of this element.";
        return false;
    }

    fe::Project project = fe::Parse(*m_document.Root());
    fe::Node* target = nullptr;

    for (fe::Node* candidate : project.pages)
    {
        if (fe::MakeKey(BaseName(candidate->name)) != page->key)
        {
            continue;
        }

        target = FindByKey(*candidate, node.key);

        if (target)
        {
            break;
        }
    }

    if (!target || target->type == fe::Type::Page)
    {
        message = "The open file has no " + NameOf(node) + " in page " + NameOf(*page) + ".";
        return false;
    }

    const ChunkDef* def = m_schema.FindChunk(target->chunk->Id());

    if (!def)
    {
        message = "No schema for this chunk.";
        return false;
    }

    NodeType type = TypeOf(node);
    bool visible = (node.flags & link::kNodeVisible) != 0;
    std::string note;
    m_document.Edit(
        [&](Chunk&)
        {
            DecodedPayload decoded = DecodePayload(m_schema, *def, target->chunk->Payload());

            if (!decoded.ok)
            {
                return;
            }

            auto set = [&](const char* name, uint32_t value)
            {
                if (Field* field = FindField(decoded.fields, name))
                {
                    field->integer = value;
                }
            };

            if (type == NodeType::Layer)
            {
                set("Visible", visible ? 1u : 0u);
                set("Alpha", node.alpha);
            }
            else if (type == NodeType::Group)
            {
                set("Alpha", node.alpha);
            }
            else if (type == NodeType::Polygon)
            {
                if (Field* points = FindField(decoded.fields, "Points"))
                {
                    for (Field& point : points->items)
                    {
                        if (Field* x = FindField(point.items, "X"))
                        {
                            x->real += std::round(node.x);
                        }

                        if (Field* y = FindField(point.items, "Y"))
                        {
                            y->real += std::round(node.y);
                        }
                    }
                }

                if (Field* colours = FindField(decoded.fields, "Colors"))
                {
                    for (Field& colour : colours->items)
                    {
                        colour.integer = (colour.integer & 0x00FFFFFF) | (static_cast<uint32_t>(node.alpha) << 24);
                    }
                }

                if (node.x != 0.0f || node.y != 0.0f)
                {
                    note = " Revert it in the game before copying again, or the move adds up.";
                }
            }
            else
            {
                set("PositionX", static_cast<uint32_t>(static_cast<int32_t>(std::lround(node.x))));
                set("PositionY", static_cast<uint32_t>(static_cast<int32_t>(std::lround(node.y))));
                set("Color", (node.colour & 0x00FFFFFF) | (static_cast<uint32_t>(node.alpha) << 24));

                if (!visible)
                {
                    note = " Elements have no Visible field in the file; hide their layer or group instead.";
                }
            }

            target->chunk->SetPayload(EncodePayload(decoded));
        });
    message = "Copied into " + target->name + "." + note;
    return true;
}

}
