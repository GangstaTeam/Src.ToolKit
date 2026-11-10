#include "ui/ViewportPanel.hh"

#include "app/Theme.hh"

namespace tk
{

void ViewportPanel::DrawSkeletonOverlay(ImVec2 origin, ImVec2 size)
{
    if (m_joints.empty() || (!m_showSkeleton && !m_showNames))
    {
        return;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);

    for (const JointPose& joint : m_joints)
    {
        float x, y;

        if (!m_camera.Project(joint.position, size.x, size.y, x, y))
        {
            continue;
        }

        ImVec2 point(origin.x + x, origin.y + y);
        bool selected = joint.chunk == m_document.Selection();

        if (m_showSkeleton)
        {
            draw->AddCircleFilled(point, selected ? 6.0f : 3.5f, selected ? theme::kGold : theme::kRed);
        }

        if (!m_showNames && !selected)
        {
            continue;
        }

        ImVec2 text(point.x + 7.0f, point.y - 9.0f);
        draw->AddText(ImVec2(text.x + 1.0f, text.y + 1.0f), IM_COL32(0, 0, 0, 200), joint.name.c_str());
        draw->AddText(text, selected ? theme::kGold : theme::kBone, joint.name.c_str());
    }

    draw->PopClipRect();
}

void ViewportPanel::PickJoint(ImVec2 origin, ImVec2 size)
{
    if (m_joints.empty() || (!m_showSkeleton && !m_showNames) || !ImGui::IsItemDeactivated())
    {
        return;
    }

    if (ImGui::GetIO().MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] > 9.0f)
    {
        return;
    }

    ImVec2 mouse = ImGui::GetIO().MousePos;
    float best = 100.0f;
    Chunk* picked = nullptr;

    for (const JointPose& joint : m_joints)
    {
        float x, y;

        if (!m_camera.Project(joint.position, size.x, size.y, x, y))
        {
            continue;
        }

        float dx = origin.x + x - mouse.x, dy = origin.y + y - mouse.y;

        if (dx * dx + dy * dy < best)
        {
            best = dx * dx + dy * dy;
            picked = joint.chunk;
        }
    }

    if (picked)
    {
        m_document.Select(picked);
    }
}

}
