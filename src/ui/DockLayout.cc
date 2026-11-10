#include "ui/DockLayout.hh"

#include "ui/ArchiveWorkspace.hh"
#include "ui/FrontendWorkspace.hh"
#include "ui/GameWorkspace.hh"
#include "ui/NisWorkspace.hh"

#include <imgui_internal.h>

namespace tk
{

namespace
{

bool BeginLayout(ImGuiID dockspace)
{
    ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspace);

    if (node && (node->IsSplitNode() || node->Windows.Size > 0))
    {
        return false;
    }

    ImGui::DockBuilderRemoveNode(dockspace);
    ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace, ImGui::GetMainViewport()->WorkSize);
    return true;
}

}

bool ApplyPure3DLayoutIfEmpty(ImGuiID dockspace)
{
    if (!BeginLayout(dockspace))
    {
        return false;
    }

    ImGuiID centre = dockspace;
    ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, 0.22f, nullptr, &centre);
    ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right, 0.32f, nullptr, &centre);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Down, 0.28f, nullptr, &centre);
    ImGui::DockBuilderDockWindow("Chunks", left);
    ImGui::DockBuilderDockWindow("Properties", right);
    ImGui::DockBuilderDockWindow("Hex", bottom);
    ImGui::DockBuilderDockWindow("Texture", centre);
    ImGui::DockBuilderDockWindow("Character Import", centre);
    ImGui::DockBuilderDockWindow("Viewport", centre);
    ImGui::DockBuilderFinish(dockspace);
    return true;
}

bool ApplyArchiveLayoutIfEmpty(ImGuiID dockspace)
{
    if (!BeginLayout(dockspace))
    {
        return false;
    }

    ImGuiID centre = dockspace;
    ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, 0.20f, nullptr, &centre);
    ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right, 0.30f, nullptr, &centre);
    ImGui::DockBuilderDockWindow(ArchiveWorkspace::FoldersWindow(), left);
    ImGui::DockBuilderDockWindow(ArchiveWorkspace::DetailsWindow(), right);
    ImGui::DockBuilderDockWindow(ArchiveWorkspace::FilesWindow(), centre);
    ImGui::DockBuilderFinish(dockspace);
    return true;
}

bool ApplyFrontendLayoutIfEmpty(ImGuiID dockspace)
{
    if (!BeginLayout(dockspace))
    {
        return false;
    }

    ImGuiID centre = dockspace;
    ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, 0.24f, nullptr, &centre);
    ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right, 0.28f, nullptr, &centre);
    ImGui::DockBuilderDockWindow(FrontendWorkspace::ProjectWindow(), left);
    ImGui::DockBuilderDockWindow(FrontendWorkspace::ElementWindow(), right);
    ImGui::DockBuilderDockWindow(FrontendWorkspace::ScreenWindow(), centre);
    ImGui::DockBuilderFinish(dockspace);
    return true;
}

bool ApplyGameLayoutIfEmpty(ImGuiID dockspace)
{
    if (!BeginLayout(dockspace))
    {
        return false;
    }

    ImGuiID centre = dockspace;
    ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, 0.26f, nullptr, &centre);
    ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right, 0.28f, nullptr, &centre);
    ImGui::DockBuilderDockWindow(GameWorkspace::TreeWindow(), left);
    ImGui::DockBuilderDockWindow(GameWorkspace::ElementWindow(), right);
    ImGui::DockBuilderDockWindow(GameWorkspace::ViewWindow(), centre);
    ImGui::DockBuilderFinish(dockspace);
    return true;
}

bool ApplyNisLayoutIfEmpty(ImGuiID dockspace)
{
    if (!BeginLayout(dockspace))
    {
        return false;
    }

    ImGuiID centre = dockspace;
    ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, 0.20f, nullptr, &centre);
    ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right, 0.30f, nullptr, &centre);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Down, 0.30f, nullptr, &centre);
    ImGui::DockBuilderDockWindow(NisWorkspace::SceneWindow(), left);
    ImGui::DockBuilderDockWindow(NisWorkspace::TrackWindow(), right);
    ImGui::DockBuilderDockWindow(NisWorkspace::ProblemsWindow(), bottom);
    ImGui::DockBuilderDockWindow(NisWorkspace::SourceWindow(), bottom);
    ImGui::DockBuilderDockWindow(NisWorkspace::TimelineWindow(), centre);
    ImGui::DockBuilderFinish(dockspace);
    return true;
}

}
