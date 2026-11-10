#pragma once

#include "app/RecentFiles.hh"
#include "doc/Document.hh"
#include "doc/EditCommands.hh"
#include "ui/AddChunkPopup.hh"
#include "ui/ArchiveWorkspace.hh"
#include "ui/NisWorkspace.hh"
#include "ui/Workspace.hh"

#include <filesystem>
#include <functional>

namespace tk
{

struct FileActions
{
    std::function<void()> newFile;
    std::function<void()> newArchive;
    std::function<void()> newScene;
    std::function<void()> newFrontend;
    std::function<void()> open;
    std::function<void(const std::filesystem::path&)> openPath;
    std::function<void()> save;
    std::function<void()> saveAs;
    std::function<void()> exit;
    std::function<void()> characterImport;
    std::function<void()> gameLink;
};

class MainMenu
{
public:
    MainMenu(Document& document,
        EditCommands& commands,
        AddChunkPopup& addPopup,
        RecentFiles& recent,
        ArchiveWorkspace& archive,
        NisWorkspace& nis,
        const Workspace& workspace,
        FileActions actions);

    void Draw();
    void HandleShortcuts();
    void OpenAddChild();

private:
    void DrawFileMenu();
    void DrawRecentMenu();
    void DrawEditMenu();
    void DrawSceneEditMenu();
    void DrawChunkMenu();
    void DrawArchiveMenu();
    void DrawToolsMenu();

    Document& m_document;
    EditCommands& m_commands;
    AddChunkPopup& m_addPopup;
    RecentFiles& m_recent;
    ArchiveWorkspace& m_archive;
    NisWorkspace& m_nis;
    const Workspace& m_workspace;
    FileActions m_actions;
};

}
