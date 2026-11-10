#include "ui/CharacterImportPanel.hh"

#include "app/Theme.hh"
#include "rig/RigMath.hh"
#include "ui/ImportPreview.hh"

#include <algorithm>
#include <cmath>

namespace tk
{

namespace
{

bool IsPaintable(const std::string& name)
{
    return name != "Motion_Root" && name != "Balance_Root" && name != "Character_Root" &&
        name.find("attach") == std::string::npos;
}

}

bool CharacterImportPanel::PaintMode() const
{
    return m_step == Step::Review && m_paint && m_session.Rig() && m_paintJoint >= 0;
}

void CharacterImportPanel::DrawPaintControls()
{
    const auto& rig = m_session.Rig();

    if (!rig)
    {
        return;
    }

    ImGui::SeparatorText("Paint weights");

    if (m_paintJoint < 0)
    {
        m_paintJoint = std::max(0, FindJoint(rig->skeleton, "Spine_2"));
    }

    if (ImGui::Checkbox("Paint", &m_paint))
    {
        m_sceneRevision = 0;
    }

    if (!m_paint)
    {
        return;
    }

    const std::string& current = rig->skeleton[static_cast<size_t>(m_paintJoint)].name;
    ImGui::SetNextItemWidth(-1);

    if (ImGui::BeginCombo("##joint", current.c_str()))
    {
        for (size_t i = 0; i < rig->skeleton.size(); ++i)
        {
            if (!IsPaintable(rig->skeleton[i].name))
            {
                continue;
            }

            if (ImGui::Selectable(rig->skeleton[i].name.c_str(), static_cast<int>(i) == m_paintJoint))
            {
                m_paintJoint = static_cast<int>(i);
                m_sceneRevision = 0;
            }
        }

        ImGui::EndCombo();
    }

    ImGui::RadioButton("Add", &m_brushMode, static_cast<int>(BrushMode::Add));
    ImGui::SameLine();
    ImGui::RadioButton("Remove", &m_brushMode, static_cast<int>(BrushMode::Remove));
    ImGui::SameLine();
    ImGui::RadioButton("Smooth", &m_brushMode, static_cast<int>(BrushMode::Smooth));
    ImGui::SetNextItemWidth(-60);
    ImGui::SliderFloat("Radius", &m_brushRadius, 5.0f, 150.0f, "%.0f px");
    ImGui::SetNextItemWidth(-60);
    ImGui::SliderFloat("Strength", &m_brushStrength, 0.05f, 1.0f, "%.2f");
    ImGui::TextDisabled(
        "Left: paint. Ctrl+click: pick the joint under the cursor.\nRight drag: orbit. Middle drag: pan.");
}

std::vector<BrushHit> CharacterImportPanel::CollectBrushHits(ImVec2 origin, ImVec2 size, float radius) const
{
    ImVec2 mouse = ImGui::GetIO().MousePos;
    Vec3 eye = m_orbit.Eye();
    struct Candidate
    {
        uint32_t vertex;
        float distance, depth;
    };
    std::vector<Candidate> candidates;
    float nearest = 1e30f;
    const std::vector<MeshVertex>& vertices = m_scene.parts.front().vertices;

    for (uint32_t v = 0; v < vertices.size(); ++v)
    {
        Vec3 p{vertices[v].x, vertices[v].y, vertices[v].z};
        float x, y;

        if (!m_orbit.Project(p, size.x, size.y, x, y))
        {
            continue;
        }

        float dx = origin.x + x - mouse.x, dy = origin.y + y - mouse.y;
        float distance = dx * dx + dy * dy;

        if (distance > radius * radius || Dot({vertices[v].nx, vertices[v].ny, vertices[v].nz}, eye - p) <= 0.0f)
        {
            continue;
        }

        float depth = Length(eye - p);
        nearest = std::min(nearest, depth);
        candidates.push_back({v, std::sqrt(distance), depth});
    }

    std::vector<BrushHit> hits;
    float slab = m_session.Height() * 0.06f;

    for (const Candidate& c : candidates)
    {
        if (c.depth > nearest + slab)
        {
            continue;
        }

        float t = c.distance / radius;
        hits.push_back({c.vertex, 1.0f - t * t});
    }

    return hits;
}

void CharacterImportPanel::HandlePaintInput(ImVec2 origin, ImVec2 size)
{
    RiggedCharacter* rig = m_session.MutableRig();

    if (!rig || m_scene.parts.empty() || !ImGui::IsItemActive() || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        return;
    }

    if (m_painterRevision != m_session.Revision())
    {
        m_painter.Prepare(*rig);
        m_painterRevision = m_session.Revision();
    }

    if (ImGui::GetIO().KeyCtrl)
    {
        if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            return;
        }

        std::vector<BrushHit> hits = CollectBrushHits(origin, size, 10.0f);

        if (hits.empty())
        {
            return;
        }

        auto best = std::max_element(hits.begin(),
            hits.end(),
            [](const BrushHit& a, const BrushHit& b)
            {
                return a.falloff < b.falloff;
            });
        m_paintJoint = rig->binding.joints[best->vertex][0];
        m_sceneRevision = 0;
        return;
    }

    std::vector<BrushHit> hits = CollectBrushHits(origin, size, m_brushRadius);

    if (hits.empty())
    {
        return;
    }

    float amount = m_brushStrength * ImGui::GetIO().DeltaTime * 6.0f;
    m_painter.Paint(*rig, hits, m_paintJoint, static_cast<BrushMode>(m_brushMode), amount);
    RefreshPaintColours();
}

void CharacterImportPanel::RefreshPaintColours()
{
    const auto& rig = m_session.Rig();

    if (!rig || m_scene.parts.empty())
    {
        return;
    }

    std::vector<uint32_t> colours = WeightColours(rig->binding, m_paintJoint);
    std::vector<MeshVertex>& vertices = m_scene.parts.front().vertices;

    for (size_t v = 0; v < vertices.size() && v < colours.size(); ++v)
    {
        vertices[v].colour = colours[v];
    }

    m_renderer.UpdateVertices(0, vertices);
}

void CharacterImportPanel::DrawBrush(ImVec2 origin, ImVec2 size)
{
    if (!ImGui::IsItemHovered() && !ImGui::IsItemActive())
    {
        return;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);
    ImVec2 mouse = ImGui::GetIO().MousePos;
    draw->AddCircle(mouse, m_brushRadius, IM_COL32(0, 0, 0, 160), 48, 3.0f);
    draw->AddCircle(mouse, m_brushRadius, theme::kGold, 48, 1.5f);
    const char* name = m_session.Rig()->skeleton[static_cast<size_t>(m_paintJoint)].name.c_str();
    draw->AddText(ImVec2(mouse.x + m_brushRadius + 6.0f, mouse.y - 8.0f), theme::kBone, name);
    draw->PopClipRect();
}

}
