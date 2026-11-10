#pragma once

#include <cstdint>

#include "doc/Document.hh"
#include "fe/FeFont.hh"
#include "fe/FeImageIndex.hh"
#include "fe/FeModel.hh"
#include "fe/FeRender.hh"
#include "fe/FeResources.hh"
#include "fe/FeTextBible.hh"
#include "p3d/Schema.hh"
#include "ui/ImageBrowser.hh"
#include "ui/PropertiesPanel.hh"

#include <d3d9.h>
#include <imgui.h>
#include <windows.h>

#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace tk
{

enum EFrontendWorkspaceColour : uint32_t
{
    kSelection = 0xFF40C4FF,
    kHover = 0x78FFFFFF,
    kBoundsColour = 0x46FFC878,
};

#define FRONTENDWORKSPACEAUTHORING_ROUND_CORNER "roundrectanglecorner.tga"

enum EFrontendWorkspaceAuthoring : int
{
    kSpriteSize = 128,
    kTextWidth = 240,
    kTextHeight = 24,
    kBoxWidth = 240,
    kBoxHeight = 48,
};

enum EFrontendWorkspaceAuthoringValue : uint32_t
{
    kBoxColour = 0xC8000000,
    kBoxRadius = 6,
};

class FrontendWorkspace
{
public:
    FrontendWorkspace(Document& document, const Schema& schema, IDirect3DDevice9* device, HWND owner);

    static const char* ProjectWindow() { return "Project"; }
    static const char* ScreenWindow() { return "Screen"; }
    static const char* ElementWindow() { return "Element"; }

    void Draw();
    void OnDeviceLost();
    void RegisterSettingsHandler();
    bool HasProject() const { return m_project.root != nullptr; }
    void SetGameFolder(const std::filesystem::path& folder);
    const std::filesystem::path& GameFolder() const { return m_gameFolder; }
    void RequestNewProject() { m_newProjectRequested = true; }

private:
    struct View
    {
        ImVec2 origin;
        float zoom = 1.0f;
        bool inGame = false;
        ImVec2 ToScreen(float x, float y) const;
        ImVec2 ToPage(ImVec2 screen) const;
    };

    void Refresh();
    void LoadGameResources();
    void RevealSelection();
    void LoadUsedImages();

    void DrawProject();
    void DrawNode(fe::Node& node);
    void DrawScreen();
    void DrawToolbar();
    void DrawGameFolderButton();
    void ChooseGameFolder();
    void DrawAddBar();
    void DrawElementExtras();
    void DrawNewProjectPopup();
    void DrawGameFilesNote();
    void DrawImageBrowser();
    void DrawAlignControls();
    void DrawCanvas();
    void DrawItems(ImDrawList* list, const View& view, const std::vector<fe::DrawItem>& items);
    void DrawOverlay(ImDrawList* list, const View& view, const std::vector<fe::DrawItem>& items);
    void HandleCanvasInput(const View& view, const std::vector<fe::DrawItem>& items, bool hovered);

    fe::Node* NodeFor(const Chunk* chunk) const;
    fe::Node* PageOf(fe::Node* node) const;
    void Select(fe::Node* node);
    void Move(fe::Node& node, int dx, int dy);
    fe::Node* CurrentPage() const;
    Chunk* TargetLayer(size_t& index) const;
    void AddElement(std::unique_ptr<Chunk> chunk);
    void AddLayer();
    void AddPage();
    void DeleteSelection();
    void AlignSelection(int horizontal, int vertical);
    std::vector<std::string> UsedImages() const;
    std::vector<const fe::Node*> ShownPages() const;

    Document& m_document;
    const Schema& m_schema;
    IDirect3DDevice9* m_device;
    HWND m_owner;
    PropertiesPanel m_element;

    fe::Project m_project;
    uint64_t m_revision = UINT64_MAX;
    std::filesystem::path m_documentPath;
    std::map<const Chunk*, fe::Node*> m_nodes;
    fe::Resources m_documentResources;
    std::vector<std::unique_ptr<fe::TextureFont>> m_documentFonts;

    std::filesystem::path m_gameFolder;
    std::filesystem::path m_loadedGameFolder;
    char m_language = 'E';
    fe::Resources m_gameResources;
    std::vector<std::unique_ptr<fe::TextureFont>> m_gameFonts;
    fe::TextBibles m_bibles;
    fe::ImageIndex m_images;
    std::set<std::filesystem::path> m_loadedImageFiles;
    ImageBrowser m_imageBrowser;
    std::string m_resourceStatus;

    const fe::Node* m_page = nullptr;
    const fe::Node* m_screen = nullptr;
    std::set<const Chunk*> m_hidden;
    const fe::Node* m_revealed = nullptr;
    bool m_revealPending = false;

    float m_zoom = 1.0f;
    bool m_fit = true;
    ImVec2 m_pan = ImVec2(0, 0);
    bool m_inGame = false;
    bool m_showBounds = false;
    bool m_gameOrder = true;
    const fe::Node* m_hover = nullptr;
    bool m_dragging = false;
    ImVec2 m_dragStart;
    ImVec2 m_dragOffset = ImVec2(0, 0);
    ImVec2 m_mousePage = ImVec2(0, 0);
    bool m_hasSelectionBounds = false;
    ImVec2 m_selectionMin = ImVec2(0, 0);
    ImVec2 m_selectionMax = ImVec2(0, 0);

    bool m_newProjectRequested = false;
    char m_newProjectName[64] = "mp_loading";
    char m_imageFilter[64] = "";
    const Chunk* m_editedChunk = nullptr;
    char m_editText[512] = "";
    char m_editName[128] = "";
};

}
