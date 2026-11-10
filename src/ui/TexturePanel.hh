#pragma once

#include "doc/Document.hh"
#include "render/GpuTexture.hh"

#include <d3d9.h>

#include <cstdint>

namespace tk
{

class TexturePanel
{
public:
    TexturePanel(Document& document, IDirect3DDevice9* device) : m_document(document), m_device(device) {}

    void Draw();

private:
    void Refresh(const Chunk* chunk);

    Document& m_document;
    IDirect3DDevice9* m_device;
    GpuTexture m_texture;
    const Chunk* m_cachedChunk = nullptr;
    uint64_t m_cachedRevision = 0;
    float m_zoom = 1.0f;
};

}
