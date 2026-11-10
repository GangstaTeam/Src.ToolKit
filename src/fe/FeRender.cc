#include "fe/FeRender.hh"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <tuple>

namespace tk::fe
{

namespace
{

uint32_t WithAlpha(uint32_t colour, uint32_t alpha)
{
    return (colour & 0x00FFFFFFu) | (std::min<uint32_t>(alpha, 255) << 24);
}

void Bounds(DrawItem& item, float x0, float y0, float x1, float y1)
{
    item.minX = std::min(x0, x1);
    item.maxX = std::max(x0, x1);
    item.minY = std::min(y0, y1);
    item.maxY = std::max(y0, y1);
}

void Quad(std::vector<Vertex>& out,
    float x0,
    float y0,
    float x1,
    float y1,
    float u0,
    float v0,
    float u1,
    float v1,
    uint32_t colour)
{
    Vertex bl{x0, y0, u0, v1, colour}, br{x1, y0, u1, v1, colour}, tl{x0, y1, u0, v0, colour},
        tr{x1, y1, u1, v0, colour};
    out.insert(out.end(), {bl, tl, tr, bl, tr, br});
}

void Rotate(std::vector<Vertex>& vertices, size_t from, float cx, float cy, float degrees)
{
    if (std::fabs(degrees) <= 0.1f)
    {
        return;
    }

    float c = std::cos(degrees * 0.017453292f), s = std::sin(degrees * 0.017453292f);

    for (size_t i = from; i < vertices.size(); ++i)
    {
        float dx = vertices[i].x - cx, dy = vertices[i].y - cy;
        vertices[i].x = cx + dx * c - dy * s;
        vertices[i].y = cy + dx * s + dy * c;
    }
}

template <typename T>
const T* FindIn(const std::vector<const Resources*>& resources,
    const std::string& name,
    const T* (Resources::*find)(const std::string&) const)
{
    for (const Resources* r : resources)
    {
        if (const T* found = (r->*find)(name))
        {
            return found;
        }
    }

    return nullptr;
}

class Builder
{
public:
    explicit Builder(const RenderContext& context) : m_context(context) {}

    void Walk(const Node& node)
    {
        if (m_context.hidden.count(&node))
        {
            return;
        }

        if (node.type == Type::Layer && !node.visible)
        {
            return;
        }

        if (node.type == Type::Layer || node.type == Type::Group || node.type == Type::Page)
        {
            for (const auto& child : node.children)
            {
                Walk(*child);
            }

            return;
        }

        switch (node.type)
        {
        case Type::RoundedRectangle:
            RoundedRectangle(node);
            break;
        case Type::Polygon:
            Polygon(node);
            break;
        case Type::Sprite:
            Sprite(node);
            break;
        case Type::Pure3dObject:
            Pure3dObject(node);
            break;
        case Type::Text:
            Text(node);
            break;
        default:
            break;
        }
    }

    std::vector<DrawItem> Finish()
    {
        if (!m_context.gameOrder)
        {
            return std::move(m_tree);
        }

        std::vector<DrawItem> out;

        for (auto& bucket : m_buckets)
        {
            for (auto& item : bucket)
            {
                out.push_back(std::move(item));
            }
        }

        return out;
    }

private:
    void Emit(DrawItem item, int bucket)
    {
        if (m_context.gameOrder)
        {
            m_buckets[bucket].push_back(std::move(item));
        }
        else
        {
            m_tree.push_back(std::move(item));
        }
    }

    void Sprite(const Node& node)
    {
        DrawItem item;
        item.node = &node;
        const fe::Sprite* sprite =
            node.images.empty() ? nullptr : FindIn(m_context.resources, node.images[0], &Resources::FindSprite);
        uint32_t colour = WithAlpha(node.colour, EffectiveAlpha(node));

        if (!sprite)
        {
            Placeholder(item, node);
            return Emit(std::move(item), 2);
        }

        float w = float(sprite->width), h = float(sprite->height);
        bool stretch = (node.flags & 1) != 0;
        float sx = stretch && sprite->width ? node.width / w : 1.0f;
        float sy = stretch && sprite->height ? node.height / h : 1.0f;
        float drawnW = w * sx, drawnH = h * sy;
        float nativeX = float(sprite->nativeX);
        float ox = node.justX == Right ? -w / nativeX : node.justX == Centre ? -w / (2 * nativeX) : 0.0f;
        float oy = node.justY == Top ? -h / nativeX : node.justY == Centre ? -h / (2 * nativeX) : 0.0f;
        float left = node.x + ox, bottom = node.y + oy;
        bool flipX = (node.flags & 2) != 0, flipY = (node.flags & 4) != 0;

        for (const SpriteTile& tile : sprite->tiles)
        {
            DrawItem part;
            part.node = &node;
            part.texture = tile.texture;
            float tx0 = tile.x0, tx1 = tile.x1, ty0 = tile.y0, ty1 = tile.y1;
            float u0 = tile.u0, u1 = tile.u1, v0 = tile.v0, v1 = tile.v1;

            if (flipX)
            {
                std::tie(tx0, tx1) = std::make_pair(w - tx1, w - tx0);
                std::swap(u0, u1);
            }

            if (flipY)
            {
                std::tie(ty0, ty1) = std::make_pair(h - ty1, h - ty0);
                std::swap(v0, v1);
            }

            Quad(part.triangles,
                left + tx0 * sx,
                bottom + ty0 * sy,
                left + tx1 * sx,
                bottom + ty1 * sy,
                u0,
                v0,
                u1,
                v1,
                colour);
            Rotate(part.triangles, 0, node.x + node.width / 2.0f, node.y + node.height / 2.0f, node.rotation);
            Bounds(part, left, bottom, left + drawnW, bottom + drawnH);
            Emit(std::move(part), 2);
        }
    }

    void Text(const Node& node)
    {
        DrawItem item;
        item.node = &node;
        Bounds(item, float(node.x), float(node.y), float(node.x + node.width), float(node.y + node.height));
        const TextureFont* font = nullptr;

        for (const TextureFont* f : m_context.fonts)
        {
            if (_stricmp(f->Name().c_str(), node.textStyle.c_str()) == 0)
            {
                font = f;
            }
        }

        if (!font && !m_context.fonts.empty())
        {
            font = m_context.fonts.front();
        }

        std::u16string text = ResolveText(node, m_context.bibles);

        if (!font || text.empty())
        {
            item.placeholder = !font;
            return Emit(std::move(item), 4);
        }

        float scale = (node.flags & 1) ? node.height / (font->FontHeight() / 1.4285715f) : 1.0f;
        int textW = static_cast<int>(font->TextWidth(text) * scale);
        int textH = static_cast<int>(font->FontHeight() * scale);
        int offX = node.justX == Right ? node.width - textW : node.justX == Centre ? (node.width - textW) >> 1 : 0;
        int offY = node.justY == Top ? node.height - textH : node.justY == Centre ? (node.height - textH) >> 1 : 0;
        std::vector<TextureFont::Quad> quads;

        if (node.textFlags & 2)
        {
            int align = node.justX == Right ? 3 : node.justX == Centre ? 2 : 1;
            font->LayoutWrapped(text, float(node.x), float(node.y + offY), scale, float(node.width), align, quads);
        }
        else
        {
            font->LayoutLine(text, float(node.x + offX), float(node.y + offY), scale, quads);
        }

        uint32_t colour = WithAlpha(node.colour, EffectiveAlpha(node));

        for (const TextureFont::Quad& q : quads)
        {
            item.minX = std::min(item.minX, q.x0), item.maxX = std::max(item.maxX, q.x1);
            item.minY = std::min(item.minY, q.y0), item.maxY = std::max(item.maxY, q.y1);
        }

        for (const TextureFont::Quad& q : quads)
        {
            DrawItem glyph;
            glyph.node = &node;
            glyph.texture = q.texture;
            Quad(glyph.triangles, q.x0, q.y0, q.x1, q.y1, q.u0, q.v0, q.u1, q.v1, colour);
            glyph.minX = item.minX, glyph.maxX = item.maxX, glyph.minY = item.minY, glyph.maxY = item.maxY;
            Emit(std::move(glyph), 4);
        }

        if (quads.empty())
        {
            Emit(std::move(item), 4);
        }
    }

    void Polygon(const Node& node)
    {
        if (node.points.size() < 3)
        {
            return;
        }

        DrawItem item;
        item.node = &node;
        uint32_t alpha = EffectiveAlpha(node);
        float minX = node.points[0].x, maxX = minX, minY = node.points[0].y, maxY = minY;

        for (const auto& p : node.points)
        {
            minX = std::min(minX, p.x), maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y), maxY = std::max(maxY, p.y);
        }

        for (size_t i = 1; i + 1 < node.points.size(); ++i)
        {
            for (size_t k : {size_t(0), i, i + 1})
            {
                const PolygonPoint& p = node.points[k];
                item.triangles.push_back({std::trunc(p.x), std::trunc(p.y), 0, 0, WithAlpha(p.colour, alpha)});
            }
        }

        Bounds(item, minX, minY, maxX, maxY);
        Emit(std::move(item), 1);
    }

    void RoundedRectangle(const Node& node)
    {
        DrawItem item;
        item.node = &node;
        uint32_t colour = WithAlpha(node.colour, EffectiveAlpha(node));
        float x0 = float(node.x), y0 = float(node.y), x1 = x0 + node.width, y1 = y0 + node.height;
        float r = float(node.cornerRadius);
        const GpuTexture* texture = FindIn(m_context.resources, "roundrectanglecorner.tga", &Resources::FindTexture);
        item.texture = texture ? texture->Get() : nullptr;
        Bounds(item, x0, y0, x1, y1);
        struct Part
        {
            float px0, py0, px1, py1, u0, v0, u1, v1;
            bool transpose;
        };
        bool tl = node.squareCorners & 1, tr = node.squareCorners & 2, bl = node.squareCorners & 4,
             br = node.squareCorners & 8;
        const Part parts[] = {
            {x0 + r, y0 + r, x1 - r, y1 - r, 0.25f, 0.0f, 0.5f, 0.5f, false},
            {x0 + r, y0, x1 - r, y0 + r, 0.5f, 1.0f, 0.25f, 0.0f, false},
            {x0 + r, y1 - r, x1 - r, y1, 0.25f, 0.0f, 0.5f, 1.0f, false},
            {x0, y0 + r, x0 + r, y1 - r, 0.25f, 1.0f, 0.5f, 0.0f, true},
            {x1 - r, y0 + r, x1, y1 - r, 0.5f, 0.0f, 0.25f, 1.0f, true},
            br ? Part{x1 - r, y0, x1, y0 + r, 0.5f, 1.0f, 0.75f, 0.0f, false}
               : Part{x1 - r, y0, x1, y0 + r, 0.25f, 1.0f, 0.0f, 0.0f, false},
            tr ? Part{x1 - r, y1 - r, x1, y1, 0.5f, 0.0f, 0.75f, 1.0f, false}
               : Part{x1 - r, y1 - r, x1, y1, 0.25f, 0.0f, 0.0f, 1.0f, false},
            tl ? Part{x0, y1 - r, x0 + r, y1, 0.5f, 1.0f, 0.75f, 0.0f, true}
               : Part{x0, y1 - r, x0 + r, y1, 0.0f, 0.0f, 0.25f, 1.0f, false},
            bl ? Part{x0, y0, x0 + r, y0 + r, 0.75f, 1.0f, 0.5f, 0.0f, false}
               : Part{x0, y0, x0 + r, y0 + r, 0.0f, 1.0f, 0.25f, 0.0f, false},
        };

        for (const Part& p : parts)
        {
            auto vertex = [&](float x, float y, float u, float v)
            {
                return Vertex{x, y, u, 1.0f - v, colour};
            };
            Vertex a = vertex(p.px0, p.py0, p.u0, p.v0);
            Vertex b = p.transpose ? vertex(p.px0, p.py1, p.u1, p.v0) : vertex(p.px0, p.py1, p.u0, p.v1);
            Vertex c = vertex(p.px1, p.py1, p.u1, p.v1);
            Vertex d = p.transpose ? vertex(p.px1, p.py0, p.u0, p.v1) : vertex(p.px1, p.py0, p.u1, p.v0);
            item.triangles.insert(item.triangles.end(), {a, b, c, a, c, d});
        }

        Emit(std::move(item), 0);
    }

    void Pure3dObject(const Node& node)
    {
        DrawItem item;
        item.node = &node;
        Placeholder(item, node);
        Emit(std::move(item), 3);
    }

    void Placeholder(DrawItem& item, const Node& node)
    {
        item.placeholder = true;
        Bounds(item, float(node.x), float(node.y), float(node.x + node.width), float(node.y + node.height));
    }

    const RenderContext& m_context;
    std::vector<DrawItem> m_tree;
    std::vector<DrawItem> m_buckets[5];
};

}

uint32_t EffectiveAlpha(const Node& node)
{
    uint32_t alpha = node.colour >> 24;

    for (const Node* parent = node.parent; parent; parent = parent->parent)
    {
        if (parent->type == Type::Layer || parent->type == Type::Group)
        {
            alpha = alpha * std::min<uint32_t>(parent->alpha, 255) / 255;
        }
    }

    return alpha;
}

std::u16string ResolveText(const Node& text, const TextBibles* bibles)
{
    if (text.strings.empty())
    {
        return {};
    }

    const std::string& raw = text.strings[std::min<size_t>(text.currentText, text.strings.size() - 1)];

    if (bibles)
    {
        return bibles->Expand(raw);
    }

    std::u16string out;

    for (unsigned char c : raw)
    {
        out += static_cast<char16_t>(c);
    }

    return out;
}

std::vector<DrawItem> BuildPage(const Node& page, const RenderContext& context)
{
    Builder builder(context);
    builder.Walk(page);
    return builder.Finish();
}

}
