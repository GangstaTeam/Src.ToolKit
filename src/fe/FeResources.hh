#pragma once

#include "p3d/Chunk.hh"
#include "render/GpuTexture.hh"

#include <d3d9.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace tk::fe
{

struct SpriteTile
{
    IDirect3DTexture9* texture = nullptr;
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
};

struct Sprite
{
    int nativeX = 640;
    int nativeY = 480;
    int width = 0;
    int height = 0;
    std::vector<SpriteTile> tiles;
};

class Resources
{
public:
    void Clear();
    void AddFrom(IDirect3DDevice9* device, const Chunk& root);

    const Sprite* FindSprite(const std::string& name) const;
    const GpuTexture* FindTexture(const std::string& name) const;

private:
    void Scan(IDirect3DDevice9* device, const Chunk& chunk);
    void AddSprite(IDirect3DDevice9* device, const Chunk& chunk);
    GpuTexture* Upload(IDirect3DDevice9* device, const Chunk& image);

    std::vector<std::unique_ptr<GpuTexture>> m_gpu;
    std::map<std::string, Sprite> m_sprites;
    std::map<std::string, GpuTexture*> m_textures;
};

}
