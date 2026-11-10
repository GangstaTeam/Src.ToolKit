#include "fe/FeResources.hh"

#include "core/ByteReader.hh"
#include "p3d/ChunkIds.hh"
#include "render/TextureLibrary.hh"

#include <algorithm>
#include <cctype>

namespace tk::fe
{

namespace
{

std::string PString(ByteReader& reader)
{
    uint8_t length = reader.U8();
    Bytes bytes = reader.Take(length);
    std::string text(bytes.begin(), bytes.end());
    size_t nul = text.find('\0');

    if (nul != std::string::npos)
    {
        text.resize(nul);
    }

    return text;
}

std::string Key(std::string name)
{
    for (char& c : name)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return name;
}

std::string LeadingName(const Chunk& chunk)
{
    ByteReader reader(chunk.Payload());
    return PString(reader);
}

}

void Resources::Clear()
{
    m_sprites.clear();
    m_textures.clear();
    m_gpu.clear();
}

GpuTexture* Resources::Upload(IDirect3DDevice9* device, const Chunk& image)
{
    const Chunk* data = FindImageData(&image);

    if (!data)
    {
        return nullptr;
    }

    auto decoded = DecodeImageData(*data);

    if (!decoded)
    {
        return nullptr;
    }

    auto gpu = std::make_unique<GpuTexture>();

    if (!gpu->Upload(device, *decoded))
    {
        return nullptr;
    }

    m_gpu.push_back(std::move(gpu));
    return m_gpu.back().get();
}

void Resources::AddFrom(IDirect3DDevice9* device, const Chunk& root)
{
    Scan(device, root);
}

void Resources::Scan(IDirect3DDevice9* device, const Chunk& chunk)
{
    for (size_t i = 0; i < chunk.ChildCount(); ++i)
    {
        const Chunk& child = *chunk.Child(i);

        if (child.Id() == chunk::Sprite)
        {
            AddSprite(device, child);
        }
        else if (child.Id() == chunk::Texture)
        {
            std::string key = Key(LeadingName(child));

            if (!m_textures.count(key))
            {
                if (GpuTexture* gpu = Upload(device, child))
                {
                    m_textures[key] = gpu;
                }
            }
        }
        else
        {
            Scan(device, child);
        }
    }
}

void Resources::AddSprite(IDirect3DDevice9* device, const Chunk& chunk)
{
    ByteReader r(chunk.Payload());
    std::string name = PString(r);
    Sprite sprite;
    sprite.nativeX = static_cast<int>(r.U32());
    sprite.nativeY = static_cast<int>(r.U32());
    PString(r);
    int width = static_cast<int>(r.U32());
    int height = static_cast<int>(r.U32());
    int count = static_cast<int>(r.U32());
    int border = static_cast<int>(r.U32());

    if (sprite.nativeX <= 0)
    {
        sprite.nativeX = 640;
    }

    std::vector<GpuTexture*> images;

    for (size_t i = 0; i < chunk.ChildCount(); ++i)
    {
        if (chunk.Child(i)->Id() == chunk::Image)
        {
            images.push_back(Upload(device, *chunk.Child(i)));
        }
    }

    if (images.empty() || !images[0])
    {
        return;
    }

    (void)count;
    (void)border;

    if (width <= 0)
    {
        width = static_cast<int>(images[0]->Width());
    }

    if (height <= 0)
    {
        height = static_cast<int>(images[0]->Height());
    }

    sprite.width = width;
    sprite.height = height;
    int b = images[0]->Width() >= uint32_t(width) && images[0]->Height() >= uint32_t(height) ? 0 : 1;
    size_t index = 0;
    int y = -b;

    while (y < height - 1 && index < images.size())
    {
        int x = -b;
        int newY = 0;

        while (x < width - 1 && index < images.size())
        {
            GpuTexture* image = images[index++];

            if (!image)
            {
                return;
            }

            int newX = static_cast<int>(image->Width());
            newY = static_cast<int>(image->Height());
            int left = x + b, top = y + b;
            int tileW = newX > width ? width : newX - 2 * b;
            int tileH = newY > height ? height : newY - 2 * b;
            SpriteTile tile;
            tile.texture = image->Get();
            tile.x0 = float(left);
            tile.x1 = float(left + tileW);
            tile.y0 = float(height - (top + tileH));
            tile.y1 = float(height - top);
            tile.u0 = (b + 0.5f) / newX;
            tile.u1 = (b + tileW + 0.5f) / newX;
            tile.v0 = (b + 0.25f) / newY;
            tile.v1 = (b + tileH + 0.25f) / newY;
            sprite.tiles.push_back(tile);
            x += newX - 2 * b;
        }

        if (newY == 0)
        {
            break;
        }

        y += newY - 2 * b;
    }

    m_sprites[Key(name)] = std::move(sprite);
}

const Sprite* Resources::FindSprite(const std::string& name) const
{
    auto it = m_sprites.find(Key(name));
    return it == m_sprites.end() ? nullptr : &it->second;
}

const GpuTexture* Resources::FindTexture(const std::string& name) const
{
    auto it = m_textures.find(Key(name));
    return it == m_textures.end() ? nullptr : it->second;
}

}
