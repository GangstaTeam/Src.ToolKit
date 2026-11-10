#include "app/App.hh"

#include "p3d/SchemaParser.hh"
#include "ui/DockLayout.hh"
#include "ui/Icons.hh"

#include <imgui.h>
#include <imgui_internal.h>

namespace tk
{

namespace
{

std::filesystem::path ExecutableDirectory()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}

}

App::App() : m_commands(m_document, m_schema), m_addPopup(m_schema)
{
}

bool App::Init()
{
    if (!m_window.Create(L"ToolKit", 1600, 900) || !m_device.Create(m_window.Handle()))
    {
        return false;
    }

    if (!m_imgui.Init(m_window.Handle(), m_device.Get()))
    {
        return false;
    }

    m_recent.RegisterSettingsHandler();
    m_device.onDeviceLost = [this]
    {
        m_imgui.OnDeviceLost();

        if (m_viewport)
        {
            m_viewport->OnDeviceLost();
        }

        if (m_import)
        {
            m_import->OnDeviceLost();
        }
    };
    m_device.onDeviceReset = [this]
    {
        m_imgui.OnDeviceReset();
    };
    m_window.onResize = [this](int width, int height)
    {
        m_device.Resize(width, height);
    };
    m_window.onDropFiles = [this](const std::vector<std::filesystem::path>& paths)
    {
        OpenDropped(paths);
    };
    m_window.onCloseRequest = [this]
    {
        return ConfirmDiscard() && m_archive->ConfirmDiscard() && m_nis->ConfirmDiscard();
    };

    std::string error;

    if (!LoadSchemaDirectory(ExecutableDirectory() / "schema", m_schema, error))
    {
        ShowError("Schema: " + error);
    }

    ArchiveActions archiveActions;
    archiveActions.editFile = [this](const std::filesystem::path& path)
    {
        OpenFile(path);
    };
    archiveActions.flushEditor = [this](const std::filesystem::path& workDir)
    {
        auto inside = [&](const std::filesystem::path& path)
        {
            std::filesystem::path relative = path.lexically_relative(workDir);
            return !relative.empty() && *relative.begin() != L"..";
        };

        if (m_document.HasPath() && m_document.Dirty() && inside(m_document.Path()))
        {
            SaveDocument();
        }

        if (m_nis->Dirty() && !m_nis->Path().empty() && inside(m_nis->Path()))
        {
            m_nis->Save();
        }
    };
    archiveActions.error = [this](const std::string& message)
    {
        ShowError(message);
    };
    m_archive = std::make_unique<ArchiveWorkspace>(m_window.Handle(), m_schema, std::move(archiveActions));
    NisActions nisActions;
    nisActions.error = [this](const std::string& message)
    {
        ShowError(message);
    };
    nisActions.gameFolder = [this]
    {
        return m_frontend ? m_frontend->GameFolder() : std::filesystem::path();
    };
    nisActions.setGameFolder = [this](const std::filesystem::path& folder)
    {
        if (m_frontend)
        {
            m_frontend->SetGameFolder(folder);
        }
    };
    nisActions.link = [this]() -> GameLink*
    {
        return m_game ? &m_game->Link() : nullptr;
    };
    m_nis = std::make_unique<NisWorkspace>(m_window.Handle(), std::move(nisActions));

    FileActions actions;
    actions.newFile = [this]
    {
        NewFile();
    };
    actions.newArchive = [this]
    {
        NewArchive();
    };
    actions.newScene = [this]
    {
        NewScene();
    };
    actions.newFrontend = [this]
    {
        NewFrontend();
    };
    actions.open = [this]
    {
        OpenWithDialog();
    };
    actions.openPath = [this](const std::filesystem::path& path)
    {
        OpenFile(path);
    };
    actions.save = [this]
    {
        Save();
    };
    actions.saveAs = [this]
    {
        SaveAs();
    };
    actions.exit = [this]
    {
        PostMessageW(m_window.Handle(), WM_CLOSE, 0, 0);
    };
    actions.characterImport = [this]
    {
        m_workspace = Workspace::Pure3D;
        m_import->Open();
    };
    actions.gameLink = [this]
    {
        m_workspace = Workspace::Game;
    };
    m_menu = std::make_unique<MainMenu>(m_document,
        m_commands,
        m_addPopup,
        m_recent,
        *m_archive,
        *m_nis,
        m_workspace,
        actions);
    m_tree = std::make_unique<ChunkTreePanel>(m_document, m_commands, m_schema, m_addPopup);
    m_properties = std::make_unique<PropertiesPanel>(m_document, m_schema);
    m_hex = std::make_unique<HexPanel>(m_document);
    m_texture = std::make_unique<TexturePanel>(m_document, m_device.Get());
    m_viewport = std::make_unique<ViewportPanel>(m_document, m_device.Get());
    m_import = std::make_unique<CharacterImportPanel>(m_document, m_device.Get(), m_window.Handle());
    m_frontend = std::make_unique<FrontendWorkspace>(m_document, m_schema, m_device.Get(), m_window.Handle());
    m_frontend->RegisterSettingsHandler();
    m_game = std::make_unique<GameWorkspace>(m_document, m_schema);
    return true;
}

void App::Shutdown()
{
    m_nis.reset();
    m_game.reset();
    m_frontend.reset();
    m_archive.reset();
    m_import.reset();
    m_viewport.reset();
    m_texture.reset();
    m_imgui.Shutdown();
    m_device.Destroy();
    m_window.Destroy();
}

int App::Run(const std::vector<std::filesystem::path>& startupFiles)
{
    if (!Init())
    {
        return 1;
    }

    for (const std::filesystem::path& path : startupFiles)
    {
        OpenFile(path);
    }

    while (m_window.PumpMessages())
    {
        Frame();
    }

    Shutdown();
    return 0;
}

void App::Frame()
{
    UpdateTitle();
    m_imgui.BeginFrame();
    m_menu->Draw();
    DrawWorkspaceBar();
    DrawDockSpace();
    m_menu->HandleShortcuts();

    if (m_workspace == Workspace::Pure3D)
    {
        m_tree->Draw();
        m_viewport->Draw();
        m_properties->Draw();
        m_hex->Draw();
        m_texture->Draw();
        m_import->Draw();
        m_addPopup.Draw();
    }
    else if (m_workspace == Workspace::Frontend)
    {
        m_frontend->Draw();
    }
    else if (m_workspace == Workspace::Game)
    {
        m_game->Draw();
    }
    else if (m_workspace == Workspace::Nis)
    {
        m_nis->Draw();
    }
    else
    {
        m_archive->Draw();
    }

    DrawErrorPopup();

    if (m_device.BeginFrame())
    {
        m_imgui.EndFrame();
        m_device.EndFrame();
    }
    else
    {
        ImGui::EndFrame();
    }
}

void App::DrawWorkspaceBar()
{
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse;
    float height = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 3.0f));
    bool open = ImGui::BeginViewportSideBar("##workspaces", ImGui::GetMainViewport(), ImGuiDir_Up, height, flags);
    ImGui::PopStyleVar();

    if (open)
    {
        auto tab = [&](Workspace workspace, const char* glyph, const char* kind, const std::string& name)
        {
            bool active = m_workspace == workspace;
            std::string label = std::string(glyph) + "  " + kind + "   " + name;
            ImVec2 size(ImGui::CalcTextSize(label.c_str()).x + 24.0f, ImGui::GetFrameHeight());
            ImGui::PushStyleColor(ImGuiCol_Button,
                ImGui::GetStyleColorVec4(active ? ImGuiCol_TabActive : ImGuiCol_Tab));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_TabHovered));

            if (ImGui::Button(label.c_str(), size))
            {
                m_workspace = workspace;
            }

            ImGui::PopStyleColor(2);
        };
        std::string document =
            m_document.HasPath() ? m_document.Path().filename().u8string() : std::string("untitled.p3d");
        if (m_document.Dirty())
        {
            document += " *";
        }

        tab(Workspace::Pure3D, icon::Model, "Pure3D", document);
        ImGui::SameLine();

        if (fe::ContainsProject(*m_document.Root()) || m_workspace == Workspace::Frontend)
        {
            tab(Workspace::Frontend, icon::Screen, "Frontend", document);
            ImGui::SameLine();
        }

        tab(Workspace::Archive, icon::Archive, "Archive", m_archive->Title());
        ImGui::SameLine();

        if (m_nis->HasScene() || m_workspace == Workspace::Nis)
        {
            tab(Workspace::Nis, icon::Video, "NIS", m_nis->Title());
            ImGui::SameLine();
        }

        tab(Workspace::Game, icon::Screen, "Game", m_game->Connected() ? "live" : "not running");
    }

    ImGui::End();
}

void App::DrawDockSpace()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##workspace", nullptr, flags);
    ImGui::PopStyleVar(3);
    ImGuiID pure3d = ImGui::GetID("Pure3DDock");
    ImGuiID archive = ImGui::GetID("ArchiveDock");
    ImGuiID frontend = ImGui::GetID("FrontendDock");
    ImGuiID game = ImGui::GetID("GameDock");
    ImGuiID nis = ImGui::GetID("NisDock");
    ImGuiID active = m_workspace == Workspace::Pure3D ? pure3d
        : m_workspace == Workspace::Frontend          ? frontend
        : m_workspace == Workspace::Game              ? game
        : m_workspace == Workspace::Nis               ? nis
                                                      : archive;
    for (ImGuiID dock : {pure3d, frontend, archive, game, nis})
    {
        if (dock != active)
        {
            ImGui::DockSpace(dock, ImVec2(0, 0), ImGuiDockNodeFlags_KeepAliveOnly);
        }
    }

    ImGui::DockSpace(active, ImVec2(0, 0));
    ImGui::End();

    if (m_workspace == Workspace::Pure3D)
    {
        if (ApplyPure3DLayoutIfEmpty(pure3d))
        {
            m_viewport->RequestFocus();
        }
    }
    else if (m_workspace == Workspace::Frontend)
    {
        ApplyFrontendLayoutIfEmpty(frontend);
    }
    else if (m_workspace == Workspace::Game)
    {
        ApplyGameLayoutIfEmpty(game);
    }
    else if (m_workspace == Workspace::Nis)
    {
        ApplyNisLayoutIfEmpty(nis);
    }
    else
    {
        ApplyArchiveLayoutIfEmpty(archive);
    }
}

void App::DrawErrorPopup()
{
    if (!m_error.empty() && !ImGui::IsPopupOpen("Error"))
    {
        ImGui::OpenPopup("Error");
    }

    if (!ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    ImGui::TextUnformatted(m_error.c_str());

    if (ImGui::Button("OK", ImVec2(120, 0)))
    {
        m_error.clear();
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void App::UpdateTitle()
{
    std::wstring name;

    if (m_workspace == Workspace::Archive)
    {
        std::string title = m_archive->Title();
        name = std::filesystem::u8path(title).wstring();
    }
    else if (m_workspace == Workspace::Nis)
    {
        name = std::filesystem::u8path(m_nis->Title()).wstring();
    }
    else
    {
        name = (m_document.HasPath() ? m_document.Path().filename().wstring() : L"untitled.p3d") +
            (m_document.Dirty() ? L" *" : L"");
    }

    std::wstring title = name + L" - ToolKit";

    if (title == m_title)
    {
        return;
    }

    m_title = title;
    m_window.SetTitle(title);
}

void App::ShowError(const std::string& message)
{
    m_error = message;
}

}
