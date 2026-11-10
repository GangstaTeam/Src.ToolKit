#pragma once

#include "fe/FeFont.hh"
#include "fe/FeModel.hh"
#include "fe/FeResources.hh"
#include "fe/FeTextBible.hh"

#include <d3d9.h>

#include <cstdint>
#include <set>
#include <vector>

namespace tk::fe
{

struct Vertex
{
    float x, y;
    float u, v;
    uint32_t colour;
};

struct DrawItem
{
    const Node* node = nullptr;
    IDirect3DTexture9* texture = nullptr;
    std::vector<Vertex> triangles;
    float minX = 0, minY = 0, maxX = 0, maxY = 0;
    bool placeholder = false;
};

struct RenderContext
{
    std::vector<const Resources*> resources;
    std::vector<const TextureFont*> fonts;
    const TextBibles* bibles = nullptr;
    std::set<const Node*> hidden;
    bool gameOrder = true;
};

std::vector<DrawItem> BuildPage(const Node& page, const RenderContext& context);
std::u16string ResolveText(const Node& text, const TextBibles* bibles);
uint32_t EffectiveAlpha(const Node& node);

}
