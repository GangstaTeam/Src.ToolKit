#pragma once

#include "fe/FeModel.hh"
#include "p3d/Chunk.hh"
#include "p3d/Schema.hh"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace tk::fe
{

enum EFeAuthoringValue : uint32_t
{
    kSpriteVersion = 2,
    kTextVersion = 18,
    kTextShadow = 0x80000000,
};

std::unique_ptr<Chunk> NewProject(const Schema& schema, const std::string& name);
std::unique_ptr<Chunk> NewPage(const Schema& schema, const std::string& name);
std::unique_ptr<Chunk> NewLayer(const Schema& schema, const std::string& name);
std::unique_ptr<Chunk> NewScreen(const Schema& schema, const std::string& name, const std::vector<std::string>& pages);
std::unique_ptr<Chunk>
NewSprite(const Schema& schema, const std::string& name, int x, int y, int width, int height, const std::string& image);
std::unique_ptr<Chunk>
NewText(const Schema& schema, const std::string& name, int x, int y, int width, int height, const std::string& text);
std::unique_ptr<Chunk> NewRoundedRectangle(const Schema& schema,
    const std::string& name,
    int x,
    int y,
    int width,
    int height,
    uint32_t argb,
    uint32_t radius);

std::string HardCodedText(const Chunk& text);
void SetHardCodedText(const Schema& schema, Chunk& text, const std::string& value);

std::string SpriteImage(const Schema& schema, const Chunk& sprite);
void SetSpriteImage(const Schema& schema, Chunk& sprite, const std::string& image);

void RenamePage(const Schema& schema, Chunk& project, Chunk& page, const std::string& name);
void ListPageOnScreen(const Schema& schema, Chunk& project, const std::string& page);
void UnlistPage(const Schema& schema, Chunk& project, const std::string& page);

void SetElementSize(const Schema& schema, Chunk& element, int width, int height);

std::string UniqueName(const Project& project, const std::string& base);

}
