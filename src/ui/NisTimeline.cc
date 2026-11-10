#include "ui/NisWorkspace.hh"

#include "ui/Icons.hh"

#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>

namespace tk
{

namespace
{

nis::Category CategoryOf(const fsc::Node& track)
{
    const nis::TrackSpec* spec = nis::FindTrack(track.key);
    return spec ? spec->category : nis::Category::Flow;
}

const char* LabelOf(const fsc::Node& track)
{
    const nis::TrackSpec* spec = nis::FindTrack(track.key);
    return spec ? spec->label : track.key.c_str();
}

int LabelStep(float zoom)
{
    for (int seconds : {1, 2, 5, 10, 20, 30, 60})
    {
        if (seconds * nis::kFramesPerSecond * zoom >= 56.0f)
        {
            return seconds;
        }
    }

    return 120;
}

void DrawDiamond(ImDrawList* list, ImVec2 centre, float radius, ImU32 colour)
{
    list->AddQuadFilled(ImVec2(centre.x, centre.y - radius),
        ImVec2(centre.x + radius, centre.y),
        ImVec2(centre.x, centre.y + radius),
        ImVec2(centre.x - radius, centre.y),
        colour);
}

}

void NisWorkspace::DrawTimeline()
{
    bool visible = ImGui::Begin(TimelineWindow());
    m_timelineFocused = visible && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    if (!visible)
    {
        return ImGui::End();
    }

    if (!m_open)
    {
        DrawEmpty(TimelineWindow());
        return ImGui::End();
    }

    fsc::Node* node = SelectedNode();

    if (!node)
    {
        ImGui::TextDisabled("Select a node in the Scene panel.");
        return ImGui::End();
    }

    DrawTimelineToolbar();
    int frames = std::max<int>(m_sceneFrames, kMinimumFrames) + nis::kFramesPerSecond;
    DrawTimelineRows(frames);
    ImGui::End();
}

void NisWorkspace::DrawTimelineToolbar()
{
    if (ImGui::Button((std::string(icon::Add) + " Add track").c_str()))
    {
        ImGui::OpenPopup("add track");
    }

    DrawAddTrackMenu();
    ImGui::SameLine();
    bool track = HasTrack();
    ImGui::BeginDisabled(!track);

    if (ImGui::Button("Duplicate"))
    {
        DuplicateTrack();
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Ctrl+D");
    }

    ImGui::SameLine();

    if (ImGui::Button((std::string(icon::Delete) + " Delete").c_str()))
    {
        DeleteTrack();
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Del");
    }

    ImGui::SameLine();

    if (ImGui::ArrowButton("up", ImGuiDir_Up))
    {
        MoveTrack(-1);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Move up (Alt+Up). Tracks of a node run in this order.");
    }

    ImGui::SameLine();

    if (ImGui::ArrowButton("down", ImGuiDir_Down))
    {
        MoveTrack(1);
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Move down (Alt+Down)");
    }

    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
    ImGui::SliderFloat("##zoom", &m_zoom, 0.5f, 12.0f, "zoom %.1f px/frame", ImGuiSliderFlags_Logarithmic);
    ImGui::SameLine();
    const fsc::Node* node = SelectedNode();
    ImGui::TextDisabled("%s  |  %zu tracks  |  scene %d frames (%.1f s)",
        node->String("name").c_str(),
        nis::TrackIndices(*node).size(),
        m_sceneFrames,
        static_cast<float>(m_sceneFrames) / nis::kFramesPerSecond);
}

void NisWorkspace::DrawAddTrackMenu()
{
    if (!ImGui::BeginPopup("add track"))
    {
        return;
    }

    std::map<nis::Category, std::vector<const nis::TrackSpec*>> groups;

    for (const nis::TrackSpec& spec : nis::Tracks())
    {
        groups[spec.category].push_back(&spec);
    }

    for (const auto& [category, specs] : groups)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, NisCategoryColour(category));
        bool open = ImGui::BeginMenu(nis::CategoryName(category));
        ImGui::PopStyleColor();

        if (!open)
        {
            continue;
        }

        for (const nis::TrackSpec* spec : specs)
        {
            if (ImGui::MenuItem(spec->label, spec->type))
            {
                AddTrack(*spec);
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", spec->help);
            }
        }

        ImGui::EndMenu();
    }

    ImGui::EndPopup();
}

void NisWorkspace::DrawTimelineRows(int frames)
{
    fsc::Node& node = *SelectedNode();
    float laneWidth = frames * m_zoom;
    float rowHeight = ImGui::GetFrameHeight();
    ImGuiTableFlags flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable;
    if (!ImGui::BeginTable("timeline", 2, flags))
    {
        return;
    }

    ImGui::TableSetupScrollFreeze(1, 1);
    ImGui::TableSetupColumn("Track", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 300.0f);
    ImGui::TableSetupColumn("Frames", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, laneWidth);

    if (ImGui::GetCurrentTable()->Columns[1].WidthRequest != laneWidth)
    {
        ImGui::TableSetColumnWidth(1, laneWidth);
    }

    ImDrawList* list = ImGui::GetWindowDrawList();
    int labelStep = LabelStep(m_zoom) * nis::kFramesPerSecond;
    int tickStep = labelStep >= 60 ? nis::kFramesPerSecond : (m_zoom >= 4.0f ? 5 : 10);
    auto drawGrid = [&](ImVec2 origin, float height)
    {
        for (int frame = 0; frame <= frames; frame += labelStep)
        {
            list->AddLine(ImVec2(origin.x + frame * m_zoom, origin.y),
                ImVec2(origin.x + frame * m_zoom, origin.y + height),
                kGridColour);
        }

        float end = origin.x + m_sceneFrames * m_zoom;
        list->AddLine(ImVec2(end, origin.y), ImVec2(end, origin.y + height), kEndColour);

        if (m_hoverFrame >= 0)
        {
            float x = origin.x + m_hoverFrame * m_zoom;
            list->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + height), kCursorColour);
        }
    };

    ImGui::TableNextRow(ImGuiTableRowFlags_Headers, rowHeight);
    ImGui::TableSetColumnIndex(0);
    ImGui::TextDisabled("Track");
    ImGui::TableSetColumnIndex(1);
    ImVec2 ruler = ImGui::GetCursorScreenPos();

    for (int frame = 0; frame <= frames; frame += tickStep)
    {
        float x = ruler.x + frame * m_zoom;
        bool label = frame % labelStep == 0;
        list->AddLine(ImVec2(x, ruler.y + rowHeight * (label ? 0.35f : 0.65f)),
            ImVec2(x, ruler.y + rowHeight),
            kRulerText);
        if (label)
        {
            char text[16];
            std::snprintf(text, sizeof(text), "%ds", frame / nis::kFramesPerSecond);
            list->AddText(ImVec2(x + 3.0f, ruler.y), kRulerText, text);
        }
    }

    if (m_hoverFrame >= 0)
    {
        char text[32];
        std::snprintf(text,
            sizeof(text),
            "%d  (%.2fs)",
            m_hoverFrame,
            static_cast<float>(m_hoverFrame) / nis::kFramesPerSecond);
        ImVec2 at(ruler.x + m_hoverFrame * m_zoom + 4.0f, ruler.y + rowHeight * 0.45f);
        list->AddRectFilled(at,
            ImVec2(at.x + ImGui::CalcTextSize(text).x + 4.0f, at.y + ImGui::GetTextLineHeight()),
            IM_COL32(30, 30, 34, 230));
        list->AddText(ImVec2(at.x + 2.0f, at.y), IM_COL32(255, 255, 255, 255), text);
    }

    ImGui::Dummy(ImVec2(laneWidth, rowHeight));

    int hoverFrame = -1;
    std::function<void()> action;
    std::vector<size_t> tracks = nis::TrackIndices(node);

    for (size_t index : tracks)
    {
        fsc::Node& track = node.children[index];
        int trackIndex = static_cast<int>(index);
        bool selected = m_track == trackIndex;
        nis::Category category = CategoryOf(track);
        nis::Timing timing = nis::TrackTiming(track, m_assets);
        bool problem = false;

        for (const nis::Issue& issue : m_issues)
        {
            if (issue.node == m_node && issue.track == trackIndex && issue.severity != nis::Severity::Info)
            {
                problem = true;
            }
        }

        ImGui::PushID(trackIndex);
        ImGui::TableNextRow(0, rowHeight);

        ImGui::TableSetColumnIndex(0);

        if (ImGui::Selectable("##row", selected, ImGuiSelectableFlags_AllowOverlap, ImVec2(0, rowHeight)))
        {
            m_track = trackIndex;
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_track = trackIndex;
            ImGui::OpenPopup("track menu");
        }

        if (selected && m_revealTrack)
        {
            ImGui::SetScrollHereY();
            m_revealTrack = false;
        }

        ImVec2 min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
        float textY = min.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f;
        list->AddRectFilled(ImVec2(min.x + 2.0f, min.y + 4.0f),
            ImVec2(min.x + 6.0f, max.y - 4.0f),
            NisCategoryColour(category));
        const char* label = LabelOf(track);
        list->AddText(ImVec2(min.x + 12.0f, textY), ImGui::GetColorU32(ImGuiCol_Text), label);
        std::string subject = nis::TrackSubject(track);

        if (!subject.empty())
        {
            float x = min.x + 18.0f + ImGui::CalcTextSize(label).x;
            list->PushClipRect(ImVec2(x, min.y), ImVec2(max.x - (problem ? 16.0f : 0.0f), max.y), true);
            list->AddText(ImVec2(x, textY), kSubjectColour, subject.c_str());
            list->PopClipRect();
        }

        if (problem)
        {
            list->AddText(ImVec2(max.x - 12.0f, textY), kTimelineWarning, "!");
        }

        ImGui::TableSetColumnIndex(1);
        ImVec2 lane = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##lane", ImVec2(laneWidth, rowHeight));
        bool hovered = ImGui::IsItemHovered();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            m_track = trackIndex;
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_track = trackIndex;
            ImGui::OpenPopup("track menu");
        }

        if (ImGui::IsItemActivated() && timing.timed)
        {
            m_dragging = true;
            m_dragTrack = trackIndex;
            m_dragFrame = timing.begin;
            m_dragX = ImGui::GetIO().MousePos.x;
        }

        if (ImGui::IsItemActive() && m_dragging && m_dragTrack == trackIndex)
        {
            int frame = std::max(0,
                m_dragFrame + static_cast<int>(std::lround((ImGui::GetIO().MousePos.x - m_dragX) / m_zoom)));
            if (frame != timing.begin)
            {
                BeginChange();
                nis::SetTrackStart(track, frame);
                timing = nis::TrackTiming(track, m_assets);
            }

            hoverFrame = frame;
        }
        else if (m_dragTrack == trackIndex)
        {
            m_dragging = false;
            m_dragTrack = -1;
        }

        if (hovered && hoverFrame < 0)
        {
            hoverFrame = std::max(0, static_cast<int>((ImGui::GetIO().MousePos.x - lane.x) / m_zoom));
        }

        drawGrid(lane, rowHeight);
        ImU32 colour = NisCategoryColour(category);
        float x0 = lane.x + timing.begin * m_zoom;
        float x1 = x0 + timing.length * m_zoom;
        float top = lane.y + 3.0f, bottom = lane.y + rowHeight - 3.0f;

        if (timing.length > 0)
        {
            list->AddRectFilled(ImVec2(x0, top),
                ImVec2(x1, bottom),
                NisCategoryColour(category, timing.timed ? 150 : 70),
                3.0f);
            if (selected)
            {
                list->AddRect(ImVec2(x0, top), ImVec2(x1, bottom), kSelectedOutline, 3.0f, 0, 1.5f);
            }
        }

        float radius = rowHeight * 0.3f;

        if (timing.timed)
        {
            DrawDiamond(list,
                ImVec2(x0, lane.y + rowHeight * 0.5f),
                radius + (selected ? 1.5f : 0.0f),
                selected ? kSelectedOutline : colour);
        }
        else if (timing.length == 0)
        {
            list->AddCircleFilled(ImVec2(lane.x + radius + 2.0f, lane.y + rowHeight * 0.5f), radius * 0.7f, colour);
        }

        std::string summary = nis::TrackSummary(track);

        if (!summary.empty())
        {
            float textX = (timing.timed ? x0 : lane.x + radius * 2.0f) + radius + 4.0f;
            list->PushClipRect(ImVec2(textX, lane.y), ImVec2(lane.x + laneWidth, lane.y + rowHeight), true);
            list->AddText(ImVec2(textX, textY),
                timing.length > 0 ? IM_COL32(255, 255, 255, 230) : kSubjectColour,
                summary.c_str());
            list->PopClipRect();
        }

        if (hovered && !m_dragging)
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(label);

            if (!subject.empty())
            {
                ImGui::TextDisabled("%s", subject.c_str());
            }

            if (!summary.empty())
            {
                ImGui::TextUnformatted(summary.c_str());
            }

            if (timing.timed)
            {
                ImGui::Text("starts at frame %d (%.2f s)",
                    timing.begin,
                    static_cast<float>(timing.begin) / nis::kFramesPerSecond);
            }
            else
            {
                ImGui::TextDisabled("runs when the node starts");
            }

            if (timing.length > 0)
            {
                ImGui::Text("lasts %d frames (%.2f s)",
                    timing.length,
                    static_cast<float>(timing.length) / nis::kFramesPerSecond);
            }

            if (timing.timed)
            {
                ImGui::TextDisabled("Drag to move it in time.");
            }

            ImGui::EndTooltip();
        }

        if (ImGui::BeginPopup("track menu"))
        {
            if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
            {
                action = [this]
                {
                    DuplicateTrack();
                };
            }

            if (ImGui::MenuItem("Delete", "Del"))
            {
                action = [this]
                {
                    DeleteTrack();
                };
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Move up", "Alt+Up"))
            {
                action = [this]
                {
                    MoveTrack(-1);
                };
            }

            if (ImGui::MenuItem("Move down", "Alt+Down"))
            {
                action = [this]
                {
                    MoveTrack(1);
                };
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Copy as text"))
            {
                std::string text = fsc::Write(track);
                ImGui::SetClipboardText(text.c_str() + (text.rfind("\r\n", 0) == 0 ? 2 : 0));
            }

            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    if (tracks.empty())
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("This node has no tracks yet.");
    }

    ImGui::EndTable();
    m_hoverFrame = hoverFrame;

    if (action)
    {
        action();
    }
}

}
