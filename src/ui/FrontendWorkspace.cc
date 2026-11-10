#include "ui/FrontendWorkspace.hh"

#include "app/FileDialogs.hh"
#include "core/FileIo.hh"
#include "fe/FeAuthoring.hh"
#include "p3d/ChunkIds.hh"
#include "p3d/ChunkIo.hh"
#include "p3d/PayloadCodec.hh"
#include "ui/Icons.hh"

#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace tk
{

namespace
{

ImU32 ToImGui(uint32_t argb)
{
    return IM_COL32((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF, argb >> 24);
}

const char* IconFor(fe::Type type)
{
    switch (type)
    {
    case fe::Type::Project:
        return icon::Archive;
    case fe::Type::Screen:
        return icon::Screen;
    case fe::Type::Page:
        return icon::Page;
    case fe::Type::Layer:
        return icon::Layers;
    case fe::Type::Group:
        return icon::Group;
    case fe::Type::Sprite:
        return icon::Image;
    case fe::Type::Text:
        return icon::Text;
    case fe::Type::Pure3dObject:
        return icon::Model;
    case fe::Type::Polygon:
    case fe::Type::RoundedRectangle:
        return icon::Shape;
    }

    return icon::Document;
}

std::string Narrow(const std::u16string& text)
{
    std::string out;

    for (char16_t c : text)
    {
        if (c < 0x80)
        {
            out += static_cast<char>(c);
        }
        else if (c < 0x800)
        {
            out += char(0xC0 | (c >> 6)), out += char(0x80 | (c & 0x3F));
        }
        else
        {
            out += char(0xE0 | (c >> 12)), out += char(0x80 | ((c >> 6) & 0x3F)), out += char(0x80 | (c & 0x3F));
        }
    }

    return out;
}

bool IsUnder(const fe::Node* node, const fe::Node* ancestor)
{
    for (; node; node = node->parent)
    {
        if (node == ancestor)
        {
            return true;
        }
    }

    return false;
}

std::filesystem::path DetectGameFolder(const std::filesystem::path& file)
{
    std::error_code ec;

    for (std::filesystem::path dir = file.parent_path(); !dir.empty(); dir = dir.parent_path())
    {
        if (std::filesystem::exists(dir / "frontend" / "font.p3d", ec) &&
            std::filesystem::is_directory(dir / "textbible", ec))
        {
            return dir;
        }

        if (dir == dir.root_path())
        {
            break;
        }
    }

    return {};
}

void* OpenEntry(ImGuiContext*, ImGuiSettingsHandler* handler, const char* name)
{
    return std::strcmp(name, "Frontend") == 0 ? handler->UserData : nullptr;
}

void ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    const char* const prefix = "GameFolder=";

    if (std::strncmp(line, prefix, std::strlen(prefix)) == 0)
    {
        static_cast<FrontendWorkspace*>(entry)->SetGameFolder(std::filesystem::u8path(line + std::strlen(prefix)));
    }
}

void WriteAll(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buffer)
{
    auto* workspace = static_cast<FrontendWorkspace*>(handler->UserData);
    buffer->appendf("[%s][Frontend]\nGameFolder=%s\n\n", handler->TypeName, workspace->GameFolder().u8string().c_str());
}

}

FrontendWorkspace::FrontendWorkspace(Document& document, const Schema& schema, IDirect3DDevice9* device, HWND owner)
    : m_document(document), m_schema(schema), m_device(device), m_owner(owner),
      m_element(document, schema, ElementWindow()), m_imageBrowser(device)
{
}

void FrontendWorkspace::RegisterSettingsHandler()
{
    ImGuiSettingsHandler handler;
    handler.TypeName = "ToolKitFrontend";
    handler.TypeHash = ImHashStr("ToolKitFrontend");
    handler.ReadOpenFn = &OpenEntry;
    handler.ReadLineFn = &ReadLine;
    handler.WriteAllFn = &WriteAll;
    handler.UserData = this;
    ImGui::AddSettingsHandler(&handler);
}

void FrontendWorkspace::SetGameFolder(const std::filesystem::path& folder)
{
    if (folder == m_gameFolder)
    {
        return;
    }

    m_gameFolder = folder;

    if (ImGui::GetCurrentContext())
    {
        ImGui::MarkIniSettingsDirty();
    }
}

void FrontendWorkspace::OnDeviceLost()
{
}

void FrontendWorkspace::Refresh()
{
    bool pathChanged = m_document.Path() != m_documentPath;

    if (!pathChanged && m_document.Revision() == m_revision)
    {
        return;
    }

    m_revision = m_document.Revision();
    m_documentPath = m_document.Path();

    if (pathChanged)
    {
        m_page = nullptr;
        m_screen = nullptr;
        m_hidden.clear();
        m_fit = true;

        if (auto detected = DetectGameFolder(m_documentPath); !detected.empty())
        {
            SetGameFolder(detected);
        }
    }

    std::string pageName = m_page ? m_page->name : std::string();
    std::string screenName = m_screen ? m_screen->name : std::string();

    m_project = fe::Parse(*m_document.Root());
    m_nodes.clear();
    std::function<void(fe::Node&)> index = [&](fe::Node& node)
    {
        m_nodes[node.chunk] = &node;

        for (auto& child : node.children)
        {
            index(*child);
        }
    };

    if (m_project.root)
    {
        index(*m_project.root);
    }

    m_page = pageName.empty() ? nullptr : fe::FindPage(m_project, pageName);
    m_screen = nullptr;

    for (fe::Node* screen : m_project.screens)
    {
        if (screen->name == screenName)
        {
            m_screen = screen;
        }
    }

    if (!m_page && !m_screen && !m_project.pages.empty())
    {
        m_page = m_project.pages.front();
    }

    m_revealed = nullptr;

    m_documentResources.Clear();
    m_documentFonts.clear();

    if (m_project.root)
    {
        m_documentResources.AddFrom(m_device, *m_document.Root());

        for (size_t i = 0; i < m_document.Root()->ChildCount(); ++i)
        {
            const Chunk& chunk = *m_document.Root()->Child(i);

            if (chunk.Id() != chunk::TextureFont)
            {
                continue;
            }

            auto font = std::make_unique<fe::TextureFont>();

            if (font->Load(m_device, chunk))
            {
                m_documentFonts.push_back(std::move(font));
            }
        }
    }
}

void FrontendWorkspace::LoadGameResources()
{
    if (m_gameFolder == m_loadedGameFolder)
    {
        return;
    }

    m_loadedGameFolder = m_gameFolder;
    m_gameResources.Clear();
    m_gameFonts.clear();
    m_bibles.Clear();
    m_images.Clear();
    m_loadedImageFiles.clear();
    m_imageBrowser.Clear();

    if (m_gameFolder.empty())
    {
        m_resourceStatus = "No game folder: text shows keys and no font";
        return;
    }

    if (auto bytes = ReadFile(m_gameFolder / "frontend" / "font.p3d"))
    {
        ParseResult parsed = ParseP3d(*bytes);

        if (parsed.root)
        {
            for (size_t i = 0; i < parsed.root->ChildCount(); ++i)
            {
                if (parsed.root->Child(i)->Id() != chunk::TextureFont)
                {
                    continue;
                }

                auto font = std::make_unique<fe::TextureFont>();

                if (font->Load(m_device, *parsed.root->Child(i)))
                {
                    m_gameFonts.push_back(std::move(font));
                }
            }
        }
    }

    size_t bibles = m_bibles.LoadFolder(m_gameFolder / "textbible", m_language);
    m_images.Build(m_gameFolder);
    m_resourceStatus = std::to_string(m_gameFonts.size()) + " font(s), " + std::to_string(bibles) + " text bible(s), " +
        std::to_string(m_bibles.Count()) + " strings (" + m_language + "), " + std::to_string(m_images.Names().size()) +
        " game image(s)";
}

fe::Node* FrontendWorkspace::NodeFor(const Chunk* chunk) const
{
    auto it = m_nodes.find(chunk);
    return it == m_nodes.end() ? nullptr : it->second;
}

fe::Node* FrontendWorkspace::PageOf(fe::Node* node) const
{
    for (; node; node = node->parent)
    {
        if (node->type == fe::Type::Page)
        {
            return node;
        }
    }

    return nullptr;
}

void FrontendWorkspace::Select(fe::Node* node)
{
    m_document.Select(node ? node->chunk : nullptr);

    if (!node)
    {
        return;
    }

    if (node->type == fe::Type::Screen)
    {
        m_screen = node;
        m_page = nullptr;
    }
    else if (fe::Node* page = PageOf(node))
    {
        std::vector<const fe::Node*> shown = ShownPages();

        if (std::find(shown.begin(), shown.end(), page) == shown.end())
        {
            m_page = page;
            m_screen = nullptr;
        }
    }

    m_revealed = node;
    m_revealPending = true;
}

void FrontendWorkspace::RevealSelection()
{
    fe::Node* selected = NodeFor(m_document.Selection());

    if (!selected || selected == m_revealed)
    {
        return;
    }

    Select(selected);
}

std::vector<const fe::Node*> FrontendWorkspace::ShownPages() const
{
    std::vector<const fe::Node*> pages;

    if (m_screen)
    {
        for (const std::string& name : m_screen->pageNames)
        {
            if (const fe::Node* page = fe::FindPage(m_project, name))
            {
                pages.push_back(page);
            }
        }
    }
    else if (m_page)
    {
        pages.push_back(m_page);
    }

    return pages;
}

void FrontendWorkspace::Move(fe::Node& node, int dx, int dy)
{
    if (!dx && !dy)
    {
        return;
    }

    std::vector<fe::Node*> targets;
    std::function<void(fe::Node&)> collect = [&](fe::Node& n)
    {
        if (n.IsDrawable())
        {
            targets.push_back(&n);
        }

        for (auto& child : n.children)
        {
            collect(*child);
        }
    };
    collect(node);
    Chunk* selected = node.chunk;
    m_document.Edit(
        [&](Chunk&)
        {
            for (fe::Node* target : targets)
            {
                const ChunkDef* def = m_schema.FindChunk(target->chunk->Id());

                if (!def)
                {
                    continue;
                }

                DecodedPayload decoded = DecodePayload(m_schema, *def, target->chunk->Payload());

                if (!decoded.ok)
                {
                    continue;
                }

                if (target->type == fe::Type::Polygon)
                {
                    Field* points = FindField(decoded.fields, "Points");

                    if (!points)
                    {
                        continue;
                    }

                    for (Field& point : points->items)
                    {
                        if (Field* x = FindField(point.items, "X"))
                        {
                            x->real += float(dx);
                        }

                        if (Field* y = FindField(point.items, "Y"))
                        {
                            y->real += float(dy);
                        }
                    }
                }
                else
                {
                    Field* x = FindField(decoded.fields, "PositionX");
                    Field* y = FindField(decoded.fields, "PositionY");

                    if (!x || !y)
                    {
                        continue;
                    }

                    x->integer = static_cast<uint32_t>(static_cast<int32_t>(x->integer) + dx);
                    y->integer = static_cast<uint32_t>(static_cast<int32_t>(y->integer) + dy);
                }

                target->chunk->SetPayload(EncodePayload(decoded));
            }
        });
    m_document.Select(selected);
}

void FrontendWorkspace::Draw()
{
    Refresh();
    LoadGameResources();
    LoadUsedImages();
    RevealSelection();
    DrawNewProjectPopup();
    DrawProject();
    DrawScreen();
    DrawElementExtras();
    m_element.Draw();
    DrawImageBrowser();
}

void FrontendWorkspace::DrawProject()
{
    if (!ImGui::Begin(ProjectWindow()))
    {
        return ImGui::End();
    }

    if (!m_project.root)
    {
        ImGui::TextDisabled("This file has no Scrooby project (chunk 0x18000).");
        ImGui::TextDisabled("Open a frontend file such as frontend/pause.p3d.");
        return ImGui::End();
    }

    fe::Node& project = *m_project.root;
    ImGui::Text("%s  %s", icon::Archive, project.name.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("%ux%u", project.resolutionX, project.resolutionY);
    ImGui::Separator();
    ImGui::BeginChild("tree", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing() * 3));

    if (ImGui::TreeNodeEx("Screens", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        for (fe::Node* screen : m_project.screens)
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

            if (m_screen == screen)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            std::string label =
                std::string(icon::Screen) + "  " + screen->name + "##" + std::to_string(uintptr_t(screen));
            bool open = ImGui::TreeNodeEx(label.c_str(), flags);

            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                Select(screen);
            }

            if (open)
            {
                for (const std::string& name : screen->pageNames)
                {
                    fe::Node* page = fe::FindPage(m_project, name);
                    ImGui::PushID(name.c_str());

                    if (ImGui::Selectable((std::string(icon::Page) + "  " + name + (page ? "" : "  (missing)")).c_str(),
                            false) &&
                        page)
                    {
                        m_page = page;
                        m_screen = nullptr;
                        Select(page);
                    }

                    ImGui::PopID();
                }

                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Pages", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
    {
        for (fe::Node* page : m_project.pages)
        {
            DrawNode(*page);
        }

        ImGui::TreePop();
    }

    m_revealPending = false;
    ImGui::EndChild();
    DrawGameFilesNote();
    ImGui::End();
}

void FrontendWorkspace::DrawNode(fe::Node& node)
{
    ImGui::PushID(&node);
    bool hidden = m_hidden.count(node.chunk) != 0;

    if (node.type != fe::Type::Page)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(hidden ? ImGuiCol_TextDisabled : ImGuiCol_Text));

        if (ImGui::SmallButton(icon::Eye))
        {
            hidden ? (void)m_hidden.erase(node.chunk) : (void)m_hidden.insert(node.chunk);
        }

        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(hidden ? "Show in the preview" : "Hide in the preview");
        }

        ImGui::SameLine();
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (node.children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (m_document.Selection() == node.chunk)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (m_revealPending && m_revealed && m_revealed != &node && IsUnder(m_revealed, &node))
    {
        ImGui::SetNextItemOpen(true);
    }

    std::string label = std::string(IconFor(node.type)) + "  " + node.name;
    bool dim = hidden || (node.type == fe::Type::Layer && !node.visible);

    if (dim)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    }

    bool open = ImGui::TreeNodeEx("node", flags, "%s", label.c_str());

    if (dim)
    {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        Select(&node);
    }

    if (m_revealPending && m_revealed == &node && !ImGui::IsItemVisible())
    {
        ImGui::SetScrollHereY();
    }

    if (node.type == fe::Type::Text)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", Narrow(fe::ResolveText(node, &m_bibles)).c_str());
    }
    else if (node.type == fe::Type::Layer && !node.visible)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(not visible)");
    }

    if (open)
    {
        for (auto& child : node.children)
        {
            DrawNode(*child);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void FrontendWorkspace::DrawScreen()
{
    if (!ImGui::Begin(ScreenWindow()))
    {
        return ImGui::End();
    }

    if (!m_project.root)
    {
        ImGui::TextDisabled("No frontend project in this file.");
        return ImGui::End();
    }

    DrawToolbar();
    DrawAddBar();
    DrawCanvas();
    ImGui::End();
}

void FrontendWorkspace::DrawToolbar()
{
    std::string preview = m_screen ? std::string(icon::Screen) + "  " + m_screen->name
        : m_page                   ? std::string(icon::Page) + "  " + m_page->name
                                   : std::string("(none)");
    ImGui::SetNextItemWidth(260);

    if (ImGui::BeginCombo("##page", preview.c_str()))
    {
        for (fe::Node* screen : m_project.screens)
        {
            if (ImGui::Selectable((std::string(icon::Screen) + "  " + screen->name).c_str(), m_screen == screen))
            {
                Select(screen);
            }
        }

        ImGui::Separator();

        for (fe::Node* page : m_project.pages)
        {
            if (ImGui::Selectable((std::string(icon::Page) + "  " + page->name).c_str(), m_page == page && !m_screen))
            {
                m_page = page;
                m_screen = nullptr;
                Select(page);
            }
        }

        ImGui::EndCombo();
    }

    ImGui::SameLine();

    if (ImGui::RadioButton("Layout", !m_inGame))
    {
        m_inGame = false, m_fit = true;
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("The 640x448 page at 1:1, as authored");
    }

    ImGui::SameLine();

    if (ImGui::RadioButton("In game", m_inGame))
    {
        m_inGame = true, m_fit = true;
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Inside a 640x480 screen with the game's title-safe scale (0.9333)");
    }

    ImGui::SameLine();

    if (ImGui::Button((std::string(icon::Fit) + "  Fit").c_str()))
    {
        m_fit = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("1:1"))
    {
        m_fit = false, m_zoom = 1.0f, m_pan = ImVec2(0, 0);
    }

    ImGui::SameLine();

    if (ImGui::Button("2:1"))
    {
        m_fit = false, m_zoom = 2.0f, m_pan = ImVec2(0, 0);
    }

    ImGui::SameLine();
    ImGui::Checkbox("Bounds", &m_showBounds);
    ImGui::SameLine();
    ImGui::Checkbox("Game draw order", &m_gameOrder);

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("The game batches by type: rounded rectangles, polygons, sprites, 3D objects, then "
                          "text.\nOff: tree order.");
    }
}

void FrontendWorkspace::ChooseGameFolder()
{
    if (auto folder = ShowFolderDialog(m_owner, L"Scarface install folder (with frontend\\font.p3d and textbible)"))
    {
        SetGameFolder(*folder);
    }
}

void FrontendWorkspace::DrawGameFolderButton()
{
    if (ImGui::Button("Game folder..."))
    {
        ChooseGameFolder();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s\n%s",
            m_gameFolder.empty() ? "(not set)" : m_gameFolder.u8string().c_str(),
            m_resourceStatus.c_str());
    }
}

ImVec2 FrontendWorkspace::View::ToScreen(float x, float y) const
{
    if (inGame)
    {
        x = (x - 320.0f) * 0.93333f + 320.0f;
        y = (y - 240.0f) * 0.93333f + 240.0f;
        return ImVec2(origin.x + x * zoom, origin.y + (480.0f - y) * zoom);
    }

    return ImVec2(origin.x + x * zoom, origin.y + (448.0f - y) * zoom);
}

ImVec2 FrontendWorkspace::View::ToPage(ImVec2 screen) const
{
    float x = (screen.x - origin.x) / zoom;
    float y = (inGame ? 480.0f : 448.0f) - (screen.y - origin.y) / zoom;

    if (inGame)
    {
        x = (x - 320.0f) / 0.93333f + 320.0f;
        y = (y - 240.0f) / 0.93333f + 240.0f;
    }

    return ImVec2(x, y);
}

void FrontendWorkspace::DrawCanvas()
{
    ImVec2 size = ImGui::GetContentRegionAvail();
    size.y -= ImGui::GetFrameHeightWithSpacing();

    if (size.x < 32 || size.y < 32)
    {
        return;
    }

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("canvas", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
    bool hovered = ImGui::IsItemHovered();

    float frameH = m_inGame ? 480.0f : 448.0f;

    if (m_fit)
    {
        m_zoom = std::max(0.1f, std::min(size.x / 640.0f, size.y / frameH) * 0.96f);
        m_pan = ImVec2(0, 0);
    }

    ImGuiIO& io = ImGui::GetIO();
    View view;
    view.zoom = m_zoom;
    view.inGame = m_inGame;
    view.origin = ImVec2(std::floor(pos.x + (size.x - 640.0f * m_zoom) / 2 + m_pan.x),
        std::floor(pos.y + (size.y - frameH * m_zoom) / 2 + m_pan.y));
    if (hovered && io.MouseWheel != 0.0f)
    {
        ImVec2 anchor = view.ToPage(io.MousePos);
        float zoom = std::clamp(m_zoom * (io.MouseWheel > 0 ? 1.15f : 1 / 1.15f), 0.1f, 16.0f);
        m_fit = false;
        View next = view;
        next.zoom = zoom;
        ImVec2 moved = next.ToScreen(anchor.x, anchor.y);
        m_pan.x += io.MousePos.x - moved.x;
        m_pan.y += io.MousePos.y - moved.y;
        m_zoom = zoom;
        view.zoom = zoom;
        view.origin = ImVec2(view.origin.x + io.MousePos.x - moved.x, view.origin.y + io.MousePos.y - moved.y);
    }

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
    {
        m_fit = false;
        m_pan.x += io.MouseDelta.x;
        m_pan.y += io.MouseDelta.y;
    }

    fe::RenderContext context;
    context.resources = {&m_documentResources, &m_gameResources};

    for (const auto& font : m_documentFonts)
    {
        context.fonts.push_back(font.get());
    }

    for (const auto& font : m_gameFonts)
    {
        context.fonts.push_back(font.get());
    }

    context.bibles = &m_bibles;

    for (const Chunk* chunk : m_hidden)
    {
        if (const fe::Node* node = NodeFor(chunk))
        {
            context.hidden.insert(node);
        }
    }

    context.gameOrder = m_gameOrder;
    std::vector<fe::DrawItem> items;

    for (const fe::Node* page : ShownPages())
    {
        std::vector<fe::DrawItem> pageItems = fe::BuildPage(*page, context);

        for (auto& item : pageItems)
        {
            items.push_back(std::move(item));
        }
    }

    ImDrawList* list = ImGui::GetWindowDrawList();
    list->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
    list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(28, 26, 26, 255));

    if (m_inGame)
    {
        list->AddRectFilled(view.origin,
            ImVec2(view.origin.x + 640.0f * m_zoom, view.origin.y + 480.0f * m_zoom),
            IM_COL32(0, 0, 0, 255));
    }

    ImVec2 pageMin = view.ToScreen(0, 448.0f), pageMax = view.ToScreen(640.0f, 0);

    if (!m_inGame)
    {
        list->AddRectFilled(pageMin, pageMax, IM_COL32(0, 0, 0, 255));
    }

    DrawItems(list, view, items);
    list->AddRect(pageMin, pageMax, IM_COL32(255, 255, 255, 60));

    if (m_inGame)
    {
        list->AddRect(view.origin,
            ImVec2(view.origin.x + 640.0f * m_zoom, view.origin.y + 480.0f * m_zoom),
            IM_COL32(255, 255, 255, 40));
    }

    HandleCanvasInput(view, items, hovered);
    DrawOverlay(list, view, items);
    list->PopClipRect();

    fe::Node* selected = NodeFor(m_document.Selection());
    ImGui::Text("x %.0f  y %.0f", m_mousePage.x, m_mousePage.y);
    ImGui::SameLine();
    ImGui::TextDisabled("|  %.0f%%", m_zoom * 100.0f);

    if (selected)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("|  %s %s", fe::TypeName(selected->type), selected->name.c_str());

        if (selected->IsDrawable() && selected->type != fe::Type::Polygon)
        {
            ImGui::SameLine();
            ImGui::TextDisabled(" (%d, %d  %dx%d)", selected->x, selected->y, selected->width, selected->height);
        }

        if (selected->type == fe::Type::Text)
        {
            ImGui::SameLine();
            ImGui::TextDisabled(" \"%s\"", Narrow(fe::ResolveText(*selected, &m_bibles)).c_str());
        }
    }

    if (m_gameFonts.empty() && m_documentFonts.empty())
    {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.3f, 1.0f));

        if (ImGui::SmallButton("No font loaded: set the game folder..."))
        {
            ChooseGameFolder();
        }

        ImGui::PopStyleColor();
    }
}

void FrontendWorkspace::DrawItems(ImDrawList* list, const View& view, const std::vector<fe::DrawItem>& items)
{
    ImTextureID white = ImGui::GetIO().Fonts->TexID;
    ImVec2 whiteUv = ImGui::GetIO().Fonts->TexUvWhitePixel;
    const fe::Node* selected = NodeFor(m_document.Selection());

    for (const fe::DrawItem& item : items)
    {
        if (item.triangles.empty())
        {
            continue;
        }

        bool moving = m_dragging && selected && IsUnder(item.node, selected);
        float ox = moving ? m_dragOffset.x : 0.0f, oy = moving ? m_dragOffset.y : 0.0f;
        list->PushTextureID(item.texture ? reinterpret_cast<ImTextureID>(item.texture) : white);
        int count = static_cast<int>(item.triangles.size());
        list->PrimReserve(count, count);

        for (const fe::Vertex& v : item.triangles)
        {
            list->PrimWriteIdx(static_cast<ImDrawIdx>(list->_VtxCurrentIdx));
            list->PrimWriteVtx(view.ToScreen(v.x + ox, v.y + oy),
                item.texture ? ImVec2(v.u, v.v) : whiteUv,
                ToImGui(v.colour));
        }

        list->PopTextureID();
    }
}

void FrontendWorkspace::DrawOverlay(ImDrawList* list, const View& view, const std::vector<fe::DrawItem>& items)
{
    const fe::Node* selected = NodeFor(m_document.Selection());
    m_hasSelectionBounds = false;
    std::set<const fe::Node*> outlined;
    auto outline = [&](const fe::DrawItem& item, ImU32 colour, float thickness, float ox, float oy)
    {
        ImVec2 a = view.ToScreen(item.minX + ox, item.maxY + oy), b = view.ToScreen(item.maxX + ox, item.minY + oy);
        list->AddRect(a, b, colour, 0.0f, 0, thickness);
    };

    for (const fe::DrawItem& item : items)
    {
        bool first = outlined.insert(item.node).second;

        if (item.placeholder && first)
        {
            ImVec2 a = view.ToScreen(item.minX, item.maxY), b = view.ToScreen(item.maxX, item.minY);
            list->AddRectFilled(a, b, IM_COL32(120, 120, 255, 40));
            list->AddRect(a, b, IM_COL32(150, 150, 255, 160));
            list->AddText(ImVec2(a.x + 3, a.y + 2), IM_COL32(200, 200, 255, 220), item.node->name.c_str());
        }

        if (!first)
        {
            continue;
        }

        if (m_showBounds)
        {
            outline(item, kBoundsColour, 1.0f, 0, 0);
        }

        if (item.node == m_hover && !(selected && IsUnder(item.node, selected)))
        {
            outline(item, kHover, 1.0f, 0, 0);
        }
    }

    if (!selected)
    {
        return;
    }

    bool any = false;
    float minX = 0, minY = 0, maxX = 0, maxY = 0;

    for (const fe::DrawItem& item : items)
    {
        if (!IsUnder(item.node, selected))
        {
            continue;
        }

        if (!any)
        {
            minX = item.minX, minY = item.minY, maxX = item.maxX, maxY = item.maxY, any = true;
        }

        minX = std::min(minX, item.minX), minY = std::min(minY, item.minY);
        maxX = std::max(maxX, item.maxX), maxY = std::max(maxY, item.maxY);
    }

    if (!any)
    {
        return;
    }

    m_hasSelectionBounds = true;
    m_selectionMin = ImVec2(minX, minY);
    m_selectionMax = ImVec2(maxX, maxY);
    float ox = m_dragging ? m_dragOffset.x : 0.0f, oy = m_dragging ? m_dragOffset.y : 0.0f;
    ImVec2 a = view.ToScreen(minX + ox, maxY + oy), b = view.ToScreen(maxX + ox, minY + oy);
    list->AddRect(ImVec2(a.x - 1, a.y - 1), ImVec2(b.x + 1, b.y + 1), kSelection, 0.0f, 0, 2.0f);

    if (selected->IsDrawable() && selected->type != fe::Type::Polygon)
    {
        ImVec2 anchor = view.ToScreen(selected->x + ox, selected->y + oy);
        list->AddCircleFilled(anchor, 3.5f, kSelection);
    }
}

void FrontendWorkspace::HandleCanvasInput(const View& view, const std::vector<fe::DrawItem>& items, bool hovered)
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 page = view.ToPage(io.MousePos);

    if (hovered)
    {
        m_mousePage = page;
    }

    m_hover = nullptr;

    if (hovered)
    {
        for (auto it = items.rbegin(); it != items.rend(); ++it)
        {
            if (page.x >= it->minX && page.x <= it->maxX && page.y >= it->minY && page.y <= it->maxY)
            {
                m_hover = it->node;
                break;
            }
        }
    }

    fe::Node* selected = NodeFor(m_document.Selection());

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        bool insideSelection = false;

        if (selected && m_hover && IsUnder(m_hover, selected))
        {
            insideSelection = true;
        }

        if (!insideSelection)
        {
            fe::Node* picked = m_hover ? NodeFor(m_hover->chunk) : nullptr;

            if (picked)
            {
                Select(picked);
            }
            else if (!ShownPages().empty())
            {
                m_document.Select(nullptr);
            }

            selected = picked;
        }

        m_dragging = selected != nullptr && m_hover != nullptr;
        m_dragStart = page;
        m_dragOffset = ImVec2(0, 0);
    }

    if (m_dragging)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            m_dragOffset = ImVec2(std::round(page.x - m_dragStart.x), std::round(page.y - m_dragStart.y));
        }
        else
        {
            m_dragging = false;

            if (selected && (m_dragOffset.x != 0 || m_dragOffset.y != 0))
            {
                Move(*selected, static_cast<int>(m_dragOffset.x), static_cast<int>(m_dragOffset.y));
            }

            m_dragOffset = ImVec2(0, 0);
        }
    }

    if (selected && !m_dragging && ImGui::IsWindowFocused() && !io.WantTextInput &&
        ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        DeleteSelection();
        return;
    }

    if (selected && !m_dragging && ImGui::IsWindowFocused() && !io.WantTextInput)
    {
        int step = io.KeyShift ? 10 : 1;
        int dx = (ImGui::IsKeyPressed(ImGuiKey_RightArrow) ? step : 0) -
            (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) ? step : 0);
        int dy =
            (ImGui::IsKeyPressed(ImGuiKey_UpArrow) ? step : 0) - (ImGui::IsKeyPressed(ImGuiKey_DownArrow) ? step : 0);
        if (dx || dy)
        {
            Move(*selected, dx, dy);
        }
    }
}

}
