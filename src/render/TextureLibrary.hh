#pragma once

#include "p3d/Chunk.hh"
#include "render/GpuTexture.hh"

#include <d3d9.h>

#include <map>
#include <memory>
#include <string>

namespace tk
{

class TextureLibrary
{
public:
    void Rebuild(IDirect3DDevice9* device, const Chunk& root);
    void Clear();
    void Assign(IDirect3DDevice9* device, const std::string& shader, const Image& image);
    IDirect3DTexture9* ForShader(const std::string& shader) const;

private:
    void Scan(const Chunk& chunk);
    void AddTexture(IDirect3DDevice9* device, const Chunk& texture);

    std::map<std::string, std::string> m_shaderTextures;
    std::map<std::string, const Chunk*> m_textureChunks;
    std::map<std::string, std::unique_ptr<GpuTexture>> m_textures;
};

const Chunk* FindImageData(const Chunk* chunk);
Bytes EncodedImage(const Chunk& imageData);
std::optional<Image> DecodeImageData(const Chunk& imageData);

}
