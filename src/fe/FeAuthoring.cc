#include "fe/FeAuthoring.hh"

#include "p3d/ChunkIds.hh"
#include "p3d/PayloadCodec.hh"

#include <algorithm>
#include <functional>
#include <set>

namespace tk::fe
{

namespace
{

using Fill = std::function<void(std::vector<Field>&)>;

std::unique_ptr<Chunk> Build(const Schema& schema, uint32_t id, const Fill& fill)
{
    DecodedPayload decoded;

    if (const ChunkDef* def = schema.FindChunk(id))
    {
        for (const FieldDef& field : def->fields)
        {
            decoded.fields.push_back(MakeDefaultField(schema, field));
        }
    }

    fill(decoded.fields);
    SyncArrayCounts(decoded.fields);
    return std::make_unique<Chunk>(id, EncodePayload(decoded));
}

void Modify(const Schema& schema, Chunk& chunk, const Fill& fill)
{
    const ChunkDef* def = schema.FindChunk(chunk.Id());

    if (!def)
    {
        return;
    }

    DecodedPayload decoded = DecodePayload(schema, *def, chunk.Payload());

    if (!decoded.ok)
    {
        return;
    }

    fill(decoded.fields);
    SyncArrayCounts(decoded.fields);
    chunk.SetPayload(EncodePayload(decoded));
}

void SetText(std::vector<Field>& fields, const char* name, const std::string& value)
{
    if (Field* field = FindField(fields, name))
    {
        field->text = value;
    }
}

void SetInt(std::vector<Field>& fields, const char* name, int64_t value)
{
    if (Field* field = FindField(fields, name))
    {
        field->integer = static_cast<uint32_t>(value);
    }
}

void SetStrings(std::vector<Field>& fields, const char* name, const std::vector<std::string>& values)
{
    Field* field = FindField(fields, name);

    if (!field)
    {
        return;
    }

    field->items.clear();

    for (const std::string& value : values)
    {
        Field item;
        item.name = field->name;
        item.type = "string";
        item.primitive = Primitive::String;
        item.text = value;
        field->items.push_back(item);
    }
}

std::vector<std::string> GetStrings(const std::vector<Field>& fields, const char* name)
{
    std::vector<std::string> values;

    if (const Field* field = FindField(fields, name))
    {
        for (const Field& item : field->items)
        {
            values.push_back(item.text);
        }
    }

    return values;
}

void SetBox(std::vector<Field>& fields, int x, int y, int width, int height)
{
    SetInt(fields, "PositionX", x);
    SetInt(fields, "PositionY", y);
    SetInt(fields, "DimensionX", width);
    SetInt(fields, "DimensionY", height);
}

std::string Stem(const std::string& name)
{
    size_t dot = name.rfind('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

std::unique_ptr<Chunk> NewHardCodedString(const Schema& schema, const std::string& value)
{
    return Build(schema,
        chunk::StringHardCoded,
        [&](std::vector<Field>& f)
        {
            SetText(f, "String", value);
        });
}

}

std::unique_ptr<Chunk> NewProject(const Schema& schema, const std::string& name)
{
    std::string stem = Stem(name);
    auto project = Build(schema,
        chunk::Project,
        [&](std::vector<Field>& f)
        {
            SetText(f, "Name", stem + ".prj");
            SetInt(f, "Version", 1);
            SetInt(f, "ResolutionX", 640);
            SetInt(f, "ResolutionY", 448);
            SetText(f, "Platform", "PS2");
            SetText(f, "PagePath", "Pages\\");
            SetText(f, "ResourcePath", "Resource\\");
            SetText(f, "ScreenPath", "Screens\\");
        });
    auto page = NewPage(schema, stem + ".pag");
    page->AppendChild(NewLayer(schema, stem + "Layer"));
    project->AppendChild(std::move(page));
    project->AppendChild(NewScreen(schema, stem + ".scr", {stem + ".pag"}));
    return project;
}

std::unique_ptr<Chunk> NewPage(const Schema& schema, const std::string& name)
{
    return Build(schema,
        chunk::Page,
        [&](std::vector<Field>& f)
        {
            SetText(f, "Name", name);
            SetInt(f, "ResolutionX", 640);
            SetInt(f, "ResolutionY", 448);
        });
}

std::unique_ptr<Chunk> NewLayer(const Schema& schema, const std::string& name)
{
    return Build(schema,
        chunk::Layer2,
        [&](std::vector<Field>& f)
        {
            SetText(f, "Name", name);
            SetInt(f, "Version", 1);
            SetInt(f, "Visible", 1);
            SetInt(f, "Editable", 1);
            SetInt(f, "Alpha", 255);
        });
}

std::unique_ptr<Chunk> NewScreen(const Schema& schema, const std::string& name, const std::vector<std::string>& pages)
{
    return Build(schema,
        chunk::Screen,
        [&](std::vector<Field>& f)
        {
            SetText(f, "Name", name);
            SetStrings(f, "PageNames", pages);
        });
}

std::unique_ptr<Chunk>
NewSprite(const Schema& schema, const std::string& name, int x, int y, int width, int height, const std::string& image)
{
    return Build(schema,
        chunk::Sprite2,
        [&](std::vector<Field>& f)
        {
            SetText(f, "Name", name);
            SetInt(f, "Version", kSpriteVersion);
            SetBox(f, x, y, width, height);
            SetInt(f, "Flags", 1);
            SetInt(f, "JustificationX", Left);
            SetInt(f, "JustificationY", Top);
            SetInt(f, "Color", 0xFFFFFFFF);
            SetStrings(f, "ImageNames", image.empty() ? std::vector<std::string>{} : std::vector<std::string>{image});
        });
}

std::unique_ptr<Chunk>
NewText(const Schema& schema, const std::string& name, int x, int y, int width, int height, const std::string& text)
{
    auto chunk = Build(schema,
        chunk::Text2,
        [&](std::vector<Field>& f)
        {
            SetText(f, "Name", name);
            SetInt(f, "Version", kTextVersion);
            SetBox(f, x, y, width, height);
            SetInt(f, "ScaleToBox", 1);
            SetInt(f, "JustificationX", Left);
            SetInt(f, "JustificationY", Top);
            SetInt(f, "Color", 0xFFFFFFFF);
            SetText(f, "TextStyleName", "TCCB_16");
            SetInt(f, "ShadowColour", kTextShadow);
            SetInt(f, "ShadowOffsetX", -1);
            SetInt(f, "ShadowOffsetY", -2);
        });
    chunk->AppendChild(NewHardCodedString(schema, text));
    return chunk;
}

std::unique_ptr<Chunk> NewRoundedRectangle(const Schema& schema,
    const std::string& name,
    int x,
    int y,
    int width,
    int height,
    uint32_t argb,
    uint32_t radius)
{
    return Build(schema,
        chunk::RoundedRectangle,
        [&](std::vector<Field>& f)
        {
            SetText(f, "Name", name);
            SetBox(f, x, y, width, height);
            SetInt(f, "Color", argb);
            SetInt(f, "CornerRadius", radius);
        });
}

std::string HardCodedText(const Chunk& text)
{
    for (size_t i = 0; i < text.ChildCount(); ++i)
    {
        const Chunk& child = *text.Child(i);

        if (child.Id() != chunk::StringHardCoded || child.Payload().empty())
        {
            continue;
        }

        const Bytes& payload = child.Payload();
        size_t length = std::min<size_t>(payload[0], payload.size() - 1);
        std::string value(payload.begin() + 1, payload.begin() + 1 + length);
        size_t nul = value.find('\0');

        if (nul != std::string::npos)
        {
            value.resize(nul);
        }

        return value;
    }

    return {};
}

void SetHardCodedText(const Schema& schema, Chunk& text, const std::string& value)
{
    for (size_t i = text.ChildCount(); i-- > 0;)
    {
        uint32_t id = text.Child(i)->Id();

        if (id == chunk::StringHardCoded || id == chunk::StringTextBible)
        {
            text.RemoveChild(i);
        }
    }

    text.AppendChild(NewHardCodedString(schema, value));
    Modify(schema,
        text,
        [](std::vector<Field>& f)
        {
            SetInt(f, "CurrentText", 0);
        });
}

std::string SpriteImage(const Schema& schema, const Chunk& sprite)
{
    const ChunkDef* def = schema.FindChunk(sprite.Id());

    if (!def)
    {
        return {};
    }

    DecodedPayload decoded = DecodePayload(schema, *def, sprite.Payload());
    std::vector<std::string> images = GetStrings(decoded.fields, "ImageNames");
    return images.empty() ? std::string() : images.front();
}

void SetSpriteImage(const Schema& schema, Chunk& sprite, const std::string& image)
{
    Modify(schema,
        sprite,
        [&](std::vector<Field>& f)
        {
            std::vector<std::string> images = GetStrings(f, "ImageNames");

            if (images.empty())
            {
                images.push_back(image);
            }
            else
            {
                images.front() = image;
            }

            SetStrings(f, "ImageNames", images);
        });
}

void RenamePage(const Schema& schema, Chunk& project, Chunk& page, const std::string& name)
{
    std::string old;
    Modify(schema,
        page,
        [&](std::vector<Field>& f)
        {
            if (Field* field = FindField(f, "Name"))
            {
                old = field->text, field->text = name;
            }
        });
    for (size_t i = 0; i < project.ChildCount(); ++i)
    {
        Chunk& screen = *project.Child(i);

        if (screen.Id() != chunk::Screen)
        {
            continue;
        }

        Modify(schema,
            screen,
            [&](std::vector<Field>& f)
            {
                std::vector<std::string> pages = GetStrings(f, "PageNames");

                for (std::string& entry : pages)
                {
                    if (entry == old)
                    {
                        entry = name;
                    }
                }

                SetStrings(f, "PageNames", pages);
            });
    }
}

void ListPageOnScreen(const Schema& schema, Chunk& project, const std::string& page)
{
    for (size_t i = 0; i < project.ChildCount(); ++i)
    {
        Chunk& screen = *project.Child(i);

        if (screen.Id() != chunk::Screen)
        {
            continue;
        }

        Modify(schema,
            screen,
            [&](std::vector<Field>& f)
            {
                std::vector<std::string> pages = GetStrings(f, "PageNames");
                pages.push_back(page);
                SetStrings(f, "PageNames", pages);
            });
        return;
    }

    std::string name;

    if (const ChunkDef* def = schema.FindChunk(project.Id()))
    {
        DecodedPayload decoded = DecodePayload(schema, *def, project.Payload());

        if (const Field* field = FindField(decoded.fields, "Name"))
        {
            name = Stem(field->text);
        }
    }

    project.AppendChild(NewScreen(schema, (name.empty() ? "screen" : name) + ".scr", {page}));
}

void UnlistPage(const Schema& schema, Chunk& project, const std::string& page)
{
    for (size_t i = 0; i < project.ChildCount(); ++i)
    {
        Chunk& screen = *project.Child(i);

        if (screen.Id() != chunk::Screen)
        {
            continue;
        }

        Modify(schema,
            screen,
            [&](std::vector<Field>& f)
            {
                std::vector<std::string> pages = GetStrings(f, "PageNames");
                pages.erase(std::remove(pages.begin(), pages.end(), page), pages.end());
                SetStrings(f, "PageNames", pages);
            });
    }
}

void SetElementSize(const Schema& schema, Chunk& element, int width, int height)
{
    Modify(schema,
        element,
        [&](std::vector<Field>& f)
        {
            SetInt(f, "DimensionX", width);
            SetInt(f, "DimensionY", height);
        });
}

std::string UniqueName(const Project& project, const std::string& base)
{
    std::set<std::string> used;
    std::function<void(const Node&)> collect = [&](const Node& node)
    {
        used.insert(node.name);

        for (const auto& child : node.children)
        {
            collect(*child);
        }
    };

    if (project.root)
    {
        collect(*project.root);
    }

    for (const Node* screen : project.screens)
    {
        used.insert(screen->name);
    }

    if (!used.count(base))
    {
        return base;
    }

    for (int i = 2;; ++i)
    {
        std::string candidate = base + std::to_string(i);

        if (!used.count(candidate))
        {
            return candidate;
        }
    }
}

}
