#pragma once

#include "doc/Document.hh"
#include "render/FrontCamera.hh"
#include "render/MeshRenderer.hh"
#include "render/OrbitCamera.hh"
#include "render/RenderTarget.hh"
#include "render/SkeletonPose.hh"
#include "render/TextureLibrary.hh"
#include "rig/ImportSession.hh"
#include "rig/WeightPaint.hh"

#include <d3d9.h>
#include <imgui.h>
#include <windows.h>

#include <cstdint>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <vector>

namespace tk
{

enum ECharacterImportMarkers : int
{
    kPaletteRows = 6,
};

class CharacterImportPanel
{
public:
    CharacterImportPanel(Document& document, IDirect3DDevice9* device, HWND owner)
        : m_document(document), m_device(device), m_owner(owner)
    {
    }

    static bool Accepts(const std::filesystem::path& path);

    void Open();
    void OpenModel(const std::filesystem::path& path);
    void Draw();
    void OnDeviceLost() { m_target.Release(); }

private:
    enum class Step
    {
        Upload,
        Orient,
        Skeleton,
        Rigging,
        Review
    };

    struct RigOutcome
    {
        std::optional<RiggedCharacter> rig;
        std::string error;
    };

    void DrawStepBar();
    void DrawInfo();
    void DrawUploadInfo();
    void DrawOrientInfo();
    void DrawSkeletonInfo();
    void DrawBoneMapping();
    void DrawRiggingInfo();
    void DrawReviewInfo();
    void DrawFooter();
    bool CanGoNext() const;
    void GoNext();
    void GoBack();

    void PickModel();
    void PickTexture();
    void StartLoad(const std::filesystem::path& path);
    void PollConversion();
    void StartRigging();
    void PollRigging();
    void ApplyToDocument();
    void SaveAsNewCharacter();
    GameSkeleton BaseSkeleton();

    void Refresh();
    void RefreshPose();
    void DrawView();
    void RenderView(int width, int height);
    void HandleCameraInput(ImVec2 size);
    void DrawOrientButtons(ImVec2 origin, ImVec2 size);
    void DrawJointNames(ImVec2 origin, ImVec2 size);
    void DrawUploadHint(ImVec2 origin, ImVec2 size);
    bool ProjectPoint(Vec3 point, ImVec2 size, float& x, float& y) const;
    bool FrontView() const;
    bool MarkerView() const;

    void DrawMarkerPalette(ImVec2 origin, ImVec2 size);
    void HandleMarkerInput(ImVec2 origin, ImVec2 size);
    void DrawMarkers(ImVec2 origin, ImVec2 size);
    ImVec2 PaletteSlot(Marker marker, ImVec2 origin, ImVec2 size) const;
    void DrawMagnifier(ImVec2 origin, ImVec2 size);

    bool PaintMode() const;
    void DrawPaintControls();
    void HandlePaintInput(ImVec2 origin, ImVec2 size);
    void DrawBrush(ImVec2 origin, ImVec2 size);
    void RefreshPaintColours();
    std::vector<BrushHit> CollectBrushHits(ImVec2 origin, ImVec2 size, float radius) const;

    Document& m_document;
    IDirect3DDevice9* m_device;
    HWND m_owner;
    ImportSession m_session;
    MeshRenderer m_renderer;
    TextureLibrary m_textures;
    RenderTarget m_target;
    OrbitCamera m_orbit;
    FrontCamera m_front;
    RenderOptions m_options;
    MeshScene m_scene;
    WeightPainter m_painter;
    uint64_t m_painterRevision = 0;
    std::vector<JointPose> m_joints;
    std::future<std::string> m_conversion;
    std::future<RigOutcome> m_rigging;
    std::filesystem::path m_pendingPath;
    std::string m_status;
    Step m_step = Step::Upload;
    uint64_t m_sceneRevision = 0;
    uint64_t m_framedModel = 0;
    uint64_t m_textureRevision = 0;
    int m_dragMarker = -1;
    int m_focusFrames = 0;
    int m_textureSize = 512;
    int m_paintJoint = -1;
    int m_brushMode = 0;
    float m_brushRadius = 40.0f;
    float m_brushStrength = 0.5f;
    bool m_paint = false;
    char m_newName[64] = "MCP_";
    float m_posePhase = 0.0f;
    float m_riggingTime = 0.0f;
    bool m_open = false;
    bool m_dockPending = true;
    bool m_symmetry = true;
    bool m_boneColours = false;
    bool m_testPose = true;
    bool m_sceneIsPosed = false;
    bool m_showNames = false;
};

}
