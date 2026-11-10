#include "ui/CharacterImportPanel.hh"

#include "app/Theme.hh"
#include "render/SkeletonRenderer.hh"
#include "rig/TestPose.hh"
#include "ui/ImportPreview.hh"

namespace tk
{

namespace
{

enum class Icon
{
    Vertical,
    Horizontal,
    Turn
};

void DrawArrowHead(ImDrawList* draw, ImVec2 tip, ImVec2 direction, ImU32 colour)
{
    ImVec2 side(-direction.y * 5.0f, direction.x * 5.0f);
    ImVec2 back(tip.x - direction.x * 8.0f, tip.y - direction.y * 8.0f);
    draw->AddTriangleFilled(tip,
        ImVec2(back.x + side.x, back.y + side.y),
        ImVec2(back.x - side.x, back.y - side.y),
        colour);
}

void DrawIcon(ImDrawList* draw, Icon icon, ImVec2 centre, ImU32 colour)
{
    float r = 13.0f;

    if (icon == Icon::Vertical)
    {
        draw->AddLine(ImVec2(centre.x, centre.y - r + 6), ImVec2(centre.x, centre.y + r - 6), colour, 3.0f);
        DrawArrowHead(draw, ImVec2(centre.x, centre.y - r), ImVec2(0, -1), colour);
        DrawArrowHead(draw, ImVec2(centre.x, centre.y + r), ImVec2(0, 1), colour);
    }
    else if (icon == Icon::Horizontal)
    {
        draw->AddLine(ImVec2(centre.x - r + 6, centre.y), ImVec2(centre.x + r - 6, centre.y), colour, 3.0f);
        DrawArrowHead(draw, ImVec2(centre.x - r, centre.y), ImVec2(-1, 0), colour);
        DrawArrowHead(draw, ImVec2(centre.x + r, centre.y), ImVec2(1, 0), colour);
    }
    else
    {
        draw->PathArcTo(centre, r - 3.0f, -1.2f, 3.8f, 24);
        draw->PathStroke(colour, 0, 3.0f);
        DrawArrowHead(draw,
            ImVec2(centre.x + (r - 3.0f) * 0.36f, centre.y - (r - 3.0f) * 0.93f),
            ImVec2(1.0f, 0.35f),
            colour);
    }
}

}

bool CharacterImportPanel::FrontView() const
{
    return m_step == Step::Orient || m_step == Step::Skeleton;
}

bool CharacterImportPanel::MarkerView() const
{
    return m_step == Step::Skeleton && m_session.Request().source == RigSource::Markers;
}

bool CharacterImportPanel::ProjectPoint(Vec3 point, ImVec2 size, float& x, float& y) const
{
    if (!FrontView())
    {
        return m_orbit.Project(point, size.x, size.y, x, y);
    }

    m_front.Project(point, size.x, size.y, x, y);
    return true;
}

void CharacterImportPanel::Refresh()
{
    const auto& rig = m_session.Rig();

    if (m_session.TextureRevision() != m_textureRevision)
    {
        m_textureRevision = m_session.TextureRevision();
        m_textures.Clear();

        if (!m_session.Texture().bgra.empty())
        {
            m_textures.Assign(m_device, IMPORTPREVIEW_IMPORT_SHADER, m_session.Texture());
        }
    }

    if (m_session.Revision() != m_sceneRevision)
    {
        m_sceneRevision = m_session.Revision();
        const RigModel& model = m_session.Model();
        std::vector<uint32_t> colours = PaintMode() ? WeightColours(rig->binding, m_paintJoint)
            : rig && m_boneColours                  ? BoneColours(rig->binding)
                                                    : std::vector<uint32_t>();
        m_scene = rig ? BuildPreviewScene(rig->positions, rig->normals, rig->uvs, rig->indices, colours)
                      : BuildPreviewScene(model.positions, model.normals, model.uvs, model.indices, colours);
        m_renderer.Upload(m_device, m_scene);
        m_sceneIsPosed = false;
    }

    if (m_session.ModelRevision() != m_framedModel)
    {
        m_framedModel = m_session.ModelRevision();
        m_orbit.Frame(m_scene.bounds);
        m_front.Frame(m_scene.bounds);
    }

    bool modelSkeleton = m_step == Step::Skeleton && m_session.Request().source == RigSource::ModelSkeleton;
    m_joints = rig && m_step == Step::Review ? RigJoints(rig->skeleton)
        : modelSkeleton                      ? SourceJoints(m_session.Model())
                                             : std::vector<JointPose>();
    if (m_step == Step::Rigging)
    {
        m_orbit.Orbit(ImGui::GetIO().DeltaTime * 110.0f, 0.0f);
    }

    RefreshPose();
}

void CharacterImportPanel::RefreshPose()
{
    const auto& rig = m_session.Rig();

    if (!rig || m_step != Step::Review || !m_testPose || m_scene.parts.empty())
    {
        if (m_sceneIsPosed)
        {
            m_sceneRevision = 0;
        }

        return;
    }

    if (!PaintMode())
    {
        m_posePhase += ImGui::GetIO().DeltaTime * 2.5f;
    }

    PosedCharacter posed = PoseCharacter(*rig, m_posePhase);
    UpdatePreviewVertices(m_scene.parts[0].vertices, posed.positions, posed.normals);
    m_renderer.UpdateVertices(0, m_scene.parts[0].vertices);

    for (size_t i = 0; i < m_joints.size() && i < posed.joints.size(); ++i)
    {
        m_joints[i].position = posed.joints[i];
    }

    m_sceneIsPosed = true;
}

void CharacterImportPanel::DrawView()
{
    Refresh();
    ImVec2 size = ImGui::GetContentRegionAvail();
    int width = static_cast<int>(size.x), height = static_cast<int>(size.y);

    if (width <= 8 || height <= 8)
    {
        return;
    }

    RenderView(width, height);
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::SetNextItemAllowOverlap();
    ImGui::InvisibleButton("importView",
        size,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    if (MarkerView())
    {
        HandleMarkerInput(origin, size);
    }

    if (PaintMode())
    {
        HandlePaintInput(origin, size);
    }

    HandleCameraInput(size);

    if (m_target.Texture() && m_session.Loaded())
    {
        ImGui::GetWindowDrawList()->AddImage(m_target.Texture(), origin, ImVec2(origin.x + size.x, origin.y + size.y));
    }

    DrawJointNames(origin, size);

    if (m_step == Step::Upload)
    {
        DrawUploadHint(origin, size);
    }

    if (m_step == Step::Orient)
    {
        DrawOrientButtons(origin, size);
    }

    if (MarkerView())
    {
        DrawMarkers(origin, size);
        DrawMarkerPalette(origin, size);
        DrawMagnifier(origin, size);
    }

    if (PaintMode())
    {
        DrawBrush(origin, size);
    }
}

void CharacterImportPanel::RenderView(int width, int height)
{
    if (!m_target.Ensure(m_device, width, height) || !m_target.Begin(m_device))
    {
        return;
    }

    D3DVIEWPORT9 viewport = {0, 0, static_cast<DWORD>(width), static_cast<DWORD>(height), 0.0f, 1.0f};
    m_device->SetViewport(&viewport);
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    bool front = FrontView();
    m_options.textured = !m_boneColours && !PaintMode();
    m_renderer.Draw(m_device,
        front ? m_front.View() : m_orbit.View(),
        front ? m_front.Projection(aspect) : m_orbit.Projection(aspect),
        m_textures,
        m_options);
    if (!m_joints.empty())
    {
        DrawSkeletonLines(m_device, m_joints, m_session.Height() * 0.03f);
    }

    m_target.End(m_device);
}

void CharacterImportPanel::HandleCameraInput(ImVec2 size)
{
    ImGuiIO& io = ImGui::GetIO();
    bool active = ImGui::IsItemActive(), hovered = ImGui::IsItemHovered();
    bool panning = ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle);

    if (FrontView())
    {
        bool leftPan = ImGui::IsMouseDragging(ImGuiMouseButton_Left) && m_dragMarker < 0 && !MarkerView();

        if (active && (panning || leftPan))
        {
            m_front.Pan(io.MouseDelta.x, io.MouseDelta.y, size.y);
        }

        if (hovered && io.MouseWheel != 0.0f)
        {
            m_front.Zoom(io.MouseWheel);
        }

        return;
    }

    if (m_step == Step::Rigging)
    {
        return;
    }

    ImGuiMouseButton orbitButton = PaintMode() ? ImGuiMouseButton_Right : ImGuiMouseButton_Left;
    bool panButton = PaintMode() ? ImGui::IsMouseDragging(ImGuiMouseButton_Middle) : panning;

    if (active && ImGui::IsMouseDragging(orbitButton))
    {
        m_orbit.Orbit(-io.MouseDelta.x, io.MouseDelta.y);
    }

    if (active && panButton)
    {
        m_orbit.Pan(io.MouseDelta.x, io.MouseDelta.y);
    }

    if (hovered && io.MouseWheel != 0.0f)
    {
        m_orbit.Zoom(io.MouseWheel);
    }
}

void CharacterImportPanel::DrawOrientButtons(ImVec2 origin, ImVec2 size)
{
    const Icon icons[3] = {Icon::Vertical, Icon::Horizontal, Icon::Turn};
    const char* tips[3] = {"Rotate 90 degrees forward (X)",
        "Rotate 90 degrees around (Y)",
        "Rotate 90 degrees sideways (Z)"};
    const int axes[3] = {0, 1, 2};
    ImVec2 cursor = ImGui::GetCursorScreenPos();

    for (int i = 0; i < 3; ++i)
    {
        ImVec2 position(origin.x + 18.0f, origin.y + size.y - 18.0f - (3 - i) * (44.0f + 10.0f));
        ImGui::SetCursorScreenPos(position);
        ImGui::PushID(i);

        if (ImGui::Button("##rotate", ImVec2(44.0f, 44.0f)))
        {
            m_session.Rotate(axes[i], 1);
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_session.Rotate(axes[i], -1);
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s\nright click: the other way", tips[i]);
        }

        ImGui::PopID();
        DrawIcon(ImGui::GetWindowDrawList(),
            icons[i],
            ImVec2(position.x + 44.0f * 0.5f, position.y + 44.0f * 0.5f),
            theme::kBone);
    }

    ImGui::SetCursorScreenPos(cursor);
}

void CharacterImportPanel::DrawUploadHint(ImVec2 origin, ImVec2 size)
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 lo(origin.x + 30.0f, origin.y + 30.0f), hi(origin.x + size.x - 30.0f, origin.y + size.y - 30.0f);

    if (!m_session.Loaded())
    {
        draw->AddRect(lo, hi, IM_COL32(212, 175, 55, 140), 12.0f, 0, 2.0f);
    }

    const char* text = m_conversion.valid() ? "Importing FBX..."
        : m_session.Loaded()                ? ""
                                            : "Drop a .glb, .gltf or .fbx here";
    ImVec2 textSize = ImGui::CalcTextSize(text);
    draw->AddText(ImVec2(origin.x + (size.x - textSize.x) * 0.5f, origin.y + (size.y - textSize.y) * 0.5f),
        theme::kBone,
        text);
}

void CharacterImportPanel::DrawJointNames(ImVec2 origin, ImVec2 size)
{
    bool sourceBones = m_step == Step::Skeleton;

    if ((!m_showNames && !sourceBones) || m_joints.empty())
    {
        return;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);

    for (const JointPose& joint : m_joints)
    {
        float x, y;

        if (!ProjectPoint(joint.position, size, x, y))
        {
            continue;
        }

        ImVec2 point(origin.x + x, origin.y + y);
        draw->AddCircleFilled(point, 3.5f, theme::kRed);

        if (m_showNames)
        {
            draw->AddText(ImVec2(point.x + 7.0f, point.y - 9.0f), theme::kBone, joint.name.c_str());
        }
    }

    draw->PopClipRect();
}

}
