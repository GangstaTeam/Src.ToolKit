#include "render/TextureLibrary.hh"

#include "core/ByteReader.hh"
#include "p3d/ChunkIds.hh"

#include <algorithm>
#include <cstring>

namespace tk
{

namespace
{

std::string ReadName(ByteReader& reader)
{
    Bytes chars = reader.Take(reader.U8());
    std::string name(chars.begin(), chars.end());
    name.resize(std::strlen(name.c_str()));
    return name;
}

std::string LeadingName(const Chunk& chunk)
{
    ByteReader reader(chunk.Payload());
    return ReadName(reader);
}

std::string ShaderTexture(const Chunk& shader)
{
    for (size_t i = 0; i < shader.ChildCount(); ++i)
    {
        const Chunk* param = shader.Child(i);

        if (param->Id() != chunk::ShaderTextureParam)
        {
            continue;
        }

        ByteReader reader(param->Payload());

        if (reader.U32() == 0x00584554u)
        {
            return ReadName(reader);
        }
    }

    return {};
}

}

const Chunk* FindImageData(const Chunk* chunk)
{
    if (!chunk)
    {
        return nullptr;
    }

    if (chunk->Id() == chunk::ImageData)
    {
        return chunk;
    }

    if (chunk->Id() != chunk::Texture && chunk->Id() != chunk::Image)
    {
        return nullptr;
    }

    for (size_t i = 0; i < chunk->ChildCount(); ++i)
    {
        if (const Chunk* found = FindImageData(chunk->Child(i)))
        {
            return found;
        }
    }

    return nullptr;
}

Bytes EncodedImage(const Chunk& imageData)
{
    ByteReader reader(imageData.Payload());
    uint32_t size = reader.U32();
    return reader.Take(size);
}

std::optional<Image> DecodeImageData(const Chunk& imageData)
{
    Bytes encoded = EncodedImage(imageData);
    std::optional<Image> image = DecodeImage(encoded);

    if (!image || encoded.size() < 4 || std::memcmp(encoded.data(), "DDS ", 4) != 0)
    {
        return image;
    }

    size_t row = static_cast<size_t>(image->width) * 4;

    for (uint32_t y = 0; y < image->height / 2; ++y)
    {
        std::swap_ranges(image->bgra.begin() + static_cast<std::ptrdiff_t>(y * row),
            image->bgra.begin() + static_cast<std::ptrdiff_t>((y + 1) * row),
            image->bgra.begin() + static_cast<std::ptrdiff_t>((image->height - 1 - y) * row));
    }

    return image;
}

void TextureLibrary::Clear()
{
    m_shaderTextures.clear();
    m_textureChunks.clear();
    m_textures.clear();
}

void TextureLibrary::Rebuild(IDirect3DDevice9* device, const Chunk& root)
{
    Clear();
    Scan(root);

    for (const auto& [name, chunk] : m_textureChunks)
    {
        AddTexture(device, *chunk);
    }
}

void TextureLibrary::Scan(const Chunk& chunk)
{
    if (chunk.Id() == chunk::Shader)
    {
        m_shaderTextures[LeadingName(chunk)] = ShaderTexture(chunk);
    }

    if (chunk.Id() == chunk::Texture)
    {
        m_textureChunks[LeadingName(chunk)] = &chunk;
    }

    for (size_t i = 0; i < chunk.ChildCount(); ++i)
    {
        Scan(*chunk.Child(i));
    }
}

void TextureLibrary::AddTexture(IDirect3DDevice9* device, const Chunk& texture)
{
    const Chunk* imageData = FindImageData(&texture);

    if (!imageData)
    {
        return;
    }

    auto image = DecodeImageData(*imageData);

    if (!image)
    {
        return;
    }

    auto gpu = std::make_unique<GpuTexture>();

    if (gpu->Upload(device, *image))
    {
        m_textures[LeadingName(texture)] = std::move(gpu);
    }
}

void TextureLibrary::Assign(IDirect3DDevice9* device, const std::string& shader, const Image& image)
{
    auto gpu = std::make_unique<GpuTexture>();

    if (!gpu->Upload(device, image))
    {
        return;
    }

    m_shaderTextures[shader] = shader;
    m_textures[shader] = std::move(gpu);
}

IDirect3DTexture9* TextureLibrary::ForShader(const std::string& shader) const
{
    auto textureName = m_shaderTextures.find(shader);

    if (textureName == m_shaderTextures.end())
    {
        return nullptr;
    }

    auto texture = m_textures.find(textureName->second);
    return texture == m_textures.end() ? nullptr : texture->second->Get();
}

}
