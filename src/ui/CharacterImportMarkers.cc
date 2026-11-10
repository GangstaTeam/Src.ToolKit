#include "ui/CharacterImportPanel.hh"

#include "app/Theme.hh"
#include "rig/MeshProbe.hh"

#include <algorithm>

namespace tk
{

namespace
{

struct MarkerStyle
{
    const char* row;
    int rowIndex;
    ImU32 colour;
};

MarkerStyle StyleOf(Marker marker)
{
    switch (marker)
    {
    case Marker::Chin:
        return {"CHIN", 0, IM_COL32(60, 210, 220, 255)};
    case Marker::ShoulderL:
    case Marker::ShoulderR:
        return {"SHOULDERS", 1, IM_COL32(175, 130, 245, 255)};
    case Marker::WristL:
    case Marker::WristR:
        return {"WRISTS", 2, IM_COL32(170, 220, 70, 255)};
    case Marker::ElbowL:
    case Marker::ElbowR:
        return {"ELBOWS", 3, IM_COL32(235, 210, 70, 255)};
    case Marker::KneeL:
    case Marker::KneeR:
        return {"KNEES", 4, IM_COL32(240, 150, 60, 255)};
    default:
        return {"GROIN", 5, IM_COL32(235, 110, 140, 255)};
    }
}

bool IsLeft(Marker marker)
{
    return marker == Marker::WristL || marker == Marker::ElbowL || marker == Marker::KneeL ||
        marker == Marker::ShoulderL;
}

void DrawRing(ImDrawList* draw, ImVec2 centre, ImU32 colour, bool active)
{
    draw->AddCircle(centre, 14.0f + 1.5f, IM_COL32(0, 0, 0, 160), 0, 5.0f);
    draw->AddCircle(centre, active ? 14.0f + 2.0f : 14.0f, colour, 0, 3.5f);
    draw->AddCircleFilled(centre, 2.5f, IM_COL32(255, 255, 255, 230));
}

float PaletteTop(ImVec2 origin, ImVec2 size)
{
    return origin.y + std::max(30.0f, size.y - kPaletteRows * 50.0f - 10.0f);
}

}

ImVec2 CharacterImportPanel::PaletteSlot(Marker marker, ImVec2 origin, ImVec2 size) const
{
    MarkerStyle style = StyleOf(marker);
    float x = origin.x + (IsLeft(marker) ? 190.0f : 140.0f);
    return ImVec2(x, PaletteTop(origin, size) + style.rowIndex * 50.0f);
}

void CharacterImportPanel::HandleMarkerInput(ImVec2 origin, ImVec2 size)
{
    ImVec2 mouse = ImGui::GetIO().MousePos;
    const MarkerSet& markers = m_session.Request().markers;

    if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        float best = 18.0f * 18.0f;

        for (size_t i = 0; i < kMarkerCount; ++i)
        {
            Marker marker = static_cast<Marker>(i);
            ImVec2 at = PaletteSlot(marker, origin, size);

            if (m_session.MarkerPlaced(marker))
            {
                float x, y;
                m_front.Project({markers[i].x, markers[i].y, 0.0f}, size.x, size.y, x, y);
                at = ImVec2(origin.x + x, origin.y + y);
            }

            float dx = at.x - mouse.x, dy = at.y - mouse.y;

            if (dx * dx + dy * dy < best)
            {
                best = dx * dx + dy * dy;
                m_dragMarker = static_cast<int>(i);
            }
        }
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        m_dragMarker = -1;
    }

    if (m_dragMarker < 0 || !ImGui::IsItemActive() || !ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f))
    {
        return;
    }

    float x, y;
    m_front.Unproject(mouse.x - origin.x, mouse.y - origin.y, size.x, size.y, x, y);
    Marker marker = static_cast<Marker>(m_dragMarker);

    if (!ImGui::GetIO().KeyCtrl && !IsOptional(marker))
    {
        bool knee = marker == Marker::KneeL || marker == Marker::KneeR;

        if (auto snapped = SnapToMesh(m_session.Model(), x, y, m_session.Height() * (knee ? 0.05f : 0.035f)))
        {
            x = snapped->x;

            if (MirrorOf(marker) != marker)
            {
                y = snapped->y;
            }
        }
    }

    m_session.MoveMarker(marker, x, y, m_symmetry);
}

void CharacterImportPanel::DrawMarkers(ImVec2 origin, ImVec2 size)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);

    if (m_symmetry)
    {
        float x, y;
        m_front.Project({0.0f, 0.0f, 0.0f}, size.x, size.y, x, y);
        draw->AddLine(ImVec2(origin.x + x, origin.y),
            ImVec2(origin.x + x, origin.y + size.y),
            IM_COL32(236, 228, 210, 150),
            1.5f);
    }

    const MarkerSet& markers = m_session.Request().markers;

    for (size_t i = 0; i < kMarkerCount; ++i)
    {
        Marker marker = static_cast<Marker>(i);

        if (!m_session.MarkerPlaced(marker) || m_dragMarker == static_cast<int>(i))
        {
            continue;
        }

        float x, y;
        m_front.Project({markers[i].x, markers[i].y, 0.0f}, size.x, size.y, x, y);
        DrawRing(draw, ImVec2(origin.x + x, origin.y + y), StyleOf(marker).colour, false);
    }

    draw->PopClipRect();
}

void CharacterImportPanel::DrawMarkerPalette(ImVec2 origin, ImVec2 size)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    float top = PaletteTop(origin, size);
    const char* rows[] = {"CHIN", "SHOULDERS", "WRISTS", "ELBOWS", "KNEES", "GROIN"};

    for (int row = 0; row < kPaletteRows; ++row)
    {
        ImVec2 text(origin.x + 22.0f, top + row * 50.0f - ImGui::GetTextLineHeight() * 0.5f);
        draw->AddText(ImVec2(text.x + 1, text.y + 1), IM_COL32(0, 0, 0, 200), rows[row]);
        draw->AddText(text, theme::kBone, rows[row]);
    }

    for (size_t i = 0; i < kMarkerCount; ++i)
    {
        Marker marker = static_cast<Marker>(i);
        ImVec2 slot = PaletteSlot(marker, origin, size);
        ImU32 colour = StyleOf(marker).colour;

        if (m_session.MarkerPlaced(marker) || m_dragMarker == static_cast<int>(i))
        {
            draw->AddCircle(slot, 14.0f, IM_COL32(236, 228, 210, 60), 0, 2.0f);
        }
        else
        {
            DrawRing(draw, slot, colour, false);
        }
    }

    if (m_dragMarker < 0)
    {
        return;
    }

    Marker dragged = static_cast<Marker>(m_dragMarker);
    ImVec2 at = ImGui::GetIO().MousePos;

    if (m_session.MarkerPlaced(dragged))
    {
        const MarkerPoint& point = m_session.Request().markers[static_cast<size_t>(dragged)];
        float x, y;
        m_front.Project({point.x, point.y, 0.0f}, size.x, size.y, x, y);
        at = ImVec2(origin.x + x, origin.y + y);
    }

    DrawRing(draw, at, StyleOf(dragged).colour, true);
}

void CharacterImportPanel::DrawMagnifier(ImVec2 origin, ImVec2 size)
{
    if (m_dragMarker < 0 || !m_target.Texture())
    {
        return;
    }

    Marker marker = static_cast<Marker>(m_dragMarker);
    ImVec2 focus = ImGui::GetIO().MousePos;

    if (m_session.MarkerPlaced(marker))
    {
        const MarkerPoint& point = m_session.Request().markers[static_cast<size_t>(marker)];
        float x, y;
        m_front.Project({point.x, point.y, 0.0f}, size.x, size.y, x, y);
        focus = ImVec2(origin.x + x, origin.y + y);
    }

    ImVec2 lo(origin.x + size.x - 150.0f - 12.0f, origin.y + 12.0f);
    ImVec2 hi(lo.x + 150.0f, lo.y + 150.0f);
    float half = 150.0f * 0.5f / 2.5f;
    ImVec2 uv0((focus.x - origin.x - half) / size.x, (focus.y - origin.y - half) / size.y);
    ImVec2 uv1((focus.x - origin.x + half) / size.x, (focus.y - origin.y + half) / size.y);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(lo, hi, IM_COL32(30, 22, 22, 255));
    draw->AddImage(m_target.Texture(), lo, hi, uv0, uv1);
    ImVec2 centre((lo.x + hi.x) * 0.5f, (lo.y + hi.y) * 0.5f);
    draw->AddLine(ImVec2(lo.x, centre.y), ImVec2(hi.x, centre.y), IM_COL32(236, 228, 210, 70));
    draw->AddLine(ImVec2(centre.x, lo.y), ImVec2(centre.x, hi.y), IM_COL32(236, 228, 210, 70));
    DrawRing(draw, centre, StyleOf(marker).colour, true);
    draw->AddRect(lo, hi, theme::kGold, 0.0f, 0, 2.0f);
}

}
