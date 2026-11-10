#include "fe/FeModel.hh"

#include "core/ByteReader.hh"
#include "p3d/ChunkIds.hh"

#include <algorithm>
#include <cctype>
#include <cstring>

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

int32_t I32(ByteReader& reader)
{
    return static_cast<int32_t>(reader.U32());
}

float F32(ByteReader& reader)
{
    return reader.F32();
}

std::string Lower(std::string text)
{
    for (char& c : text)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return text;
}

void ReadBox(ByteReader& r, Node& node)
{
    node.x = I32(r);
    node.y = I32(r);
    node.width = I32(r);
    node.height = I32(r);
}

std::unique_ptr<Node> ParseElement(Chunk& chunk, Node* parent);

void ParseChildren(Chunk& chunk, Node& node)
{
    for (size_t i = 0; i < chunk.ChildCount(); ++i)
    {
        if (auto child = ParseElement(*chunk.Child(i), &node))
        {
            node.children.push_back(std::move(child));
        }
    }
}

std::unique_ptr<Node> ParseElement(Chunk& chunk, Node* parent)
{
    auto node = std::make_unique<Node>();
    node->chunk = &chunk;
    node->parent = parent;
    ByteReader r(chunk.Payload());

    switch (chunk.Id())
    {
    case chunk::Layer:
    case chunk::Layer2:
        node->type = Type::Layer;
        node->name = PString(r);
        node->version = r.U32();
        node->visible = r.U32() != 0;
        r.U32();
        node->alpha = r.U32();
        ParseChildren(chunk, *node);
        break;
    case chunk::Group:
    case chunk::Group2:
        node->type = Type::Group;
        node->name = PString(r);
        node->version = r.U32();
        node->alpha = r.U32();
        ParseChildren(chunk, *node);
        break;
    case chunk::MultiSprite:
    case chunk::Sprite2:
    {
        node->type = Type::Sprite;
        node->name = PString(r);
        node->version = r.U32();
        ReadBox(r, *node);

        if (node->version >= 2)
        {
            node->flags = r.U8();
        }

        node->justX = r.U32();
        node->justY = r.U32();
        node->colour = r.U32();
        r.U32();
        node->rotation = F32(r);
        uint32_t count = r.U32();

        for (uint32_t i = 0; i < count && !r.Failed(); ++i)
        {
            node->images.push_back(PString(r));
        }
        break;
    }
    case chunk::MultiText:
    case chunk::Text2:
        node->type = Type::Text;
        node->name = PString(r);
        node->version = r.U32();
        ReadBox(r, *node);

        if (node->version >= 18)
        {
            node->flags = r.U8();
        }

        node->justX = r.U32();
        node->justY = r.U32();
        node->colour = r.U32();
        r.U32();
        node->rotation = F32(r);
        node->textStyle = PString(r);

        if (node->version > 16)
        {
            node->textFlags = r.U8();
            r.U32();
            r.U32();
            r.U32();
            node->currentText = r.U32();
        }

        for (size_t i = 0; i < chunk.ChildCount(); ++i)
        {
            Chunk& child = *chunk.Child(i);
            ByteReader s(child.Payload());

            if (child.Id() == chunk::StringHardCoded)
            {
                node->strings.push_back(PString(s));
            }
            else if (child.Id() == chunk::StringTextBible)
            {
                PString(s);
                node->strings.push_back("[" + PString(s) + "]");
            }
        }
        break;
    case chunk::Pure3dObject:
    case chunk::Pure3dObject2:
        node->type = Type::Pure3dObject;
        node->name = PString(r);
        node->version = r.U32();
        ReadBox(r, *node);

        if (node->version >= 3)
        {
            node->flags = r.U8();
        }

        node->justX = r.U32();
        node->justY = r.U32();
        node->colour = r.U32();
        r.U32();
        node->rotation = F32(r);
        node->drawable = PString(r);

        if (r.Remaining())
        {
            node->camera = PString(r);
        }

        if (r.Remaining())
        {
            node->animation = PString(r);
        }

        if (r.Remaining())
        {
            node->lights = PString(r);
        }
        break;
    case chunk::Polygon:
    {
        node->type = Type::Polygon;
        node->name = PString(r);
        node->version = r.U32();
        r.U32();
        uint32_t count = r.U32();
        node->points.resize(count);

        for (auto& point : node->points)
        {
            point.x = F32(r);
            point.y = F32(r);
            F32(r);
        }

        for (auto& point : node->points)
        {
            point.colour = r.U32();
        }

        if (!node->points.empty())
        {
            node->colour = node->points.back().colour;
        }
        break;
    }
    case chunk::RoundedRectangle:
        node->type = Type::RoundedRectangle;
        node->name = PString(r);
        ReadBox(r, *node);
        node->squareCorners = r.U32();
        node->colour = r.U32();
        r.U32();
        node->cornerRadius = r.U32();
        break;
    default:
        return nullptr;
    }

    return node;
}

}

bool ContainsProject(const Chunk& root)
{
    for (size_t i = 0; i < root.ChildCount(); ++i)
    {
        if (root.Child(i)->Id() == chunk::Project)
        {
            return true;
        }
    }

    return false;
}

Project Parse(Chunk& root)
{
    Project project;
    Chunk* chunk = nullptr;

    for (size_t i = 0; i < root.ChildCount() && !chunk; ++i)
    {
        if (root.Child(i)->Id() == chunk::Project)
        {
            chunk = root.Child(i);
        }
    }

    if (!chunk)
    {
        return project;
    }

    project.root = std::make_unique<Node>();
    Node& node = *project.root;
    node.type = Type::Project;
    node.chunk = chunk;
    ByteReader r(chunk->Payload());
    node.name = PString(r);
    node.version = r.U32();
    node.resolutionX = r.U32();
    node.resolutionY = r.U32();

    for (size_t i = 0; i < chunk->ChildCount(); ++i)
    {
        Chunk& child = *chunk->Child(i);
        ByteReader c(child.Payload());
        auto item = std::make_unique<Node>();
        item->chunk = &child;
        item->parent = &node;

        if (child.Id() == chunk::Page)
        {
            item->type = Type::Page;
            item->name = PString(c);
            item->version = c.U32();
            item->resolutionX = c.U32();
            item->resolutionY = c.U32();
            ParseChildren(child, *item);
            project.pages.push_back(item.get());
        }
        else if (child.Id() == chunk::Screen)
        {
            item->type = Type::Screen;
            item->name = PString(c);
            item->version = c.U32();
            uint32_t count = c.U32();

            for (uint32_t p = 0; p < count && !c.Failed(); ++p)
            {
                item->pageNames.push_back(PString(c));
            }

            project.screens.push_back(item.get());
        }
        else
        {
            continue;
        }

        node.children.push_back(std::move(item));
    }

    return project;
}

const char* TypeName(Type type)
{
    switch (type)
    {
    case Type::Project:
        return "Project";
    case Type::Screen:
        return "Screen";
    case Type::Page:
        return "Page";
    case Type::Layer:
        return "Layer";
    case Type::Group:
        return "Group";
    case Type::Sprite:
        return "Sprite";
    case Type::Text:
        return "Text";
    case Type::Pure3dObject:
        return "Pure3D object";
    case Type::Polygon:
        return "Polygon";
    case Type::RoundedRectangle:
        return "Rounded rectangle";
    }

    return "?";
}

Node* FindPage(const Project& project, const std::string& name)
{
    std::string key = Lower(name);

    for (Node* page : project.pages)
    {
        if (Lower(page->name) == key)
        {
            return page;
        }
    }

    return nullptr;
}

}
