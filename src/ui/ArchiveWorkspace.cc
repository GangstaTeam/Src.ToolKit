#include "ui/ArchiveWorkspace.hh"

#include "app/FileDialogs.hh"
#include "p3d/ChunkIo.hh"
#include "rcf/CementFolder.hh"
#include "rcf/FileTime.hh"
#include "rcf/RawFile.hh"
#include "ui/Icons.hh"
#include "ui/Widgets.hh"

#include <imgui.h>
#include <shellapi.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>

namespace tk
{

namespace
{

const ImVec4 kNewColour(1.0f, 0.75f, 0.3f, 1.0f);
const ImVec4 kFolderColour(0.95f, 0.78f, 0.45f, 1.0f);

std::string Lower(std::string text)
{
    for (char& c : text)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return text;
}

std::string Extension(const std::string& name)
{
    size_t dot = name.find_last_of('.');
    size_t slash = name.find_last_of('\\');

    if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
    {
        return {};
    }

    return Lower(name.substr(dot));
}

std::string ParentOf(const std::string& name)
{
    size_t slash = name.find_last_of('\\');
    return slash == std::string::npos ? std::string() : name.substr(0, slash);
}

std::string LeafOf(const std::string& name)
{
    size_t slash = name.find_last_of('\\');
    return slash == std::string::npos ? name : name.substr(slash + 1);
}

std::string Join(const std::string& folder, const std::string& name)
{
    return folder.empty() ? name : folder + "\\" + name;
}

bool IsUnder(const std::string& name, const std::string& folder)
{
    if (folder.empty())
    {
        return true;
    }

    return name.size() > folder.size() && name[folder.size()] == '\\' &&
        Lower(name.substr(0, folder.size())) == Lower(folder);
}

std::string FormatSize(uint64_t size)
{
    char text[32];

    if (size < 1024)
    {
        std::snprintf(text, sizeof(text), "%llu B", static_cast<unsigned long long>(size));
    }
    else if (size < 1024 * 1024)
    {
        std::snprintf(text, sizeof(text), "%.1f KB", size / 1024.0);
    }
    else if (size < 1024ull * 1024 * 1024)
    {
        std::snprintf(text, sizeof(text), "%.1f MB", size / (1024.0 * 1024.0));
    }
    else
    {
        std::snprintf(text, sizeof(text), "%.2f GB", size / (1024.0 * 1024.0 * 1024.0));
    }

    return text;
}

std::string FormatTime(uint32_t unixTime)
{
    std::time_t time = unixTime;
    char text[32] = "?";

    if (std::tm* local = std::localtime(&time))
    {
        std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M", local);
    }

    return text;
}

struct FileKind
{
    const char* icon;
    const char* label;
    bool text;
};

FileKind KindOf(const std::string& name)
{
    std::string ext = Extension(name);

    if (ext == ".p3d")
    {
        return {icon::Model, "Pure3D", false};
    }

    if (ext == ".cso")
    {
        return {icon::Script, "Compiled script", false};
    }

    if (ext == ".cs")
    {
        return {icon::Script, "Script", true};
    }

    if (ext == ".rcf")
    {
        return {icon::Archive, "Cement archive", false};
    }

    if (ext == ".bik" || ext == ".bk2")
    {
        return {icon::Video, "Bink video", false};
    }

    if (ext == ".rsd" || ext == ".wav" || ext == ".ogg" || ext == ".mp3" || ext == ".rsr" || ext == ".snd" ||
        ext == ".spt")
    {
        return {icon::Audio, "Audio", false};
    }

    if (ext == ".png" || ext == ".bmp" || ext == ".tga" || ext == ".dds" || ext == ".jpg")
    {
        return {icon::Image, "Image", false};
    }

    if (ext == ".txt" || ext == ".ini" || ext == ".cfg" || ext == ".xml" || ext == ".lst" || ext == ".csv" ||
        ext == ".log" || ext == ".json" || ext == ".h")
    {
        return {icon::Text, "Text", true};
    }

    return {icon::Document, "File", false};
}

bool LooksLikeText(const Bytes& bytes)
{
    if (bytes.empty())
    {
        return false;
    }

    size_t sample = std::min<size_t>(bytes.size(), 4096), printable = 0;

    for (size_t i = 0; i < sample; ++i)
    {
        uint8_t c = bytes[i];

        if (c == '\n' || c == '\r' || c == '\t' || (c >= 0x20 && c < 0x7F) || c >= 0x80)
        {
            ++printable;
        }
        else if (c == 0)
        {
            return false;
        }
    }

    return printable * 100 >= sample * 97;
}

bool ReadSourceBytes(const CementSource& source, size_t limit, Bytes& out, std::string& error)
{
    size_t count = std::min<size_t>(source.size, limit);
    out.resize(count);

    if (source.kind == CementSource::Kind::Memory)
    {
        std::memcpy(out.data(), source.bytes->data(), count);
        return true;
    }

    RawFile file(source.path, RawFile::Mode::Read);
    bool ok = file.IsOpen() && (source.kind != CementSource::Kind::ArchiveSlice || file.Seek(source.offset)) &&
        file.Read(out.data(), count);
    if (!ok)
    {
        error = "Cannot read " + source.name;
    }

    return ok;
}

class ButtonRow
{
public:
    bool Button(const char* glyph, const char* text)
    {
        std::string label = std::string(glyph) + "  " + text;
        float width = ImGui::CalcTextSize(label.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;

        if (m_count++ &&
            ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + width <=
                ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x)
        {
            ImGui::SameLine();
        }

        return ImGui::Button(label.c_str());
    }

private:
    int m_count = 0;
};

void ShellOpen(const std::filesystem::path& path)
{
    ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

}

ArchiveWorkspace::ArchiveWorkspace(HWND owner, const Schema& schema, ArchiveActions actions)
    : m_owner(owner), m_schema(schema), m_actions(std::move(actions))
{
}

ArchiveWorkspace::~ArchiveWorkspace()
{
    if (m_job.joinable())
    {
        m_jobCancel = true;
        m_job.join();
    }
}

bool ArchiveWorkspace::Accepts(const std::filesystem::path& path)
{
    return Lower(path.extension().string()) == ".rcf";
}

std::string ArchiveWorkspace::Title() const
{
    if (!m_hasArchive)
    {
        return "no archive";
    }

    return (m_path.empty() ? std::string("untitled.rcf") : m_path.filename().u8string()) + (m_dirty ? " *" : "");
}

bool ArchiveWorkspace::ConfirmDiscard()
{
    if (Busy())
    {
        return false;
    }

    ApplyWorkingCopies();

    if (!m_dirty)
    {
        return true;
    }

    std::wstring message =
        L"Save changes to " + (m_path.empty() ? std::wstring(L"the new archive") : m_path.filename().wstring()) + L"?";
    int answer = MessageBoxW(m_owner, message.c_str(), L"ToolKit", MB_YESNOCANCEL | MB_ICONQUESTION);

    if (answer == IDCANCEL)
    {
        return false;
    }

    if (answer == IDYES)
    {
        Save();
        return false;
    }

    return true;
}

void ArchiveWorkspace::New()
{
    if (!ConfirmDiscard())
    {
        return;
    }

    m_archive.Close();
    m_hasArchive = true;
    m_path.clear();
    m_alignment = kCementAlignment;
    m_sources.clear();
    m_working.clear();
    m_selected.clear();
    m_selectedFolders.clear();
    m_folder.clear();
    m_history.clear();
    m_dirty = false;
    m_treeDirty = m_rowsDirty = true;
    m_focusFrames = 3;
}

void ArchiveWorkspace::Open(const std::filesystem::path& path)
{
    if (!ConfirmDiscard())
    {
        return;
    }

    std::string error;

    if (!Reload(path, error))
    {
        return m_actions.error(error);
    }

    m_working.clear();
    m_selected.clear();
    m_selectedFolders.clear();
    m_folder.clear();
    m_history.clear();
    m_focusFrames = 3;
}

bool ArchiveWorkspace::Reload(const std::filesystem::path& path, std::string& error)
{
    if (!m_archive.Open(path, error))
    {
        return false;
    }

    m_hasArchive = true;
    m_path = path;
    m_alignment = m_archive.Alignment();
    m_sources = SourcesFromArchive(m_archive);
    m_dirty = false;
    m_treeDirty = m_rowsDirty = true;
    ++m_previewStamp;

    for (auto& [hash, copy] : m_working)
    {
        copy.extractedTime = GetFileUnixTime(copy.path).value_or(0);
    }

    return true;
}

void ArchiveWorkspace::OpenWithDialog()
{
    if (auto path = ShowOpenArchiveDialog(m_owner))
    {
        Open(*path);
    }
}

void ArchiveWorkspace::Save()
{
    if (!m_hasArchive || Busy())
    {
        return;
    }

    if (m_path.empty())
    {
        return SaveAs();
    }

    SaveTo(m_path);
}

void ArchiveWorkspace::SaveAs()
{
    if (!m_hasArchive || Busy())
    {
        return;
    }

    if (auto path = ShowSaveArchiveDialog(m_owner, m_path.empty() ? std::filesystem::path(L"new.rcf") : m_path))
    {
        SaveTo(*path);
    }
}

void ArchiveWorkspace::SaveTo(const std::filesystem::path& target)
{
    m_actions.flushEditor(WorkDirectory());
    ApplyWorkingCopies();
    auto sources = std::make_shared<std::vector<CementSource>>(m_sources);
    uint32_t alignment = m_alignment;
    RunJob(
        "Saving " + target.filename().u8string(),
        [this, sources, alignment, target](const CementProgress& progress, std::string& error)
        {
            return SaveCement(*sources,
                alignment,
                target,
                progress,
                error,
                [this]
                {
                    m_archive.Close();
                });
        },
        [this, target]
        {
            std::string error;

            if (!Reload(target, error))
            {
                m_actions.error(error);
            }
        });
}

void ArchiveWorkspace::PackFolder()
{
    if (Busy())
    {
        return;
    }

    auto folder = ShowFolderDialog(m_owner, L"Folder to pack (an extracted archive keeps its original order)");

    if (!folder)
    {
        return;
    }

    auto target = ShowSaveArchiveDialog(m_owner, folder->filename().wstring() + L".rcf");

    if (!target)
    {
        return;
    }

    std::filesystem::path root = *folder, output = *target;
    RunJob(
        "Packing " + root.filename().u8string(),
        [root, output](const CementProgress& progress, std::string& error)
        {
            std::vector<CementSource> sources;
            uint32_t alignment = kCementAlignment;
            return CollectCementFolder(root, sources, alignment, error) &&
                SaveCement(sources, alignment, output, progress, error);
        },
        [this, output]
        {
            Open(output);
        });
}

void ArchiveWorkspace::MarkChanged()
{
    m_dirty = true;
    m_treeDirty = m_rowsDirty = true;
    ++m_previewStamp;
}

std::filesystem::path ArchiveWorkspace::WorkDirectory() const
{
    wchar_t temp[MAX_PATH];
    GetTempPathW(MAX_PATH, temp);
    std::wstring stem = m_path.empty() ? std::wstring(L"untitled") : m_path.stem().wstring();
    wchar_t suffix[16];
    swprintf(suffix, 16, L"-%08X", CementHash(m_path.u8string()));
    return std::filesystem::path(temp) / L"ToolKit" / L"rcf" / (stem + suffix);
}

void ArchiveWorkspace::ApplyWorkingCopies()
{
    for (auto& [hash, copy] : m_working)
    {
        auto time = GetFileUnixTime(copy.path);

        if (!time || *time == copy.extractedTime)
        {
            continue;
        }

        copy.extractedTime = *time;

        for (CementSource& source : m_sources)
        {
            if (CementHash(source.name) != hash)
            {
                continue;
            }

            CementSource edited = CementSource::FromFile(source.name, copy.path);
            edited.alignment = source.alignment;
            edited.reserved = source.reserved;
            source = edited;
            MarkChanged();
        }
    }
}

void ArchiveWorkspace::AddSource(CementSource source)
{
    uint32_t hash = CementHash(source.name);
    m_working.erase(hash);
    m_selected.insert(hash);
    MarkChanged();

    for (CementSource& existing : m_sources)
    {
        if (CementHash(existing.name) != hash)
        {
            continue;
        }

        source.name = existing.name;
        source.alignment = existing.alignment;
        source.reserved = existing.reserved;
        existing = std::move(source);
        return;
    }

    source.alignment = m_alignment;
    m_sources.push_back(std::move(source));
}

void ArchiveWorkspace::AddFolderContents(const std::filesystem::path& folder, const std::string& prefix)
{
    std::vector<CementSource> sources;
    uint32_t alignment = kCementAlignment;
    std::string error;

    if (!CollectCementFolder(folder, sources, alignment, error))
    {
        return m_actions.error(error);
    }

    for (CementSource& source : sources)
    {
        source.name = CementNormalizeName(Join(prefix, source.name));
        AddSource(std::move(source));
    }
}

void ArchiveWorkspace::AddDropped(const std::vector<std::filesystem::path>& paths)
{
    if (Busy())
    {
        return;
    }

    if (!m_hasArchive)
    {
        New();
    }

    m_selected.clear();
    m_selectedFolders.clear();

    for (const auto& path : paths)
    {
        std::error_code ec;

        if (std::filesystem::is_directory(path, ec))
        {
            AddFolderContents(path, Join(m_folder, path.filename().string()));
        }
        else
        {
            std::string name = CementNormalizeName(Join(m_folder, path.filename().string()));

            if (!name.empty())
            {
                AddSource(CementSource::FromFile(name, path));
            }
        }
    }
}

void ArchiveWorkspace::AddFilesWithDialog()
{
    std::vector<std::filesystem::path> files = ShowOpenFilesDialog(m_owner);

    if (!files.empty())
    {
        AddDropped(files);
    }
}

void ArchiveWorkspace::AddFolderWithDialog()
{
    if (auto folder = ShowFolderDialog(m_owner, L"Add folder into the current archive folder"))
    {
        AddDropped({*folder});
    }
}

void ArchiveWorkspace::ReplaceSelected()
{
    std::vector<size_t> selected = SelectedSources();

    if (selected.size() != 1)
    {
        return;
    }

    std::vector<std::filesystem::path> files = ShowOpenFilesDialog(m_owner);

    if (files.size() != 1)
    {
        return;
    }

    AddSource(CementSource::FromFile(m_sources[selected[0]].name, files[0]));
}

void ArchiveWorkspace::RemoveSelected()
{
    std::vector<size_t> selected = SelectedSources();

    if (selected.empty())
    {
        return;
    }

    std::set<size_t> drop(selected.begin(), selected.end());
    std::vector<CementSource> kept;

    for (size_t i = 0; i < m_sources.size(); ++i)
    {
        if (drop.count(i))
        {
            m_working.erase(CementHash(m_sources[i].name));
        }
        else
        {
            kept.push_back(std::move(m_sources[i]));
        }
    }

    m_sources = std::move(kept);
    m_selected.clear();
    m_selectedFolders.clear();
    MarkChanged();
}

void ArchiveWorkspace::ExtractSelected()
{
    std::vector<size_t> selected = SelectedSources();

    if (selected.empty())
    {
        return;
    }

    auto folder = ShowFolderDialog(m_owner, L"Extract selected files to");

    if (!folder)
    {
        return;
    }

    ApplyWorkingCopies();
    auto sources = std::make_shared<std::vector<CementSource>>();

    for (size_t index : selected)
    {
        CementSource source = m_sources[index];

        if (IsUnder(source.name, m_folder))
        {
            source.name = source.name.substr(m_folder.empty() ? 0 : m_folder.size() + 1);
        }

        sources->push_back(std::move(source));
    }

    uint32_t alignment = m_alignment;
    std::filesystem::path root = *folder;
    RunJob(
        "Extracting " + std::to_string(sources->size()) + " files",
        [sources, root, alignment](const CementProgress& progress, std::string& error)
        {
            return ExtractCementSources(*sources, root, std::string(), alignment, progress, error);
        },
        [root]
        {
            ShellOpen(root);
        });
}

void ArchiveWorkspace::ExtractAll()
{
    if (!m_hasArchive || m_sources.empty())
    {
        return;
    }

    auto folder = ShowFolderDialog(m_owner, L"Extract the whole archive to (writes cement.manifest for repacking)");

    if (!folder)
    {
        return;
    }

    ApplyWorkingCopies();
    auto sources = std::make_shared<std::vector<CementSource>>(m_sources);
    uint32_t alignment = m_alignment;
    std::filesystem::path root = *folder;
    std::string label = m_path.empty() ? std::string("untitled.rcf") : m_path.filename().u8string();
    RunJob(
        "Extracting " + label,
        [sources, root, alignment, label](const CementProgress& progress, std::string& error)
        {
            return ExtractCementSources(*sources, root, label, alignment, progress, error);
        },
        [root]
        {
            ShellOpen(root);
        });
}

void ArchiveWorkspace::EditInToolKit(size_t index)
{
    const CementSource& source = m_sources[index];
    uint32_t hash = CementHash(source.name);
    auto it = m_working.find(hash);

    if (it == m_working.end() || !std::filesystem::exists(it->second.path))
    {
        std::string error;

        if (!ExtractCementSources({source}, WorkDirectory(), std::string(), m_alignment, nullptr, error))
        {
            return m_actions.error(error);
        }

        std::filesystem::path path = CementEntryPath(WorkDirectory(), source.name);
        it = m_working.insert_or_assign(hash, WorkingCopy{path, GetFileUnixTime(path).value_or(0)}).first;
    }

    if (KindOf(source.name).icon == icon::Model)
    {
        m_actions.editFile(it->second.path);
    }
    else
    {
        ShellOpen(it->second.path);
    }
}

void ArchiveWorkspace::RunJob(std::string title,
    std::function<bool(const CementProgress&, std::string&)> work,
    std::function<void()> done)
{
    if (Busy())
    {
        return;
    }

    m_jobTitle = std::move(title);
    m_jobDone = 0;
    m_jobTotal = 0;
    m_jobCancel = false;
    m_jobFinished = false;
    m_jobError.clear();
    m_jobDoneCallback = std::move(done);
    m_job = std::thread(
        [this, work = std::move(work)]
        {
            CementProgress progress = [this](uint64_t done, uint64_t total)
            {
                m_jobDone = done;
                m_jobTotal = total;
                return !m_jobCancel.load();
            };
            std::string error;
            m_jobOk = work(progress, error);
            m_jobError = error;
            m_jobFinished = true;
        });
}

void ArchiveWorkspace::FinishJob()
{
    m_job.join();
    std::function<void()> done = std::move(m_jobDoneCallback);
    m_jobDoneCallback = nullptr;

    if (m_jobOk)
    {
        if (done)
        {
            done();
        }
    }
    else if (m_jobError != "Cancelled")
    {
        m_actions.error(m_jobError);
    }

    std::string ignored;

    if (!m_jobOk && !m_path.empty() && !m_archive.IsOpen() && m_hasArchive)
    {
        m_archive.Open(m_path, ignored);
    }
}

void ArchiveWorkspace::RebuildTree()
{
    m_treeDirty = false;
    m_root = Folder();

    for (const CementSource& source : m_sources)
    {
        Folder* folder = &m_root;
        folder->totalFiles++;
        folder->totalBytes += source.size;
        std::string parent = ParentOf(source.name);
        size_t start = 0;

        while (!parent.empty() && start <= parent.size())
        {
            size_t end = parent.find('\\', start);

            if (end == std::string::npos)
            {
                end = parent.size();
            }

            std::string part = parent.substr(start, end - start);
            auto& child = folder->children[Lower(part)];

            if (!child)
            {
                child = std::make_unique<Folder>();
                child->name = part;
                child->path = parent.substr(0, end);
            }

            folder = child.get();
            folder->totalFiles++;
            folder->totalBytes += source.size;
            start = end + 1;
        }

        folder->files++;
    }

    if (!FindFolder(m_folder))
    {
        std::string folder = m_folder;

        while (!folder.empty() && !FindFolder(folder))
        {
            folder = ParentOf(folder);
        }

        m_folder = folder;
    }
}

const ArchiveWorkspace::Folder* ArchiveWorkspace::FindFolder(const std::string& path) const
{
    const Folder* folder = &m_root;
    size_t start = 0;

    while (!path.empty() && start <= path.size())
    {
        size_t end = path.find('\\', start);

        if (end == std::string::npos)
        {
            end = path.size();
        }

        auto it = folder->children.find(Lower(path.substr(start, end - start)));

        if (it == folder->children.end())
        {
            return nullptr;
        }

        folder = it->second.get();
        start = end + 1;
    }

    return folder;
}

void ArchiveWorkspace::RebuildRows()
{
    if (m_treeDirty)
    {
        RebuildTree();
    }

    m_rowsDirty = false;
    m_rows.clear();
    std::string filter = Lower(m_filter);
    const Folder* current = FindFolder(m_folder);

    if (filter.empty() && current)
    {
        for (const auto& [key, child] : current->children)
        {
            m_rows.push_back({child.get(), 0});
        }
    }

    for (size_t i = 0; i < m_sources.size(); ++i)
    {
        const std::string& name = m_sources[i].name;

        if (filter.empty())
        {
            if (Lower(ParentOf(name)) == Lower(m_folder))
            {
                m_rows.push_back({nullptr, i});
            }
        }
        else if (IsUnder(name, m_folder) && Lower(name).find(filter) != std::string::npos)
        {
            m_rows.push_back({nullptr, i});
        }
    }

    auto key = [&](const Row& a, const Row& b)
    {
        if (a.folder || b.folder)
        {
            if (a.folder && b.folder)
            {
                return Lower(a.folder->name) < Lower(b.folder->name);
            }

            return false;
        }

        const CementSource& x = m_sources[a.source];
        const CementSource& y = m_sources[b.source];

        switch (m_sortColumn)
        {
        case 1:
            return std::strcmp(KindOf(x.name).label, KindOf(y.name).label) < 0;
        case 2:
            return x.size < y.size;
        case 3:
            return x.timestamp < y.timestamp;
        case 4:
            return x.offset < y.offset;
        case 5:
            return CementHash(x.name) < CementHash(y.name);
        default:
            return Lower(x.name) < Lower(y.name);
        }
    };
    std::stable_sort(m_rows.begin(),
        m_rows.end(),
        [&](const Row& a, const Row& b)
        {
            if (bool(a.folder) != bool(b.folder))
            {
                return bool(a.folder);
            }

            if (m_sortColumn < 0 && !a.folder)
            {
                return a.source < b.source;
            }

            return m_sortAscending ? key(a, b) : key(b, a);
        });
}

void ArchiveWorkspace::EnterFolder(const std::string& path)
{
    if (Lower(path) == Lower(m_folder))
    {
        return;
    }

    m_history.push_back(m_folder);
    m_folder = path;
    m_filter.clear();
    m_selected.clear();
    m_selectedFolders.clear();
    m_anchorRow = -1;
    m_rowsDirty = true;
    m_revealFolder = path;
}

void ArchiveWorkspace::ActivateRow(const Row& row)
{
    if (row.folder)
    {
        return EnterFolder(row.folder->path);
    }

    EditInToolKit(row.source);
}

bool ArchiveWorkspace::RowSelected(const Row& row) const
{
    if (row.folder)
    {
        return m_selectedFolders.count(Lower(row.folder->path)) != 0;
    }

    return m_selected.count(CementHash(m_sources[row.source].name)) != 0;
}

void ArchiveWorkspace::SelectRow(size_t index, bool ctrl, bool shift)
{
    auto toggle = [&](const Row& row, bool on)
    {
        if (row.folder)
        {
            std::string key = Lower(row.folder->path);
            on ? (void)m_selectedFolders.insert(key) : (void)m_selectedFolders.erase(key);
        }
        else
        {
            uint32_t hash = CementHash(m_sources[row.source].name);
            on ? (void)m_selected.insert(hash) : (void)m_selected.erase(hash);
        }
    };

    if (shift && m_anchorRow >= 0 && m_anchorRow < static_cast<int>(m_rows.size()))
    {
        if (!ctrl)
        {
            m_selected.clear();
            m_selectedFolders.clear();
        }

        size_t from = std::min<size_t>(m_anchorRow, index), to = std::max<size_t>(m_anchorRow, index);

        for (size_t i = from; i <= to; ++i)
        {
            toggle(m_rows[i], true);
        }

        return;
    }

    if (ctrl)
    {
        toggle(m_rows[index], !RowSelected(m_rows[index]));
    }
    else
    {
        m_selected.clear();
        m_selectedFolders.clear();
        toggle(m_rows[index], true);
    }

    m_anchorRow = static_cast<int>(index);
}

std::vector<size_t> ArchiveWorkspace::SelectedSources() const
{
    std::vector<size_t> result;

    for (size_t i = 0; i < m_sources.size(); ++i)
    {
        const std::string& name = m_sources[i].name;
        bool picked = m_selected.count(CementHash(name)) != 0;

        for (auto it = m_selectedFolders.begin(); !picked && it != m_selectedFolders.end(); ++it)
        {
            picked = IsUnder(Lower(name), *it);
        }

        if (picked)
        {
            result.push_back(i);
        }
    }

    return result;
}

void ArchiveWorkspace::Draw()
{
    if (m_jobFinished && m_job.joinable())
    {
        FinishJob();
    }

    if (!Busy() && ImGui::GetTime() - m_lastWorkingCheck > 1.0)
    {
        m_lastWorkingCheck = ImGui::GetTime();
        ApplyWorkingCopies();
    }

    if (m_treeDirty)
    {
        RebuildTree();
    }

    if (m_rowsDirty)
    {
        RebuildRows();
    }

    DrawFolders();
    DrawFiles();
    DrawDetails();
    DrawJobPopup();
}

void ArchiveWorkspace::DrawFolders()
{
    if (!ImGui::Begin(FoldersWindow()))
    {
        return ImGui::End();
    }

    if (!m_hasArchive)
    {
        ImGui::TextDisabled("No archive open.");
        return ImGui::End();
    }

    std::string label = std::string(icon::Archive) + "  " + Title() + "##root";
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (m_folder.empty())
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool open = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        EnterFolder("");
    }

    if (open)
    {
        for (const auto& [key, child] : m_root.children)
        {
            DrawFolderNode(*child);
        }

        ImGui::TreePop();
    }

    m_revealFolder.clear();
    ImGui::End();
}

void ArchiveWorkspace::DrawFolderNode(const Folder& folder)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (folder.children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool current = Lower(folder.path) == Lower(m_folder);

    if (current)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (!m_revealFolder.empty() && IsUnder(Lower(m_revealFolder), Lower(folder.path)))
    {
        ImGui::SetNextItemOpen(true);
    }

    std::string label =
        std::string(current ? icon::FolderOpen : icon::Folder) + "  " + folder.name + "##" + folder.path;
    ImGui::PushStyleColor(ImGuiCol_Text, kFolderColour);
    bool open = ImGui::TreeNodeEx(label.c_str(), flags);
    ImGui::PopStyleColor();

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        EnterFolder(folder.path);
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%zu files, %s", folder.totalFiles, FormatSize(folder.totalBytes).c_str());
    }

    if (open)
    {
        for (const auto& [key, child] : folder.children)
        {
            DrawFolderNode(*child);
        }

        ImGui::TreePop();
    }
}

void ArchiveWorkspace::DrawEmpty()
{
    ImGui::Dummy(ImVec2(0, 40));
    ImGui::TextUnformatted("Cement archives");
    ImGui::TextDisabled("patch01.rcf, cement.rcf, movies.rcf, sound1.rcf, sound2.rcf, snd_*.rcf");
    ImGui::Dummy(ImVec2(0, 8));

    if (ImGui::Button((std::string(icon::FolderOpen) + "  Open archive...").c_str()))
    {
        OpenWithDialog();
    }

    ImGui::SameLine();

    if (ImGui::Button((std::string(icon::Add) + "  New archive").c_str()))
    {
        New();
    }

    ImGui::SameLine();

    if (ImGui::Button((std::string(icon::Archive) + "  Pack a folder...").c_str()))
    {
        PackFolder();
    }

    ImGui::Dummy(ImVec2(0, 8));
    ImGui::TextDisabled(
        "Or drop an .rcf on the window. Dropping other files or folders starts a new archive with them.");
}

void ArchiveWorkspace::DrawFiles()
{
    if (m_focusFrames > 0)
    {
        ImGui::SetNextWindowFocus();
        --m_focusFrames;
    }

    if (!ImGui::Begin(FilesWindow()))
    {
        return ImGui::End();
    }

    if (!m_hasArchive)
    {
        DrawEmpty();
        return ImGui::End();
    }

    DrawFilesToolbar();
    DrawFileTable();

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        HandleFileKeys();
    }

    ImGui::End();
}

void ArchiveWorkspace::DrawFilesToolbar()
{
    auto button = [](const char* glyph, const char* text)
    {
        return ImGui::Button((std::string(glyph) + "  " + text).c_str());
    };

    if (button(icon::Save, "Save"))
    {
        Save();
    }

    ImGui::SameLine();

    if (button(icon::Add, "Add files"))
    {
        AddFilesWithDialog();
    }

    ImGui::SameLine();

    if (button(icon::Folder, "Add folder"))
    {
        AddFolderWithDialog();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!HasSelection());

    if (button(icon::Download, "Extract"))
    {
        ExtractSelected();
    }

    ImGui::SameLine();

    if (button(icon::Delete, "Remove"))
    {
        RemoveSelected();
    }

    ImGui::EndDisabled();
    ImGui::SameLine();

    if (button(icon::Download, "Extract all"))
    {
        ExtractAll();
    }

    ImGui::BeginDisabled(m_folder.empty());

    if (ImGui::Button(icon::Up))
    {
        EnterFolder(ParentOf(m_folder));
    }

    ImGui::EndDisabled();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Up (Backspace)");
    }

    ImGui::SameLine();

    if (ImGui::SmallButton((std::string(icon::Archive) + " " +
            (m_path.empty() ? std::string("untitled.rcf") : m_path.filename().u8string()))
                .c_str()))
    {
        EnterFolder("");
    }

    std::string crumb;
    size_t start = 0;

    while (!m_folder.empty() && start <= m_folder.size())
    {
        size_t end = m_folder.find('\\', start);

        if (end == std::string::npos)
        {
            end = m_folder.size();
        }

        crumb = m_folder.substr(0, end);
        ImGui::SameLine(0, 2);
        ImGui::TextDisabled("%s", icon::Chevron);
        ImGui::SameLine(0, 2);
        ImGui::PushID(crumb.c_str());

        if (ImGui::SmallButton(m_folder.substr(start, end - start).c_str()))
        {
            EnterFolder(crumb);
        }

        ImGui::PopID();
        start = end + 1;
    }

    float filterWidth = 260.0f;
    ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 8.0f, ImGui::GetWindowContentRegionMax().x - filterWidth));
    ImGui::TextDisabled("%s", icon::Search);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);

    if (InputString("##search", m_filter))
    {
        m_rowsDirty = true;
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Search names in this folder and below");
    }
}

void ArchiveWorkspace::DrawFileTable()
{
    if (m_rowsDirty)
    {
        RebuildRows();
    }

    ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_Sortable | ImGuiTableFlags_SortTristate;
    float footer = ImGui::GetFrameHeightWithSpacing();

    if (!ImGui::BeginTable("files", 6, flags, ImVec2(0, -footer)))
    {
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120);
    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80);
    ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed, 125);
    ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_WidthFixed, 90);
    ImGui::TableSetupColumn("Hash", ImGuiTableColumnFlags_WidthFixed, 95);
    ImGui::TableHeadersRow();

    if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs(); specs && specs->SpecsDirty)
    {
        m_sortColumn = specs->SpecsCount ? specs->Specs[0].ColumnIndex : -1;
        m_sortAscending = !specs->SpecsCount || specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
        specs->SpecsDirty = false;
        RebuildRows();
    }

    std::function<void()> deferred;
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(m_rows.size()));

    while (clipper.Step())
    {
        for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; ++index)
        {
            const Row& row = m_rows[index];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushID(index);
            bool selected = RowSelected(row);
            std::string label;

            if (row.folder)
            {
                label = std::string(icon::Folder) + "  " + row.folder->name;
                ImGui::PushStyleColor(ImGuiCol_Text, kFolderColour);
            }
            else
            {
                const CementSource& source = m_sources[row.source];
                std::string shown = m_filter.empty() ? LeafOf(source.name)
                                                     : source.name.substr(m_folder.empty() ? 0 : m_folder.size() + 1);
                label = std::string(KindOf(source.name).icon) + "  " + shown;

                if (source.kind != CementSource::Kind::ArchiveSlice)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, kNewColour);
                }
            }

            bool tinted = row.folder || m_sources[row.source].kind != CementSource::Kind::ArchiveSlice;

            if (ImGui::Selectable(label.c_str(),
                    selected,
                    ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
            {
                ImGuiIO& io = ImGui::GetIO();

                if (ImGui::IsMouseDoubleClicked(0))
                {
                    Row copy = row;
                    deferred = [this, copy]
                    {
                        ActivateRow(copy);
                    };
                }
                else
                {
                    SelectRow(index, io.KeyCtrl, io.KeyShift);
                }
            }

            if (tinted)
            {
                ImGui::PopStyleColor();
            }

            if (ImGui::BeginPopupContextItem("row"))
            {
                if (!selected)
                {
                    SelectRow(index, false, false);
                }

                DrawRowMenu(row);
                ImGui::EndPopup();
            }

            ImGui::TableNextColumn();

            if (row.folder)
            {
                ImGui::TextDisabled("Folder");
                ImGui::TableNextColumn();
                ImGui::TextDisabled("%s", FormatSize(row.folder->totalBytes).c_str());
                ImGui::TableNextColumn();
                ImGui::TextDisabled("%zu files", row.folder->totalFiles);
                ImGui::TableNextColumn();
                ImGui::TableNextColumn();
            }
            else
            {
                const CementSource& source = m_sources[row.source];
                uint32_t hash = CementHash(source.name);
                ImGui::TextUnformatted(KindOf(source.name).label);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatSize(source.size).c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatTime(source.timestamp).c_str());
                ImGui::TableNextColumn();

                if (source.kind == CementSource::Kind::ArchiveSlice)
                {
                    ImGui::TextUnformatted(FormatHex(source.offset).c_str());
                }
                else
                {
                    ImGui::TextColored(kNewColour, m_working.count(hash) ? "edited" : "new");
                }

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatHex(hash).c_str());
            }

            ImGui::PopID();
        }
    }

    ImGui::EndTable();

    if (deferred)
    {
        deferred();
    }

    size_t files = 0;

    for (const Row& row : m_rows)
    {
        files += row.folder == nullptr;
    }

    std::vector<size_t> selected = SelectedSources();
    uint64_t selectedBytes = 0;

    for (size_t index : selected)
    {
        selectedBytes += m_sources[index].size;
    }

    ImGui::TextDisabled("%zu folders, %zu files here  |  %zu files in archive (%s)",
        m_rows.size() - files,
        files,
        m_sources.size(),
        FormatSize(m_root.totalBytes).c_str());
    if (!selected.empty())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("  |  %zu selected (%s)", selected.size(), FormatSize(selectedBytes).c_str());
    }
}

void ArchiveWorkspace::DrawRowMenu(const Row& row)
{
    bool single = !row.folder && m_selected.size() == 1 && m_selectedFolders.empty();

    if (row.folder)
    {
        if (ImGui::MenuItem("Open"))
        {
            EnterFolder(row.folder->path);
        }
    }
    else
    {
        bool model = KindOf(m_sources[row.source].name).icon == icon::Model;

        if (ImGui::MenuItem(model ? "Edit in Pure3D" : "Open with default program", "Enter", false, single))
        {
            EditInToolKit(row.source);
        }
    }

    if (ImGui::MenuItem("Extract..."))
    {
        ExtractSelected();
    }

    if (ImGui::MenuItem("Replace...", nullptr, false, single))
    {
        ReplaceSelected();
    }

    if (ImGui::MenuItem("Remove", "Del"))
    {
        RemoveSelected();
    }

    ImGui::Separator();
    std::string name = row.folder ? row.folder->path : m_sources[row.source].name;

    if (ImGui::MenuItem("Copy path"))
    {
        ImGui::SetClipboardText(name.c_str());
    }

    if (!row.folder && ImGui::MenuItem("Copy hash"))
    {
        ImGui::SetClipboardText(FormatHex(CementHash(name)).c_str());
    }
}

void ArchiveWorkspace::HandleFileKeys()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput)
    {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        RemoveSelected();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && !m_folder.empty())
    {
        EnterFolder(ParentOf(m_folder));
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))
    {
        for (const Row& row : m_rows)
        {
            if (row.folder)
            {
                m_selectedFolders.insert(Lower(row.folder->path));
            }
            else
            {
                m_selected.insert(CementHash(m_sources[row.source].name));
            }
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Enter))
    {
        for (const Row& row : m_rows)
        {
            if (!RowSelected(row))
            {
                continue;
            }

            Row copy = row;
            ActivateRow(copy);
            break;
        }
    }
}

bool ArchiveWorkspace::LoadPreview(const CementSource& source)
{
    m_preview.clear();
    m_previewText.clear();
    m_previewChunks.clear();
    m_previewError.clear();
    FileKind kind = KindOf(source.name);
    bool model = kind.icon == icon::Model;
    size_t limit = model ? kPreviewLimit : std::max(kTextPreviewLimit, kHexPreviewBytes);

    if (model && source.size > kPreviewLimit)
    {
        m_previewError = "Too large to preview here; open it in the Pure3D workspace.";
        return false;
    }

    if (!ReadSourceBytes(source, limit, m_preview, m_previewError))
    {
        return false;
    }

    if (model)
    {
        ParseResult parsed = ParseP3d(m_preview);

        if (!parsed.root)
        {
            m_previewError = "Not a valid Pure3D file: " + parsed.error;
        }
        else
        {
            for (size_t i = 0; i < parsed.root->ChildCount(); ++i)
            {
                uint32_t id = parsed.root->Child(i)->Id();
                auto it = std::find_if(m_previewChunks.begin(),
                    m_previewChunks.end(),
                    [&](const auto& p)
                    {
                        return p.first == id;
                    });
                if (it == m_previewChunks.end())
                {
                    m_previewChunks.emplace_back(id, 1);
                }
                else
                {
                    ++it->second;
                }
            }
        }
    }
    else if (kind.text || LooksLikeText(m_preview))
    {
        m_previewText.assign(m_preview.begin(), m_preview.end());

        if (source.size > m_preview.size())
        {
            m_previewText += "\n...";
        }
    }

    return true;
}

void ArchiveWorkspace::DrawPreview(const CementSource& source)
{
    uint32_t hash = CementHash(source.name);

    if (hash != m_previewHash || m_previewLoadedStamp != m_previewStamp)
    {
        LoadPreview(source);
        m_previewHash = hash;
        m_previewLoadedStamp = m_previewStamp;
    }

    ImGui::SeparatorText("Preview");

    if (!m_previewError.empty())
    {
        ImGui::TextWrapped("%s", m_previewError.c_str());
        return;
    }

    if (!m_previewChunks.empty())
    {
        ImGui::TextDisabled("Top-level chunks");

        if (ImGui::BeginTable("chunks",
                3,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY,
                ImVec2(0, -ImGui::GetFrameHeightWithSpacing())))
        {
            ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthFixed, 90);
            ImGui::TableSetupColumn("Chunk", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 50);

            for (const auto& [id, count] : m_previewChunks)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatHex(id).c_str());
                ImGui::TableNextColumn();
                std::string name = m_schema.ChunkName(id);
                ImGui::TextUnformatted(name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%zu", count);
            }

            ImGui::EndTable();
        }

        return;
    }

    if (!m_previewText.empty())
    {
        ImGui::InputTextMultiline("##text",
            m_previewText.data(),
            m_previewText.size() + 1,
            ImVec2(-FLT_MIN, -FLT_MIN),
            ImGuiInputTextFlags_ReadOnly);
        return;
    }

    if (KindOf(source.name).icon == icon::Script)
    {
        ImGui::TextDisabled("Compiled Torque script. Decompiling is not built in yet; showing bytes.");
    }

    ImGui::BeginChild("hex", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    size_t count = std::min<size_t>(m_preview.size(), kHexPreviewBytes);
    float glyph = ImGui::CalcTextSize("0").x;
    size_t perLine = ImGui::GetContentRegionAvail().x >= glyph * (8 + 16 * 4 + 2) ? 16 : 8;
    char line[128];

    for (size_t row = 0; row < count; row += perLine)
    {
        int length = std::snprintf(line, sizeof(line), "%06zX ", row);

        for (size_t i = 0; i < perLine; ++i)
        {
            length += row + i < count ? std::snprintf(line + length, sizeof(line) - length, " %02X", m_preview[row + i])
                                      : std::snprintf(line + length, sizeof(line) - length, "   ");
        }

        length += std::snprintf(line + length, sizeof(line) - length, "  ");

        for (size_t i = 0; i < perLine && row + i < count; ++i)
        {
            uint8_t c = m_preview[row + i];
            line[length++] = c >= 0x20 && c < 0x7F ? static_cast<char>(c) : '.';
        }

        line[length] = '\0';
        ImGui::TextUnformatted(line);
    }

    if (m_preview.size() > count)
    {
        ImGui::TextDisabled("... first %zu of %u bytes", count, source.size);
    }

    ImGui::EndChild();
}

void ArchiveWorkspace::DrawDetails()
{
    if (!ImGui::Begin(DetailsWindow()))
    {
        return ImGui::End();
    }

    if (!m_hasArchive)
    {
        ImGui::TextDisabled("Open or create an archive.");
        return ImGui::End();
    }

    auto property = [](const char* key, const std::string& value)
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%s", key);
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(value.c_str());
    };
    auto beginProperties = []
    {
        return ImGui::BeginTable("properties", 2, ImGuiTableFlags_SizingFixedFit);
    };

    std::vector<size_t> selected = SelectedSources();
    bool singleFile = m_selected.size() == 1 && m_selectedFolders.empty() && selected.size() == 1;

    if (singleFile)
    {
        const CementSource& source = m_sources[selected[0]];
        uint32_t hash = CementHash(source.name);
        FileKind kind = KindOf(source.name);
        ImGui::Text("%s  %s", kind.icon, LeafOf(source.name).c_str());

        if (beginProperties())
        {
            property("Folder", ParentOf(source.name).empty() ? std::string("(root)") : ParentOf(source.name));
            property("Type", kind.label);
            property("Size", FormatSize(source.size) + "  (" + std::to_string(source.size) + " bytes)");
            property("Modified", FormatTime(source.timestamp));
            property("Hash", FormatHex(hash));
            property("Offset",
                source.kind == CementSource::Kind::ArchiveSlice ? FormatHex(source.offset)
                                                                : std::string("(not saved yet)"));
            property("Alignment", FormatHex(source.alignment));
            property("State",
                source.kind == CementSource::Kind::ArchiveSlice ? "in archive"
                    : m_working.count(hash)                     ? "edited"
                                                                : "added");
            ImGui::EndTable();
        }

        bool model = kind.icon == icon::Model;
        ButtonRow row;

        if (row.Button(model ? icon::Edit : icon::FolderOpen, model ? "Edit in Pure3D" : "Open"))
        {
            EditInToolKit(selected[0]);
        }

        if (row.Button(icon::Download, "Extract"))
        {
            ExtractSelected();
        }

        if (row.Button(icon::Swap, "Replace"))
        {
            ReplaceSelected();
        }

        if (row.Button(icon::Delete, "Remove"))
        {
            RemoveSelected();
        }

        DrawPreview(source);
    }
    else if (!selected.empty() || !m_selectedFolders.empty())
    {
        uint64_t bytes = 0;

        for (size_t index : selected)
        {
            bytes += m_sources[index].size;
        }

        ImGui::Text("%zu files selected", selected.size());

        if (beginProperties())
        {
            property("Folders", std::to_string(m_selectedFolders.size()));
            property("Total size", FormatSize(bytes));
            ImGui::EndTable();
        }

        ButtonRow row;

        if (row.Button(icon::Download, "Extract"))
        {
            ExtractSelected();
        }

        if (row.Button(icon::Delete, "Remove"))
        {
            RemoveSelected();
        }
    }
    else
    {
        const Folder* folder = FindFolder(m_folder);
        ImGui::Text("%s  %s", icon::Archive, Title().c_str());

        if (beginProperties())
        {
            property("Path", m_path.empty() ? std::string("(not saved yet)") : m_path.u8string());
            property("Format",
                "ATG cement " + std::to_string(kCementVersionMajor) + "." + std::to_string(kCementVersionMinor) +
                    ", alignment " + FormatHex(m_alignment));
            property("Files", std::to_string(m_root.totalFiles) + "  (" + FormatSize(m_root.totalBytes) + ")");

            if (folder && !m_folder.empty())
            {
                property("This folder",
                    std::to_string(folder->totalFiles) + " files  (" + FormatSize(folder->totalBytes) + ")");
            }

            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0, 6));
        ImGui::TextDisabled("Select a file to see its details and a preview.");
        ImGui::TextDisabled("Drop files or folders on the window to add them to the current folder.");
    }

    ImGui::End();
}

void ArchiveWorkspace::DrawJobPopup()
{
    if (!Busy())
    {
        return;
    }

    if (!ImGui::IsPopupOpen("Working"))
    {
        ImGui::OpenPopup("Working");
    }

    ImGui::SetNextWindowSize(ImVec2(440, 0));

    if (!ImGui::BeginPopupModal("Working", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        return;
    }

    ImGui::TextUnformatted(m_jobTitle.c_str());
    uint64_t total = m_jobTotal, done = m_jobDone;
    float fraction = total ? static_cast<float>(double(done) / double(total)) : 0.0f;
    std::string label = FormatSize(done) + " / " + FormatSize(total);
    ImGui::ProgressBar(fraction, ImVec2(-FLT_MIN, 0), label.c_str());
    ImGui::BeginDisabled(m_jobCancel);

    if (ImGui::Button("Cancel", ImVec2(120, 0)))
    {
        m_jobCancel = true;
    }

    ImGui::EndDisabled();

    if (m_jobFinished)
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

}
