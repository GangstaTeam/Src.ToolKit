#pragma once

#include "p3d/Chunk.hh"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace tk
{

namespace fe
{

enum class Type
{
    Project,
    Screen,
    Page,
    Layer,
    Group,
    Sprite,
    Text,
    Pure3dObject,
    Polygon,
    RoundedRectangle
};

enum Justification : uint32_t
{
    Left = 0,
    Right = 1,
    Top = 2,
    Bottom = 3,
    Centre = 4
};

struct PolygonPoint
{
    float x = 0.0f;
    float y = 0.0f;
    uint32_t colour = 0xFFFFFFFF;
};

struct Node
{
    Type type = Type::Group;
    Chunk* chunk = nullptr;
    Node* parent = nullptr;
    std::vector<std::unique_ptr<Node>> children;

    std::string name;
    uint32_t version = 0;

    uint32_t resolutionX = 640;
    uint32_t resolutionY = 448;
    std::vector<std::string> pageNames;
    bool visible = true;
    uint32_t alpha = 255;

    int x = 0, y = 0, width = 0, height = 0;
    uint32_t justX = Left, justY = Bottom;
    uint32_t colour = 0xFFFFFFFF;
    float rotation = 0.0f;
    uint8_t flags = 0;

    std::vector<std::string> images;
    std::string textStyle;
    std::vector<std::string> strings;
    uint8_t textFlags = 0;
    uint32_t currentText = 0;
    std::string drawable, camera, animation, lights;
    std::vector<PolygonPoint> points;
    uint32_t squareCorners = 0;
    uint32_t cornerRadius = 0;

    bool IsDrawable() const { return type >= Type::Sprite; }
};

struct Project
{
    std::unique_ptr<Node> root;
    std::vector<Node*> pages;
    std::vector<Node*> screens;
    std::string error;
};

Project Parse(Chunk& root);
bool ContainsProject(const Chunk& root);
const char* TypeName(Type type);
Node* FindPage(const Project& project, const std::string& name);

}

}
