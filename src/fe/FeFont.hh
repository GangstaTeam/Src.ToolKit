#pragma once

#include "p3d/Chunk.hh"
#include "render/GpuTexture.hh"

#include <d3d9.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tk::fe
{

enum EFeFontValue : uint32_t
{
    kPictureGlyph = 0x80000000u,
};

class TextureFont
{
public:
    struct Glyph
    {
        uint32_t texture = 0;
        bool picture = false;
        float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
        float leftBearing = 0, rightBearing = 0, width = 0, advance = 0;
        uint16_t code = 0;
        int16_t verticalOffset = 0;
        uint32_t kernIndex = 0, kernCount = 0;
    };

    struct Quad
    {
        IDirect3DTexture9* texture = nullptr;
        float x0, y0, x1, y1;
        float u0, v0, u1, v1;
    };

    bool Load(IDirect3DDevice9* device, const Chunk& chunk);
    const std::string& Name() const { return m_name; }
    float FontHeight() const { return m_fontHeight; }
    float BaseLine() const { return m_baseLine; }

    const Glyph* Find(char16_t code) const;
    const Glyph& Missing() const { return m_glyphs[m_missing]; }
    int Kern(const Glyph& glyph, int previous) const;

    float TextWidth(const std::u16string& text) const;
    void LayoutLine(const std::u16string& text, float x, float y, float scale, std::vector<Quad>& out) const;
    void LayoutWrapped(const std::u16string& text,
        float x,
        float y,
        float scale,
        float boxWidth,
        int align,
        std::vector<Quad>& out) const;

private:
    void EmitGlyph(const Glyph& glyph, float penX, float y, float scale, int kern, std::vector<Quad>& out) const;

    std::string m_name;
    float m_fontSize = 0, m_fontWidth = 0, m_fontHeight = 0, m_baseLine = 0;
    std::vector<std::unique_ptr<GpuTexture>> m_textures;
    std::vector<Glyph> m_glyphs;
    std::vector<std::pair<uint16_t, int16_t>> m_kerning;
    size_t m_missing = 0;
};

}
