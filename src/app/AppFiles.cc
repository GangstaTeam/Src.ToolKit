#include "app/App.hh"

#include "app/FileDialogs.hh"

namespace tk
{

bool App::ConfirmDiscard()
{
    if (!m_document.Dirty())
    {
        return true;
    }

    int answer = MessageBoxW(m_window.Handle(),
        L"Save changes before continuing?",
        L"ToolKit",
        MB_YESNOCANCEL | MB_ICONQUESTION);
    if (answer == IDCANCEL)
    {
        return false;
    }

    if (answer == IDYES)
    {
        SaveDocument();
        return !m_document.Dirty();
    }

    return true;
}

void App::NewFile()
{
    m_workspace = Workspace::Pure3D;

    if (ConfirmDiscard())
    {
        m_document.New();
    }
}

void App::NewArchive()
{
    m_workspace = Workspace::Archive;
    m_archive->New();
}

void App::NewScene()
{
    m_workspace = Workspace::Nis;
    m_nis->New();
}

void App::NewFrontend()
{
    if (!ConfirmDiscard())
    {
        return;
    }

    m_workspace = Workspace::Frontend;
    m_frontend->RequestNewProject();
}

void App::OpenFile(const std::filesystem::path& path)
{
    if (NisWorkspace::Accepts(path))
    {
        m_workspace = Workspace::Nis;

        if (m_nis->Open(path))
        {
            m_recent.Add(path);
        }
        else if (!std::filesystem::exists(path))
        {
            m_recent.Remove(path);
        }

        return;
    }

    if (CharacterImportPanel::Accepts(path))
    {
        m_workspace = Workspace::Pure3D;
        return m_import->OpenModel(path);
    }

    if (ArchiveWorkspace::Accepts(path))
    {
        m_workspace = Workspace::Archive;
        m_archive->Open(path);

        if (m_archive->HasArchive())
        {
            m_recent.Add(path);
        }

        return;
    }

    if (!ConfirmDiscard())
    {
        return;
    }

    std::string error;

    if (m_document.Open(path, error))
    {
        m_recent.Add(path);
        ShowDocument();
        return;
    }

    if (!std::filesystem::exists(path))
    {
        m_recent.Remove(path);
    }

    ShowError(error);
}

void App::ShowDocument()
{
    m_workspace = fe::ContainsProject(*m_document.Root()) ? Workspace::Frontend : Workspace::Pure3D;
}

void App::OpenDropped(const std::vector<std::filesystem::path>& paths)
{
    std::vector<std::filesystem::path> add;

    for (const auto& path : paths)
    {
        if (ArchiveWorkspace::Accepts(path))
        {
            return OpenFile(path);
        }

        add.push_back(path);
    }

    if (m_workspace == Workspace::Archive)
    {
        return m_archive->AddDropped(add);
    }

    OpenFile(paths.front());
}

void App::OpenWithDialog()
{
    if (auto path = ShowOpenDialog(m_window.Handle()))
    {
        OpenFile(*path);
    }
}

void App::Save()
{
    if (m_workspace == Workspace::Archive)
    {
        return m_archive->Save();
    }

    if (m_workspace == Workspace::Nis)
    {
        return m_nis->Save();
    }

    SaveDocument();
}

void App::SaveAs()
{
    if (m_workspace == Workspace::Archive)
    {
        return m_archive->SaveAs();
    }

    if (m_workspace == Workspace::Nis)
    {
        return m_nis->SaveAs();
    }

    SaveDocumentAs();
}

void App::SaveDocument()
{
    if (!m_document.HasPath())
    {
        return SaveDocumentAs();
    }

    std::string error;

    if (!m_document.Save(error))
    {
        ShowError(error);
    }
}

void App::SaveDocumentAs()
{
    std::filesystem::path suggested = m_document.HasPath() ? m_document.Path() : std::filesystem::path("untitled.p3d");
    auto path = ShowSaveDialog(m_window.Handle(), suggested);

    if (!path)
    {
        return;
    }

    std::string error;

    if (m_document.SaveAs(*path, error))
    {
        m_recent.Add(*path);
    }
    else
    {
        ShowError(error);
    }
}

}
