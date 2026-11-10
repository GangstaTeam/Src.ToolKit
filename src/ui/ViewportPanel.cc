#include "ui/ViewportPanel.hh"

#include "render/MeshExtractor.hh"
#include "render/SkeletonRenderer.hh"

namespace tk
{

void ViewportPanel::Refresh()
{
    uint64_t revision = m_document.Revision();

    if (revision != m_textureRevision)
    {
        m_textures.Rebuild(m_device, *m_document.Root());
        m_textureRevision = revision;
    }

    const Chunk* selection = m_document.Selection();

    if (selection == m_lastSelection && revision == m_meshRevision && m_wholeFile == m_lastWholeFile)
    {
        return;
    }

    ModelSelection model =
        m_wholeFile ? ResolveWholeFile(*m_document.Root()) : ResolveModel(*m_document.Root(), selection);
    bool geometryChanged = revision != m_meshRevision || !(model == m_model);
    bool reframe = model.meshes != m_model.meshes;
    m_lastSelection = selection;
    m_meshRevision = revision;
    m_lastWholeFile = m_wholeFile;
    m_joints = model.skeleton ? ExtractSkeleton(*model.skeleton) : std::vector<JointPose>();
    m_model = std::move(model);

    if (!geometryChanged)
    {
        return;
    }

    m_scene = ExtractMeshes(m_model.meshes);
    m_renderer.Upload(m_device, m_scene);

    if (reframe)
    {
        m_camera.Frame(m_scene.bounds);
    }
}

void ViewportPanel::Draw()
{
    if (m_focusFrames > 0)
    {
        ImGui::SetNextWindowFocus();
        --m_focusFrames;
    }

    if (ImGui::Begin("Viewport"))
    {
        Refresh();
        DrawToolbar();
        DrawView();
    }

    ImGui::End();
}

void ViewportPanel::DrawView()
{
    ImVec2 size = ImGui::GetContentRegionAvail();
    int width = static_cast<int>(size.x), height = static_cast<int>(size.y);

    if (width <= 8 || height <= 8)
    {
        return;
    }

    Render(width, height);
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("view",
        size,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    HandleInput();
    PickJoint(origin, size);

    if (m_target.Texture())
    {
        ImGui::GetWindowDrawList()->AddImage(m_target.Texture(), origin, ImVec2(origin.x + size.x, origin.y + size.y));
    }

    DrawSkeletonOverlay(origin, size);

    if (m_scene.parts.empty())
    {
        ImGui::SetCursorScreenPos(ImVec2(origin.x + 12, origin.y + 10));
        ImGui::TextDisabled("Select a model: CharacterLoader, Skin, Mesh, Skeleton or CompositeDrawable.");
    }
}

void ViewportPanel::DrawToolbar()
{
    ImGui::Checkbox("Whole file", &m_wholeFile);
    ImGui::SameLine();
    ImGui::Checkbox("Textures", &m_options.textured);
    ImGui::SameLine();
    ImGui::Checkbox("Lighting", &m_options.lighting);
    ImGui::SameLine();
    ImGui::Checkbox("Wireframe", &m_options.wireframe);
    ImGui::SameLine();
    ImGui::BeginDisabled(m_joints.empty());
    ImGui::Checkbox("Skeleton", &m_showSkeleton);
    ImGui::SameLine();
    ImGui::Checkbox("Joint names", &m_showNames);
    ImGui::EndDisabled();
    ImGui::SameLine();

    if (ImGui::Button("Frame"))
    {
        m_camera.Frame(m_scene.bounds);
    }

    ImGui::SameLine();
    ImGui::TextDisabled("%zu part(s), %zu triangles", m_scene.parts.size(), m_scene.triangles);
}

void ViewportPanel::HandleInput()
{
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_camera.Orbit(-io.MouseDelta.x, io.MouseDelta.y);
    }

    bool panning = ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle);

    if (ImGui::IsItemActive() && panning)
    {
        m_camera.Pan(io.MouseDelta.x, io.MouseDelta.y);
    }

    if (ImGui::IsItemHovered() && io.MouseWheel != 0.0f)
    {
        m_camera.Zoom(io.MouseWheel);
    }
}

void ViewportPanel::Render(int width, int height)
{
    if (!m_target.Ensure(m_device, width, height) || !m_target.Begin(m_device))
    {
        return;
    }

    D3DVIEWPORT9 viewport = {0, 0, static_cast<DWORD>(width), static_cast<DWORD>(height), 0.0f, 1.0f};
    m_device->SetViewport(&viewport);
    m_renderer.Draw(m_device,
        m_camera.View(),
        m_camera.Projection(static_cast<float>(width) / static_cast<float>(height)),
        m_textures,
        m_options);
    if (m_showSkeleton)
    {
        DrawSkeletonLines(m_device, m_joints, m_camera.Radius() * 0.05f);
    }

    m_target.End(m_device);
}

}
