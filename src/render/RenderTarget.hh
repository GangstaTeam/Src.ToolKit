#pragma once

#include <d3d9.h>

namespace tk
{

class RenderTarget
{
public:
    RenderTarget() = default;
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    ~RenderTarget() { Release(); }

    bool Ensure(IDirect3DDevice9* device, int width, int height);
    void Release();
    bool Begin(IDirect3DDevice9* device);
    void End(IDirect3DDevice9* device);

    IDirect3DTexture9* Texture() const { return m_texture; }
    int Width() const { return m_width; }
    int Height() const { return m_height; }

private:
    IDirect3DTexture9* m_texture = nullptr;
    IDirect3DSurface9* m_surface = nullptr;
    IDirect3DSurface9* m_depth = nullptr;
    IDirect3DSurface9* m_savedColour = nullptr;
    IDirect3DSurface9* m_savedDepth = nullptr;
    int m_width = 0;
    int m_height = 0;
};

}
