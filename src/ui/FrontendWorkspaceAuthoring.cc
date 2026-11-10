#include "ui/FrontendWorkspace.hh"

#include "core/FileIo.hh"
#include "fe/FeAuthoring.hh"
#include "p3d/ChunkIds.hh"
#include "p3d/ChunkIo.hh"
#include "ui/Icons.hh"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iterator>
#include <cstring>
#include <functional>

namespace tk
{

namespace
{

bool ContainsNoCase(const std::string& text, const char* filter)
{
    if (!filter[0])
    {
        return true;
    }

    std::string a = text, b = filter;

    for (char& c : a)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    for (char& c : b)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return a.find(b) != std::string::npos;
}

void CopyText(char* buffer, size_t size, const std::string& text)
{
    std::strncpy(buffer, text.c_str(), size - 1);
    buffer[size - 1] = '\0';
}

}

void FrontendWorkspace::DrawNewProjectPopup()
{
    if (m_newProjectRequested)
    {
        ImGui::OpenPopup("New frontend project");
        m_newProjectRequested = false;
    }

    if (!ImGui::BeginPopupModal("New frontend project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    ImGui::TextUnformatted("Name of the project (the .prj, its first .pag and .scr):");
    ImGui::SetNextItemWidth(280);

    if (ImGui::IsWindowAppearing())
    {
        ImGui::SetKeyboardFocusHere();
    }

    bool accept =
        ImGui::InputText("##name", m_newProjectName, sizeof(m_newProjectName), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::TextDisabled("Builds %s.prj > %s.pag > %sLayer, and %s.scr",
        m_newProjectName,
        m_newProjectName,
        m_newProjectName,
        m_newProjectName);
    accept |= ImGui::Button("Create", ImVec2(120, 0));
    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(120, 0)))
    {
        ImGui::CloseCurrentPopup();
    }

    if (accept && m_newProjectName[0])
    {
        m_document.New();
        Chunk* root = m_document.Root();
        m_document.Insert(root, root->ChildCount(), fe::NewProject(m_schema, m_newProjectName));
        m_page = nullptr;
        m_screen = nullptr;
        m_fit = true;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

fe::Node* FrontendWorkspace::CurrentPage() const
{
    if (fe::Node* selected = NodeFor(m_document.Selection()))
    {
        if (fe::Node* page = PageOf(selected))
        {
            return page;
        }
    }

    if (m_page)
    {
        return NodeFor(m_page->chunk);
    }

    std::vector<const fe::Node*> shown = ShownPages();
    return shown.empty() ? nullptr : NodeFor(shown.front()->chunk);
}

Chunk* FrontendWorkspace::TargetLayer(size_t& index) const
{
    if (fe::Node* selected = NodeFor(m_document.Selection()))
    {
        if (selected->type == fe::Type::Layer || selected->type == fe::Type::Group)
        {
            index = selected->chunk->ChildCount();
            return selected->chunk;
        }

        if (selected->IsDrawable() && selected->parent &&
            (selected->parent->type == fe::Type::Layer || selected->parent->type == fe::Type::Group))
        {
            index = selected->chunk->IndexInParent() + 1;
            return selected->parent->chunk;
        }
    }

    fe::Node* page = CurrentPage();

    if (!page)
    {
        return nullptr;
    }

    for (auto it = page->children.rbegin(); it != page->children.rend(); ++it)
    {
        if ((*it)->type == fe::Type::Layer)
        {
            index = (*it)->chunk->ChildCount();
            return (*it)->chunk;
        }
    }

    return nullptr;
}

void FrontendWorkspace::AddElement(std::unique_ptr<Chunk> chunk)
{
    size_t index = 0;
    Chunk* layer = TargetLayer(index);

    if (!layer)
    {
        return;
    }

    Chunk* added = m_document.Insert(layer, index, std::move(chunk));
    m_document.Select(added);
}

void FrontendWorkspace::AddLayer()
{
    fe::Node* page = CurrentPage();

    if (!page)
    {
        return;
    }

    Chunk* added = m_document.Insert(page->chunk,
        page->chunk->ChildCount(),
        fe::NewLayer(m_schema, fe::UniqueName(m_project, "Layer")));
    m_document.Select(added);
}

void FrontendWorkspace::AddPage()
{
    if (!m_project.root)
    {
        return;
    }

    Chunk* project = m_project.root->chunk;
    std::string name;

    for (int i = 1; name.empty(); ++i)
    {
        std::string candidate = "Page" + std::to_string(i) + ".pag";

        if (!fe::FindPage(m_project, candidate))
        {
            name = candidate;
        }
    }

    std::string layer = fe::UniqueName(m_project, name.substr(0, name.size() - 4) + "Layer");
    Chunk* added = nullptr;
    m_document.Edit(
        [&](Chunk&)
        {
            size_t index = project->ChildCount();

            for (size_t i = 0; i < project->ChildCount(); ++i)
            {
                if (project->Child(i)->Id() == chunk::Screen)
                {
                    index = i;
                    break;
                }
            }

            auto page = fe::NewPage(m_schema, name);
            page->AppendChild(fe::NewLayer(m_schema, layer));
            added = project->InsertChild(std::move(page), index);
            fe::ListPageOnScreen(m_schema, *project, name);
        });
    m_document.Select(added);
    m_page = nullptr;
    m_screen = nullptr;
}

void FrontendWorkspace::DeleteSelection()
{
    fe::Node* selected = NodeFor(m_document.Selection());

    if (!selected || selected->type == fe::Type::Project)
    {
        return;
    }

    if (selected->type == fe::Type::Page && m_project.root)
    {
        Chunk* project = m_project.root->chunk;
        Chunk* page = selected->chunk;
        std::string name = selected->name;
        m_document.Edit(
            [&](Chunk&)
            {
                fe::UnlistPage(m_schema, *project, name);
                project->RemoveChild(page->IndexInParent());
            });
        m_page = nullptr;
        return;
    }

    m_document.Remove(selected->chunk);
}

void FrontendWorkspace::DrawAddBar()
{
    bool canAdd = m_project.root != nullptr;
    size_t index = 0;
    bool hasLayer = canAdd && TargetLayer(index) != nullptr;
    ImVec2 centre(320.0f, 224.0f);
    ImGui::TextDisabled("Add");
    ImGui::SameLine();
    ImGui::BeginDisabled(!hasLayer);

    if (ImGui::Button((std::string(icon::Image) + "  Sprite").c_str()))
    {
        AddElement(fe::NewSprite(m_schema,
            fe::UniqueName(m_project, "Sprite"),
            int(centre.x) - kSpriteSize / 2,
            int(centre.y) - kSpriteSize / 2,
            kSpriteSize,
            kSpriteSize,
            ""));
    }

    ImGui::SameLine();

    if (ImGui::Button((std::string(icon::Text) + "  Text").c_str()))
    {
        AddElement(fe::NewText(m_schema,
            fe::UniqueName(m_project, "Text"),
            int(centre.x) - kTextWidth / 2,
            int(centre.y) - kTextHeight / 2,
            kTextWidth,
            kTextHeight,
            "TEXT"));
    }

    ImGui::SameLine();

    if (ImGui::Button((std::string(icon::Shape) + "  Rounded rect").c_str()))
    {
        AddElement(fe::NewRoundedRectangle(m_schema,
            fe::UniqueName(m_project, "Box"),
            int(centre.x) - kBoxWidth / 2,
            int(centre.y) - kBoxHeight / 2,
            kBoxWidth,
            kBoxHeight,
            kBoxColour,
            kBoxRadius));
    }

    ImGui::EndDisabled();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasLayer)
    {
        ImGui::SetTooltip("Select a page or a layer first");
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!canAdd || !CurrentPage());

    if (ImGui::Button((std::string(icon::Layers) + "  Layer").c_str()))
    {
        AddLayer();
    }

    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!canAdd);

    if (ImGui::Button((std::string(icon::Page) + "  Page").c_str()))
    {
        AddPage();
    }

    ImGui::EndDisabled();
    ImGui::SameLine();
    fe::Node* selected = NodeFor(m_document.Selection());
    ImGui::BeginDisabled(!selected || selected->type == fe::Type::Project);

    if (ImGui::Button("Delete"))
    {
        DeleteSelection();
    }

    ImGui::EndDisabled();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Del");
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    DrawGameFolderButton();
}

void FrontendWorkspace::DrawElementExtras()
{
    if (!ImGui::Begin(ElementWindow()))
    {
        return ImGui::End();
    }

    fe::Node* node = NodeFor(m_document.Selection());

    if (node && (node->IsDrawable() || node->type == fe::Type::Layer || node->type == fe::Type::Group))
    {
        DrawAlignControls();
    }

    if (!node || !(node->type == fe::Type::Text || node->type == fe::Type::Sprite || node->type == fe::Type::Page))
    {
        return ImGui::End();
    }

    Chunk* chunk = node->chunk;

    if (m_editedChunk != chunk)
    {
        m_editedChunk = chunk;
        CopyText(m_editText,
            sizeof(m_editText),
            node->type == fe::Type::Text ? fe::HardCodedText(*chunk) : std::string());
        CopyText(m_editName, sizeof(m_editName), node->name);
        m_imageFilter[0] = '\0';
    }

    if (node->type == fe::Type::Page)
    {
        ImGui::SetNextItemWidth(-90);
        ImGui::InputText("Page name", m_editName, sizeof(m_editName));

        if (ImGui::IsItemDeactivatedAfterEdit() && m_editName[0] && node->name != m_editName && m_project.root)
        {
            Chunk* project = m_project.root->chunk;
            std::string name = m_editName;
            m_document.Edit(
                [&](Chunk&)
                {
                    fe::RenamePage(m_schema, *project, *chunk, name);
                });
            m_document.Select(chunk);
            m_page = nullptr;
        }

        ImGui::TextDisabled("Screens that list the page follow the new name.");
    }
    else if (node->type == fe::Type::Text)
    {
        ImGui::TextUnformatted("Text");
        ImGui::InputTextMultiline("##text",
            m_editText,
            sizeof(m_editText),
            ImVec2(-1, ImGui::GetTextLineHeight() * 3.5f));
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            std::string value = m_editText;
            m_document.Edit(
                [&](Chunk&)
                {
                    fe::SetHardCodedText(m_schema, *chunk, value);
                });
            m_document.Select(chunk);
        }

        ImGui::TextDisabled("Hard-coded string; the game can still change it at run time (SetText).");
    }
    else
    {
        std::string current = fe::SpriteImage(m_schema, *chunk);

        if (m_imageBrowser.IsOpen())
        {
            m_imageBrowser.SetCurrent(current);
        }

        float browse =
            ImGui::CalcTextSize("Browse...").x + ImGui::GetStyle().FramePadding.x * 2 + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetNextItemWidth(-90 - browse);
        float popupWidth = std::max(ImGui::CalcItemWidth(), 360.0f);
        ImGui::SetNextWindowSizeConstraints(ImVec2(popupWidth, 380), ImVec2(popupWidth, 380));
        bool open =
            ImGui::BeginCombo("##image", current.empty() ? "(none)" : current.c_str(), ImGuiComboFlags_NoArrowButton);
        ImGui::SameLine();

        if (ImGui::Button("Browse..."))
        {
            m_imageBrowser.Open(current);
        }

        ImGui::SameLine();
        ImGui::TextUnformatted("Image");

        if (open)
        {
            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetKeyboardFocusHere();
            }

            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##filter", "filter", m_imageFilter, sizeof(m_imageFilter));

            for (const std::string& name : m_images.Names())
            {
                if (!ContainsNoCase(name, m_imageFilter))
                {
                    continue;
                }

                if (ImGui::Selectable(name.c_str(), name == current))
                {
                    m_document.Edit(
                        [&](Chunk&)
                        {
                            fe::SetSpriteImage(m_schema, *chunk, name);
                        });
                    m_document.Select(chunk);
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", m_images.RelativeFile(name).c_str());
                }
            }

            if (m_images.Names().empty())
            {
                ImGui::TextDisabled("Set the game folder to list the game's images.");
            }

            ImGui::EndCombo();
        }

        std::string file = m_images.RelativeFile(current);

        if (!current.empty())
        {
            ImGui::TextDisabled("%s", file.empty() ? "not in the game's frontend folder" : ("from " + file).c_str());
        }

        const fe::Sprite* sprite = current.empty() ? nullptr : m_gameResources.FindSprite(current);

        if (!sprite && !current.empty())
        {
            sprite = m_documentResources.FindSprite(current);
        }

        ImGui::BeginDisabled(!sprite);

        if (ImGui::Button("Native size") && sprite)
        {
            int width = sprite->width, height = sprite->height;
            m_document.Edit(
                [&](Chunk&)
                {
                    fe::SetElementSize(m_schema, *chunk, width, height);
                });
            m_document.Select(chunk);
        }

        ImGui::EndDisabled();

        if (sprite)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("%dx%d", sprite->width, sprite->height);
        }
    }

    ImGui::Separator();
    ImGui::End();
}

std::vector<std::string> FrontendWorkspace::UsedImages() const
{
    std::vector<std::string> images;
    std::function<void(const fe::Node&)> collect = [&](const fe::Node& node)
    {
        if (node.type == fe::Type::Sprite)
        {
            for (const std::string& image : node.images)
            {
                if (std::find(images.begin(), images.end(), image) == images.end())
                {
                    images.push_back(image);
                }
            }
        }

        if (node.type == fe::Type::RoundedRectangle &&
            std::find(images.begin(), images.end(), FRONTENDWORKSPACEAUTHORING_ROUND_CORNER) == images.end())
        {
            images.push_back(FRONTENDWORKSPACEAUTHORING_ROUND_CORNER);
        }

        for (const auto& child : node.children)
        {
            collect(*child);
        }
    };

    if (m_project.root)
    {
        collect(*m_project.root);
    }

    return images;
}

void FrontendWorkspace::LoadUsedImages()
{
    if (m_images.Names().empty())
    {
        return;
    }

    for (const std::string& image : UsedImages())
    {
        if (m_documentResources.FindSprite(image) || m_documentResources.FindTexture(image))
        {
            continue;
        }

        if (m_gameResources.FindSprite(image) || m_gameResources.FindTexture(image))
        {
            continue;
        }

        const std::filesystem::path* file = m_images.Find(image);

        if (!file || !m_loadedImageFiles.insert(*file).second)
        {
            continue;
        }

        if (auto bytes = ReadFile(*file))
        {
            ParseResult parsed = ParseP3d(*bytes);

            if (parsed.root)
            {
                m_gameResources.AddFrom(m_device, *parsed.root);
            }
        }
    }
}

void FrontendWorkspace::AlignSelection(int horizontal, int vertical)
{
    fe::Node* node = NodeFor(m_document.Selection());

    if (!node || !m_hasSelectionBounds)
    {
        return;
    }

    auto offset = [](int mode, float min, float max, float page) -> int
    {
        switch (mode)
        {
        case -1:
            return static_cast<int>(std::lround(-min));
        case 0:
            return static_cast<int>(std::lround((page - (max - min)) / 2 - min));
        case 1:
            return static_cast<int>(std::lround(page - max));
        default:
            return 0;
        }
    };
    int dx = offset(horizontal, m_selectionMin.x, m_selectionMax.x, 640.0f);
    int dy = offset(vertical, m_selectionMin.y, m_selectionMax.y, 448.0f);
    Move(*node, dx, dy);
}

void FrontendWorkspace::DrawAlignControls()
{
    ImGui::TextUnformatted("Align on page");
    ImGui::BeginDisabled(!m_hasSelectionBounds);
    struct Button
    {
        const char* label;
        const char* tip;
        int horizontal, vertical;
    };
    const Button buttons[] = {
        {"Left", "Left edge on the page's left edge", -1, 2},
        {"Centre", "Centred horizontally", 0, 2},
        {"Right", "Right edge on the page's right edge", 1, 2},
        {"Top", "Top edge on the page's top edge", 2, 1},
        {"Middle", "Centred vertically", 2, 0},
        {"Bottom", "Bottom edge on the page's bottom edge", 2, -1},
        {"Centre both", "Centred on the page", 0, 0},
    };

    for (size_t i = 0; i < std::size(buttons); ++i)
    {
        if (i != 0 && i != 3 && i != 6)
        {
            ImGui::SameLine();
        }

        if (ImGui::Button(buttons[i].label))
        {
            AlignSelection(buttons[i].horizontal, buttons[i].vertical);
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("%s", buttons[i].tip);
        }
    }

    ImGui::EndDisabled();
    ImGui::Separator();
}

void FrontendWorkspace::DrawImageBrowser()
{
    std::string picked;

    if (!m_imageBrowser.Draw(m_images, picked))
    {
        return;
    }

    fe::Node* node = NodeFor(m_document.Selection());

    if (!node || node->type != fe::Type::Sprite)
    {
        return;
    }

    Chunk* chunk = node->chunk;
    m_document.Edit(
        [&](Chunk&)
        {
            fe::SetSpriteImage(m_schema, *chunk, picked);
        });
    m_document.Select(chunk);
}

void FrontendWorkspace::DrawGameFilesNote()
{
    std::vector<std::string> images;

    for (const std::string& image : UsedImages())
    {
        if (!m_documentResources.FindSprite(image) && !m_documentResources.FindTexture(image))
        {
            images.push_back(image);
        }
    }

    std::stable_sort(images.begin(),
        images.end(),
        [&](const std::string& a, const std::string& b)
        {
            return m_images.Files(a).size() < m_images.Files(b).size();
        });
    std::vector<std::string> files;

    for (const std::string& image : images)
    {
        const std::vector<std::filesystem::path>& holders = m_images.Files(image);

        if (holders.empty())
        {
            files.push_back(image + " (not found)");
            continue;
        }

        bool covered = false;

        for (const auto& holder : holders)
        {
            covered = covered || std::find(files.begin(), files.end(), m_images.Relative(holder)) != files.end();
        }

        if (!covered)
        {
            files.push_back(m_images.Relative(holders.front()));
        }
    }

    ImGui::Separator();

    if (files.empty())
    {
        ImGui::TextDisabled("Uses no game images.");
        return;
    }

    std::string list;

    for (const std::string& file : files)
    {
        list += (list.empty() ? "" : ", ") + file;
    }

    ImGui::TextDisabled("Game files to load first (%d):", static_cast<int>(files.size()));
    ImGui::TextWrapped("%s", list.c_str());
}

}
