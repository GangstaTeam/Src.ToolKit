#pragma once

#include "core/Bytes.hh"
#include "p3d/Schema.hh"
#include "rcf/CementArchive.hh"
#include "rcf/CementWriter.hh"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace tk
{

enum EArchiveWorkspaceSize : size_t
{
    kPreviewLimit = 16u << 20,
    kTextPreviewLimit = 256u << 10,
    kHexPreviewBytes = 4096,
};

struct ArchiveActions
{
    std::function<void(const std::filesystem::path&)> editFile;
    std::function<void(const std::filesystem::path& workDir)> flushEditor;
    std::function<void(const std::string&)> error;
};

class ArchiveWorkspace
{
public:
    ArchiveWorkspace(HWND owner, const Schema& schema, ArchiveActions actions);
    ~ArchiveWorkspace();

    static bool Accepts(const std::filesystem::path& path);
    static const char* FoldersWindow() { return "Folders"; }
    static const char* FilesWindow() { return "Files"; }
    static const char* DetailsWindow() { return "Details"; }

    void Draw();
    void Open(const std::filesystem::path& path);
    void OpenWithDialog();
    void New();
    void Save();
    void SaveAs();
    void PackFolder();
    void AddFilesWithDialog();
    void AddFolderWithDialog();
    void ExtractSelected();
    void ExtractAll();
    void AddDropped(const std::vector<std::filesystem::path>& paths);
    bool ConfirmDiscard();

    bool HasArchive() const { return m_hasArchive; }
    bool HasSelection() const { return !m_selected.empty() || !m_selectedFolders.empty(); }
    bool Dirty() const { return m_dirty; }
    bool Busy() const { return m_job.joinable(); }
    std::string Title() const;

private:
    struct Folder
    {
        std::string name;
        std::string path;
        std::map<std::string, std::unique_ptr<Folder>> children;
        size_t files = 0;
        size_t totalFiles = 0;
        uint64_t totalBytes = 0;
    };

    struct Row
    {
        const Folder* folder = nullptr;
        size_t source = 0;
    };

    struct WorkingCopy
    {
        std::filesystem::path path;
        uint32_t extractedTime = 0;
    };

    void DrawFolders();
    void DrawFolderNode(const Folder& folder);
    void DrawFiles();
    void DrawFilesToolbar();
    void DrawFileTable();
    void DrawRowMenu(const Row& row);
    void DrawDetails();
    void DrawPreview(const CementSource& source);
    void DrawJobPopup();
    void DrawEmpty();
    void HandleFileKeys();

    void RebuildTree();
    void RebuildRows();
    const Folder* FindFolder(const std::string& path) const;
    void EnterFolder(const std::string& path);
    void ActivateRow(const Row& row);
    void SelectRow(size_t row, bool ctrl, bool shift);
    bool RowSelected(const Row& row) const;
    std::vector<size_t> SelectedSources() const;
    void MarkChanged();

    void AddSource(CementSource source);
    void AddFolderContents(const std::filesystem::path& folder, const std::string& prefix);
    void ReplaceSelected();
    void RemoveSelected();
    void EditInToolKit(size_t index);
    void ApplyWorkingCopies();
    std::filesystem::path WorkDirectory() const;
    bool LoadPreview(const CementSource& source);

    void RunJob(std::string title,
        std::function<bool(const CementProgress&, std::string&)> work,
        std::function<void()> done);
    void FinishJob();
    void SaveTo(const std::filesystem::path& target);
    bool Reload(const std::filesystem::path& path, std::string& error);

    HWND m_owner;
    const Schema& m_schema;
    ArchiveActions m_actions;

    CementArchive m_archive;
    bool m_hasArchive = false;
    std::filesystem::path m_path;
    uint32_t m_alignment = kCementAlignment;
    std::vector<CementSource> m_sources;
    std::map<uint32_t, WorkingCopy> m_working;
    bool m_dirty = false;
    int m_focusFrames = 0;
    double m_lastWorkingCheck = 0.0;

    Folder m_root;
    bool m_treeDirty = true;
    std::string m_folder;
    std::vector<std::string> m_history;
    std::string m_filter;
    std::vector<Row> m_rows;
    bool m_rowsDirty = true;
    int m_sortColumn = -1;
    bool m_sortAscending = true;
    std::set<uint32_t> m_selected;
    std::set<std::string> m_selectedFolders;
    int m_anchorRow = -1;
    std::string m_revealFolder;

    uint32_t m_previewHash = 0;
    uint64_t m_previewStamp = 0;
    uint64_t m_previewLoadedStamp = UINT64_MAX;
    Bytes m_preview;
    std::string m_previewText;
    std::vector<std::pair<uint32_t, size_t>> m_previewChunks;
    std::string m_previewError;

    std::thread m_job;
    std::string m_jobTitle;
    std::atomic<uint64_t> m_jobDone{0};
    std::atomic<uint64_t> m_jobTotal{0};
    std::atomic<bool> m_jobCancel{false};
    std::atomic<bool> m_jobFinished{false};
    bool m_jobOk = false;
    std::string m_jobError;
    std::function<void()> m_jobDoneCallback;
};

}
