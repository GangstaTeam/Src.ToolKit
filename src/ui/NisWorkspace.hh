#pragma once

#include "link/GameLink.hh"
#include "nis/FscTree.hh"
#include "nis/NisCatalog.hh"
#include "nis/NisPackage.hh"
#include "nis/NisScene.hh"

#include <imgui.h>
#include <windows.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace tk
{

enum ENisTimelineColour : uint32_t
{
    kGridColour = 0x12FFFFFF,
    kRulerText = 0xC8C8C8C8,
    kEndColour = 0xA01F16C8,
    kCursorColour = 0x5AFFFFFF,
    kSelectedOutline = 0xFF40C4FF,
    kSubjectColour = 0xFFAFAAAA,
    kTimelineWarning = 0xFF4CB2F2,
};

enum ENisTimeline : int
{
    kMinimumFrames = 150,
};

enum ENisWorkspaceSize : size_t
{
    kUndoLimit = 200,
};

ImU32 NisCategoryColour(nis::Category category, int alpha = 255);

struct NisActions
{
    std::function<void(const std::string&)> error;
    std::function<std::filesystem::path()> gameFolder;
    std::function<void(const std::filesystem::path&)> setGameFolder;
    std::function<GameLink*()> link;
};

class NisWorkspace
{
public:
    NisWorkspace(HWND owner, NisActions actions);

    static bool Accepts(const std::filesystem::path& path);
    static const char* SceneWindow() { return "Scene"; }
    static const char* TimelineWindow() { return "Timeline"; }
    static const char* TrackWindow() { return "Track"; }
    static const char* ProblemsWindow() { return "Problems"; }
    static const char* SourceWindow() { return "FSC Source"; }

    void Draw();
    bool Open(const std::filesystem::path& path);
    void New();
    void Save();
    void SaveAs();
    bool ConfirmDiscard();

    bool HasScene() const { return m_open; }
    bool Dirty() const { return m_open && m_undo.size() != m_savedDepth; }
    const std::filesystem::path& Path() const { return m_path; }
    std::string Title() const;

    bool CanUndo() const { return !m_undo.empty(); }
    bool CanRedo() const { return !m_redo.empty(); }
    void Undo();
    void Redo();
    bool HasTrack() const { return SelectedTrack() != nullptr; }
    void DuplicateTrack();
    void DeleteTrack();
    void MoveTrack(int direction);

private:
    void BeginChange(bool discrete = false);
    void Refresh();
    void Reset(std::vector<fsc::Node> roots, const std::filesystem::path& path);
    bool SaveTo(const std::filesystem::path& path);
    void ReloadAssets();
    std::filesystem::path GameFolder() const;
    void SetAssetsFrom(const std::string& scene);
    void OpenBuild();
    void ClampSelection();
    void SelectNode(const nis::NodePath& path, int track = -1);

    fsc::Node* SelectedNode();
    const fsc::Node* SelectedNode() const;
    fsc::Node* SelectedTrack();
    const fsc::Node* SelectedTrack() const;
    std::vector<std::string> Suggestions(nis::Ref ref) const;

    void AddTrack(const nis::TrackSpec& spec);
    void AddAnimationTrack(const nis::AnimationInfo& animation);
    void AddNode();
    void DuplicateNode(const nis::NodePath& path);
    void DeleteNode(const nis::NodePath& path);
    void RenameNode(const nis::NodePath& path, const std::string& name);

    void DrawEmpty(const char* window);
    void DrawScene();
    void DrawNodes();
    void DrawCast();
    void DrawAnimations();
    void DrawTimeline();
    void DrawTimelineToolbar();
    void DrawAddTrackMenu();
    void DrawTimelineRows(int frames);
    void DrawTrack();
    void DrawField(fsc::Node& track, const nis::TrackSpec& spec, const nis::FieldSpec& field);
    void DrawRawField(fsc::Node& track, size_t index);
    bool DrawSuggestions(const char* id, nis::Ref ref, std::string& value);
    void DrawRawEditor(fsc::Node& track);
    void DrawProblems();
    void DrawSource();
    void DrawNewScenePopup();
    void DrawRenamePopup();
    bool DrawSceneChooser(const char* id, std::string& scene);
    bool DrawGameFolder();
    void DrawBuildPopup();
    void DrawBuildResult();
    void DrawPlayInGame();
    GameLink* LiveGame() const;
    void HandleKeys();

    HWND m_owner;
    NisActions m_actions;

    std::vector<fsc::Node> m_roots;
    std::filesystem::path m_path;
    bool m_open = false;
    enum ESaved : size_t
    {
        kNeverSaved = SIZE_MAX,
    };
    size_t m_savedDepth = 0;
    std::vector<std::vector<fsc::Node>> m_undo;
    std::vector<std::vector<fsc::Node>> m_redo;
    bool m_editOpen = false;
    uint64_t m_revision = 0;

    nis::NodePath m_node;
    int m_track = -1;
    bool m_revealTrack = false;

    nis::SceneAssets m_assets;
    std::string m_assetsFrom;
    std::vector<std::string> m_sceneFolders;
    std::filesystem::path m_detectedFor;
    std::string m_chooserFilter;
    uint64_t m_cacheRevision = UINT64_MAX;
    std::vector<nis::SceneNode> m_nodes;
    nis::Cast m_cast;
    std::vector<nis::Issue> m_issues;
    std::string m_source;
    int m_sceneFrames = 0;

    float m_zoom = 3.0f;
    bool m_timelineFocused = false;
    int m_hoverFrame = -1;
    bool m_dragging = false;
    int m_dragTrack = -1;
    int m_dragFrame = 0;
    float m_dragX = 0.0f;

    std::string m_sceneName;
    std::string m_newName;
    bool m_openNewPopup = false;
    nis::NodePath m_renamePath;
    std::string m_renameText;
    bool m_openRenamePopup = false;
    bool m_openBuildPopup = false;
    bool m_buildDone = false;
    nis::BuildOptions m_build;
    nis::BuildPlan m_buildPlan;
    std::string m_buildKey;
    std::string m_raw;
    std::string m_rawError;
    uint64_t m_rawRevision = UINT64_MAX;
    int m_rawTrack = -1;
};

}
