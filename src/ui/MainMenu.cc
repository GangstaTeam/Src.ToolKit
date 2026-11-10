#include "ui/MainMenu.hh"

#include <imgui.h>

namespace tk
{

MainMenu::MainMenu(Document& document,
    EditCommands& commands,
    AddChunkPopup& addPopup,
    RecentFiles& recent,
    ArchiveWorkspace& archive,
    NisWorkspace& nis,
    const Workspace& workspace,
    FileActions actions)
    : m_document(document), m_commands(commands), m_addPopup(addPopup), m_recent(recent), m_archive(archive),
      m_nis(nis), m_workspace(workspace), m_actions(std::move(actions))
{
}

void MainMenu::Draw()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    DrawFileMenu();

    if (m_workspace == Workspace::Archive)
    {
        DrawArchiveMenu();
    }
    else if (m_workspace == Workspace::Nis)
    {
        DrawSceneEditMenu();
    }
    else
    {
        DrawEditMenu();

        if (m_workspace == Workspace::Pure3D)
        {
            DrawChunkMenu();
        }
    }

    DrawToolsMenu();
    ImGui::EndMainMenuBar();
}

void MainMenu::OpenAddChild()
{
    Chunk* parent = m_document.Selection() ? m_document.Selection() : m_document.Root();
    m_addPopup.Open("Add child chunk",
        parent->Id(),
        [this](uint32_t id)
        {
            m_commands.AddChild(id);
        });
}

void MainMenu::DrawFileMenu()
{
    if (!ImGui::BeginMenu("File"))
    {
        return;
    }

    if (ImGui::BeginMenu("New"))
    {
        bool pure3d = m_workspace == Workspace::Pure3D || m_workspace == Workspace::Game;

        if (ImGui::MenuItem("Pure3D file (.p3d)", pure3d ? "Ctrl+N" : nullptr))
        {
            m_actions.newFile();
        }

        if (ImGui::MenuItem("Frontend project (.p3d)", m_workspace == Workspace::Frontend ? "Ctrl+N" : nullptr))
        {
            m_actions.newFrontend();
        }

        if (ImGui::MenuItem("Cement archive (.rcf)", m_workspace == Workspace::Archive ? "Ctrl+N" : nullptr))
        {
            m_actions.newArchive();
        }

        if (ImGui::MenuItem("NIS scene (.fsc)", m_workspace == Workspace::Nis ? "Ctrl+N" : nullptr))
        {
            m_actions.newScene();
        }

        ImGui::EndMenu();
    }

    if (ImGui::MenuItem("Open...", "Ctrl+O"))
    {
        m_actions.open();
    }

    DrawRecentMenu();
    bool canSave = m_workspace == Workspace::Archive ? m_archive.HasArchive()
        : m_workspace == Workspace::Nis              ? m_nis.HasScene()
                                                     : true;
    if (ImGui::MenuItem("Save", "Ctrl+S", false, canSave))
    {
        m_actions.save();
    }

    if (ImGui::MenuItem("Save as...", "Ctrl+Shift+S", false, canSave))
    {
        m_actions.saveAs();
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Exit"))
    {
        m_actions.exit();
    }

    ImGui::EndMenu();
}

void MainMenu::DrawRecentMenu()
{
    if (!ImGui::BeginMenu("Open Recent", !m_recent.Items().empty()))
    {
        return;
    }

    std::filesystem::path picked;

    for (size_t i = 0; i < m_recent.Items().size(); ++i)
    {
        const std::filesystem::path& path = m_recent.Items()[i];
        ImGui::PushID(static_cast<int>(i));

        if (ImGui::MenuItem(path.filename().u8string().c_str()))
        {
            picked = path;
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", path.u8string().c_str());
        }

        ImGui::PopID();
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Clear recent files"))
    {
        m_recent.Clear();
    }

    ImGui::EndMenu();

    if (!picked.empty())
    {
        m_actions.openPath(picked);
    }
}

void MainMenu::DrawEditMenu()
{
    if (!ImGui::BeginMenu("Edit"))
    {
        return;
    }

    bool selected = m_commands.HasSelection() && m_document.Selection()->Parent();

    if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_document.CanUndo()))
    {
        m_document.Undo();
    }

    if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_document.CanRedo()))
    {
        m_document.Redo();
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Cut", "Ctrl+X", false, selected))
    {
        m_commands.Cut();
    }

    if (ImGui::MenuItem("Copy", "Ctrl+C", false, selected))
    {
        m_commands.Copy();
    }

    if (ImGui::MenuItem("Paste as child", "Ctrl+V", false, m_commands.CanPaste()))
    {
        m_commands.Paste();
    }

    if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, selected))
    {
        m_commands.Duplicate();
    }

    if (ImGui::MenuItem("Delete", "Del", false, selected))
    {
        m_commands.Delete();
    }

    ImGui::EndMenu();
}

void MainMenu::DrawSceneEditMenu()
{
    if (!ImGui::BeginMenu("Edit"))
    {
        return;
    }

    if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_nis.CanUndo()))
    {
        m_nis.Undo();
    }

    if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_nis.CanRedo()))
    {
        m_nis.Redo();
    }

    ImGui::Separator();
    bool track = m_nis.HasTrack();

    if (ImGui::MenuItem("Duplicate track", "Ctrl+D", false, track))
    {
        m_nis.DuplicateTrack();
    }

    if (ImGui::MenuItem("Delete track", "Del", false, track))
    {
        m_nis.DeleteTrack();
    }

    if (ImGui::MenuItem("Move track up", "Alt+Up", false, track))
    {
        m_nis.MoveTrack(-1);
    }

    if (ImGui::MenuItem("Move track down", "Alt+Down", false, track))
    {
        m_nis.MoveTrack(1);
    }

    ImGui::EndMenu();
}

void MainMenu::DrawChunkMenu()
{
    if (!ImGui::BeginMenu("Chunk"))
    {
        return;
    }

    bool selected = m_commands.HasSelection() && m_document.Selection()->Parent();

    if (ImGui::MenuItem("Add child...", "Ctrl+Enter"))
    {
        OpenAddChild();
    }

    if (ImGui::MenuItem("Move up", "Alt+Up", false, selected))
    {
        m_commands.MoveUp();
    }

    if (ImGui::MenuItem("Move down", "Alt+Down", false, selected))
    {
        m_commands.MoveDown();
    }

    ImGui::EndMenu();
}

void MainMenu::DrawArchiveMenu()
{
    if (!ImGui::BeginMenu("Archive"))
    {
        return;
    }

    bool open = m_archive.HasArchive();

    if (ImGui::MenuItem("Add files...", nullptr, false, open))
    {
        m_archive.AddFilesWithDialog();
    }

    if (ImGui::MenuItem("Add folder...", nullptr, false, open))
    {
        m_archive.AddFolderWithDialog();
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Extract selected...", nullptr, false, m_archive.HasSelection()))
    {
        m_archive.ExtractSelected();
    }

    if (ImGui::MenuItem("Extract all (unpack)...", nullptr, false, open))
    {
        m_archive.ExtractAll();
    }

    if (ImGui::MenuItem("Pack a folder into an archive..."))
    {
        m_archive.PackFolder();
    }

    ImGui::EndMenu();
}

void MainMenu::DrawToolsMenu()
{
    if (!ImGui::BeginMenu("Tools"))
    {
        return;
    }

    if (ImGui::MenuItem("Character import..."))
    {
        m_actions.characterImport();
    }

    if (ImGui::MenuItem("Live game UI"))
    {
        m_actions.gameLink();
    }

    ImGui::EndMenu();
}

void MainMenu::HandleShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput)
    {
        return;
    }

    bool ctrl = io.KeyCtrl;

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N))
    {
        if (m_workspace == Workspace::Archive)
        {
            m_actions.newArchive();
        }
        else if (m_workspace == Workspace::Nis)
        {
            m_actions.newScene();
        }
        else if (m_workspace == Workspace::Frontend)
        {
            m_actions.newFrontend();
        }
        else
        {
            m_actions.newFile();
        }
    }

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O))
    {
        m_actions.open();
    }

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S))
    {
        io.KeyShift ? m_actions.saveAs() : m_actions.save();
    }

    if (m_workspace == Workspace::Archive)
    {
        return;
    }

    if (m_workspace == Workspace::Nis)
    {
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z))
        {
            m_nis.Undo();
        }

        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y))
        {
            m_nis.Redo();
        }

        return;
    }

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z))
    {
        m_document.Undo();
    }

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y))
    {
        m_document.Redo();
    }

    if (m_workspace == Workspace::Frontend)
    {
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_D))
        {
            m_commands.Duplicate();
        }

        return;
    }

    if (m_workspace != Workspace::Pure3D)
    {
        return;
    }

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C))
    {
        m_commands.Copy();
    }

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_X))
    {
        m_commands.Cut();
    }

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V))
    {
        m_commands.Paste();
    }

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_D))
    {
        m_commands.Duplicate();
    }

    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Enter))
    {
        OpenAddChild();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        m_commands.Delete();
    }

    if (io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
    {
        m_commands.MoveUp();
    }

    if (io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
    {
        m_commands.MoveDown();
    }
}

}
