#pragma once

#include "app/Dx9Device.hh"
#include "app/ImGuiLayer.hh"
#include "app/RecentFiles.hh"
#include "app/Window.hh"
#include "doc/Document.hh"
#include "doc/EditCommands.hh"
#include "p3d/Schema.hh"
#include "ui/AddChunkPopup.hh"
#include "ui/ArchiveWorkspace.hh"
#include "ui/CharacterImportPanel.hh"
#include "ui/FrontendWorkspace.hh"
#include "ui/GameWorkspace.hh"
#include "ui/ChunkTreePanel.hh"
#include "ui/HexPanel.hh"
#include "ui/MainMenu.hh"
#include "ui/NisWorkspace.hh"
#include "ui/PropertiesPanel.hh"
#include "ui/TexturePanel.hh"
#include "ui/ViewportPanel.hh"
#include "ui/Workspace.hh"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace tk
{

class App
{
public:
    App();
    int Run(const std::vector<std::filesystem::path>& startupFiles);

private:
    bool Init();
    void Shutdown();
    void Frame();
    void DrawWorkspaceBar();
    void DrawDockSpace();
    void DrawErrorPopup();
    void UpdateTitle();

    bool ConfirmDiscard();
    void NewFile();
    void NewArchive();
    void NewScene();
    void NewFrontend();
    void OpenFile(const std::filesystem::path& path);
    void ShowDocument();
    void OpenDropped(const std::vector<std::filesystem::path>& paths);
    void OpenWithDialog();
    void Save();
    void SaveAs();
    void SaveDocument();
    void SaveDocumentAs();
    void ShowError(const std::string& message);

    Window m_window;
    Dx9Device m_device;
    ImGuiLayer m_imgui;
    RecentFiles m_recent;
    Schema m_schema;
    Document m_document;
    EditCommands m_commands;
    AddChunkPopup m_addPopup;
    Workspace m_workspace = Workspace::Pure3D;
    std::unique_ptr<MainMenu> m_menu;
    std::unique_ptr<ChunkTreePanel> m_tree;
    std::unique_ptr<PropertiesPanel> m_properties;
    std::unique_ptr<HexPanel> m_hex;
    std::unique_ptr<TexturePanel> m_texture;
    std::unique_ptr<ViewportPanel> m_viewport;
    std::unique_ptr<CharacterImportPanel> m_import;
    std::unique_ptr<ArchiveWorkspace> m_archive;
    std::unique_ptr<FrontendWorkspace> m_frontend;
    std::unique_ptr<GameWorkspace> m_game;
    std::unique_ptr<NisWorkspace> m_nis;
    std::string m_error;
    std::wstring m_title;
    bool m_running = true;
};

}
