#pragma once

#include "doc/Document.hh"
#include "render/MeshRenderer.hh"
#include "render/ModelResolver.hh"
#include "render/OrbitCamera.hh"
#include "render/RenderTarget.hh"
#include "render/SkeletonPose.hh"
#include "render/TextureLibrary.hh"

#include <d3d9.h>
#include <imgui.h>

#include <cstdint>
#include <vector>

namespace tk
{

class ViewportPanel
{
public:
    ViewportPanel(Document& document, IDirect3DDevice9* device) : m_document(document), m_device(device) {}

    void Draw();
    void OnDeviceLost() { m_target.Release(); }
    void RequestFocus() { m_focusFrames = 3; }

private:
    void Refresh();
    void DrawToolbar();
    void DrawView();
    void HandleInput();
    void Render(int width, int height);
    void DrawSkeletonOverlay(ImVec2 origin, ImVec2 size);
    void PickJoint(ImVec2 origin, ImVec2 size);

    Document& m_document;
    IDirect3DDevice9* m_device;
    MeshRenderer m_renderer;
    TextureLibrary m_textures;
    RenderTarget m_target;
    OrbitCamera m_camera;
    RenderOptions m_options;
    MeshScene m_scene;
    ModelSelection m_model;
    std::vector<JointPose> m_joints;
    const Chunk* m_lastSelection = nullptr;
    uint64_t m_meshRevision = 0;
    uint64_t m_textureRevision = 0;
    bool m_lastWholeFile = false;
    bool m_wholeFile = false;
    bool m_showSkeleton = true;
    bool m_showNames = true;
    int m_focusFrames = 0;
};

}
