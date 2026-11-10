#include "fe/FeFont.hh"

#include "core/ByteReader.hh"
#include "p3d/ChunkIds.hh"
#include "render/TextureLibrary.hh"

#include <algorithm>
#include <map>

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

}

bool TextureFont::Load(IDirect3DDevice9* device, const Chunk& chunk)
{
    ByteReader r(chunk.Payload());
    r.U32();
    m_name = PString(r);
    PString(r);
    m_fontSize = r.F32();
    m_fontWidth = r.F32();
    m_fontHeight = r.F32();
    m_baseLine = r.F32();
    r.U32();

    if (r.Failed())
    {
        return false;
    }

    std::map<uint16_t, int16_t> pictureOffsets;
    std::vector<std::tuple<uint16_t, uint16_t, int32_t>> kerning;

    for (size_t i = 0; i < chunk.ChildCount(); ++i)
    {
        const Chunk& child = *chunk.Child(i);
        ByteReader c(child.Payload());

        if (child.Id() == chunk::Texture)
        {
            auto gpu = std::make_unique<GpuTexture>();
            const Chunk* data = FindImageData(&child);

            if (data)
            {
                if (auto image = DecodeImageData(*data))
                {
                    gpu->Upload(device, *image);
                }
            }

            m_textures.push_back(std::move(gpu));
        }
        else if (child.Id() == chunk::FontGlyphs)
        {
            uint32_t count = c.U32();

            for (uint32_t g = 0; g < count && !c.Failed(); ++g)
            {
                Glyph glyph;
                uint32_t texture = c.U32();
                glyph.picture = (texture & kPictureGlyph) != 0;
                glyph.texture = texture & ~kPictureGlyph;
                glyph.u0 = c.F32();
                glyph.v0 = c.F32();
                glyph.u1 = c.F32();
                glyph.v1 = c.F32();
                glyph.leftBearing = c.F32();
                glyph.rightBearing = c.F32();
                glyph.width = c.F32();
                glyph.advance = c.F32();
                glyph.code = static_cast<uint16_t>(c.U32());
                m_glyphs.push_back(glyph);
            }
        }
        else if (child.Id() == chunk::FontPictureOffsets)
        {
            uint32_t count = c.U32();

            for (uint32_t p = 0; p < count && !c.Failed(); ++p)
            {
                uint16_t code = static_cast<uint16_t>(c.U32());
                pictureOffsets[code] = static_cast<int16_t>(c.U32());
            }
        }
        else if (child.Id() == chunk::FontKerning)
        {
            uint32_t count = c.U32();

            for (uint32_t k = 0; k < count && !c.Failed(); ++k)
            {
                uint16_t previous = static_cast<uint16_t>(c.U32());
                uint16_t current = static_cast<uint16_t>(c.U32());
                int32_t kern = static_cast<int32_t>(c.U32());
                c.U32();
                kerning.emplace_back(previous, current, kern);
            }
        }
    }

    if (m_glyphs.empty())
    {
        return false;
    }

    std::sort(m_glyphs.begin(),
        m_glyphs.end(),
        [](const Glyph& a, const Glyph& b)
        {
            return a.code < b.code;
        });
    for (Glyph& glyph : m_glyphs)
    {
        auto it = pictureOffsets.find(glyph.code);

        if (it != pictureOffsets.end())
        {
            glyph.verticalOffset = it->second;
        }
    }

    for (const auto& [previous, current, kern] : kerning)
    {
        auto glyph = std::lower_bound(m_glyphs.begin(),
            m_glyphs.end(),
            current,
            [](const Glyph& g, uint16_t code)
            {
                return g.code < code;
            });
        if (glyph == m_glyphs.end() || glyph->code != current)
        {
            continue;
        }

        if (glyph->kernCount == 0)
        {
            glyph->kernIndex = static_cast<uint32_t>(m_kerning.size());
        }

        ++glyph->kernCount;
        m_kerning.emplace_back(previous, static_cast<int16_t>(kern));
    }

    auto missing = std::lower_bound(m_glyphs.begin(),
        m_glyphs.end(),
        uint16_t(0),
        [](const Glyph& g, uint16_t code)
        {
            return g.code < code;
        });
    m_missing = missing != m_glyphs.end() && missing->code == 0 ? size_t(missing - m_glyphs.begin()) : 0;
    return true;
}

const TextureFont::Glyph* TextureFont::Find(char16_t code) const
{
    auto it = std::lower_bound(m_glyphs.begin(),
        m_glyphs.end(),
        static_cast<uint16_t>(code),
        [](const Glyph& g, uint16_t c)
        {
            return g.code < c;
        });
    if (it == m_glyphs.end() || it->code != code || it == m_glyphs.begin() + m_missing)
    {
        return nullptr;
    }

    return &*it;
}

int TextureFont::Kern(const Glyph& glyph, int previous) const
{
    for (uint32_t i = 0; i < glyph.kernCount; ++i)
    {
        const auto& [prev, kern] = m_kerning[glyph.kernIndex + i];

        if (prev == previous)
        {
            return kern;
        }
    }

    return 0;
}

float TextureFont::TextWidth(const std::u16string& text) const
{
    float widest = 0, line = 0;

    for (char16_t c : text)
    {
        if (c == u'\n')
        {
            widest = std::max(widest, line);
            line = 0;
            continue;
        }

        const Glyph* glyph = Find(c);
        line += (glyph ? *glyph : Missing()).advance;
    }

    return std::max(widest, line);
}

void TextureFont::EmitGlyph(const Glyph& glyph, float penX, float y, float scale, int kern, std::vector<Quad>& out)
    const
{
    if (glyph.texture >= m_textures.size() || !m_textures[glyph.texture]->Get())
    {
        return;
    }

    const GpuTexture& texture = *m_textures[glyph.texture];
    Quad quad;
    quad.texture = texture.Get();
    quad.x0 = penX + (kern + glyph.leftBearing) * scale;
    quad.x1 = quad.x0 + glyph.width * scale;

    if (glyph.picture)
    {
        float height = (glyph.v1 - glyph.v0) * float(texture.Height());
        quad.y0 = y - glyph.verticalOffset * scale;
        quad.y1 = quad.y0 + height * scale;
    }
    else
    {
        quad.y0 = y + (m_baseLine - m_fontHeight) * scale;
        quad.y1 = quad.y0 + m_fontHeight * scale;
    }

    quad.u0 = glyph.u0;
    quad.u1 = glyph.u1;
    quad.v0 = 1.0f - glyph.v1;
    quad.v1 = 1.0f - glyph.v0;
    out.push_back(quad);
}

void TextureFont::LayoutLine(const std::u16string& text, float x, float y, float scale, std::vector<Quad>& out) const
{
    float pen = x;
    int previous = -1;

    for (char16_t c : text)
    {
        if (c == u'\n')
        {
            break;
        }

        const Glyph* found = Find(c);
        const Glyph& glyph = found ? *found : Missing();
        int kern = Kern(glyph, previous);

        if (glyph.width > 0)
        {
            EmitGlyph(glyph, pen, y, scale, kern, out);
        }

        pen += (kern + glyph.advance) * scale;
        previous = c;
    }
}

void TextureFont::LayoutWrapped(const std::u16string& text,
    float x,
    float y,
    float scale,
    float boxWidth,
    int align,
    std::vector<Quad>& out) const
{
    size_t start = 0;

    while (start < text.size())
    {
        while (start < text.size() && (text[start] == u' ' || text[start] == u'\t'))
        {
            ++start;
        }

        float width = 0, widthAtSpace = 0;
        size_t end = start, lastSpace = std::u16string::npos;
        int previous = -1;

        for (; end < text.size() && text[end] != u'\n'; ++end)
        {
            const Glyph* found = Find(text[end]);
            const Glyph& glyph = found ? *found : Missing();
            float step = (Kern(glyph, previous) + glyph.advance) * scale;

            if (width + step > boxWidth && end > start)
            {
                break;
            }

            width += step;

            if (text[end] == u' ')
            {
                lastSpace = end;
                widthAtSpace = width;
            }

            previous = text[end];
        }

        size_t next = end;

        if (end < text.size() && text[end] != u'\n' && lastSpace != std::u16string::npos)
        {
            end = lastSpace;
            next = lastSpace + 1;
            width = widthAtSpace;
        }

        if (next < text.size() && text[next] == u'\n' && next == end)
        {
            ++next;
        }

        float lineX = align == 2 ? x + (boxWidth - width) / 2 : align == 3 ? x + boxWidth - width : x;
        LayoutLine(text.substr(start, end - start), lineX, y, scale, out);
        y -= m_fontHeight * scale;

        if (next <= start)
        {
            break;
        }

        start = next;
    }
}

}
