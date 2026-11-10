#pragma once

#include "render/ImageDecoder.hh"

#include <d3d9.h>

namespace tk
{

class GpuTexture
{
public:
    GpuTexture() = default;
    GpuTexture(const GpuTexture&) = delete;
    GpuTexture& operator=(const GpuTexture&) = delete;
    ~GpuTexture() { Release(); }

    bool Upload(IDirect3DDevice9* device, const Image& image);
    void Release();
    IDirect3DTexture9* Get() const { return m_texture; }
    uint32_t Width() const { return m_width; }
    uint32_t Height() const { return m_height; }

private:
    IDirect3DTexture9* m_texture = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

}
