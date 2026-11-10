#include "ui/ImageBrowser.hh"

#include "core/FileIo.hh"
#include "p3d/ChunkIo.hh"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cmath>

namespace tk
{

namespace
{

std::string Lower(std::string text)
{
    for (char& c : text)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return text;
}

void DrawSprite(ImDrawList* list, const fe::Sprite& sprite, ImVec2 min, ImVec2 max)
{
    if (sprite.width <= 0 || sprite.height <= 0)
    {
        return;
    }

    float scale = std::min((max.x - min.x) / sprite.width, (max.y - min.y) / sprite.height);
    float w = sprite.width * scale, h = sprite.height * scale;
    float ox = std::floor(min.x + ((max.x - min.x) - w) / 2), oy = std::floor(min.y + ((max.y - min.y) - h) / 2);

    for (const fe::SpriteTile& tile : sprite.tiles)
    {
        ImVec2 a(ox + tile.x0 * scale, oy + (sprite.height - tile.y1) * scale);
        ImVec2 b(ox + tile.x1 * scale, oy + (sprite.height - tile.y0) * scale);
        list->AddImage(reinterpret_cast<ImTextureID>(tile.texture),
            a,
            b,
            ImVec2(tile.u0, tile.v0),
            ImVec2(tile.u1, tile.v1));
    }
}

void DrawTexture(ImDrawList* list, const GpuTexture& texture, ImVec2 min, ImVec2 max)
{
    if (!texture.Width() || !texture.Height())
    {
        return;
    }

    float scale = std::min((max.x - min.x) / texture.Width(), (max.y - min.y) / texture.Height());
    float w = texture.Width() * scale, h = texture.Height() * scale;
    ImVec2 a(std::floor(min.x + ((max.x - min.x) - w) / 2), std::floor(min.y + ((max.y - min.y) - h) / 2));
    list->AddImage(reinterpret_cast<ImTextureID>(texture.Get()), a, ImVec2(a.x + w, a.y + h));
}

}

void ImageBrowser::Open(const std::string& current)
{
    m_open = true;
    m_focusFilter = true;
    m_current = current;
    ImGui::SetWindowFocus(Window());
}

void ImageBrowser::Clear()
{
    m_resources.Clear();
    m_loaded.clear();
    m_indexedCount = SIZE_MAX;
}

void ImageBrowser::Filter(const fe::ImageIndex& index)
{
    if (m_appliedFilter == m_filter && m_indexedCount == index.Names().size())
    {
        return;
    }

    m_appliedFilter = m_filter;
    m_indexedCount = index.Names().size();
    std::string needle = Lower(m_filter);
    m_filtered.clear();

    for (const std::string& name : index.Names())
    {
        if (needle.empty() || Lower(name).find(needle) != std::string::npos)
        {
            m_filtered.push_back(name);
        }
    }
}

void ImageBrowser::DrawCell(const fe::ImageIndex& index,
    const std::string& name,
    float cell,
    int& budget,
    std::string& picked,
    bool& chosen)
{
    ImGui::PushID(name.c_str());
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float label = ImGui::GetTextLineHeight();
    ImGui::InvisibleButton("cell", ImVec2(cell, cell + label + 4));
    bool hovered = ImGui::IsItemHovered();

    if (ImGui::IsItemClicked())
    {
        picked = name, chosen = true, m_current = name;
    }

    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        m_open = false;
    }

    const fe::Sprite* sprite = m_resources.FindSprite(name);
    const GpuTexture* texture = sprite ? nullptr : m_resources.FindTexture(name);

    if (!sprite && !texture && budget > 0)
    {
        if (const std::filesystem::path* file = index.Find(name); file && m_loaded.insert(*file).second)
        {
            --budget;

            if (auto bytes = ReadFile(*file))
            {
                ParseResult parsed = ParseP3d(*bytes);

                if (parsed.root)
                {
                    m_resources.AddFrom(m_device, *parsed.root);
                }
            }

            sprite = m_resources.FindSprite(name);
            texture = sprite ? nullptr : m_resources.FindTexture(name);
        }
    }

    ImDrawList* list = ImGui::GetWindowDrawList();
    ImVec2 boxMin = pos, boxMax(pos.x + cell, pos.y + cell);
    list->AddRectFilled(boxMin, boxMax, hovered ? kCellHover : kCellBack, 4.0f);
    ImVec2 inner(boxMin.x + 4, boxMin.y + 4), innerMax(boxMax.x - 4, boxMax.y - 4);

    if (sprite)
    {
        DrawSprite(list, *sprite, inner, innerMax);
    }
    else if (texture)
    {
        DrawTexture(list, *texture, inner, innerMax);
    }
    else
    {
        list->AddText(ImVec2(boxMin.x + cell / 2 - 6, boxMin.y + cell / 2 - label / 2),
            IM_COL32(120, 120, 120, 255),
            "...");
    }

    if (Lower(name) == Lower(m_current))
    {
        list->AddRect(boxMin, boxMax, kCellCurrent, 4.0f, 0, 2.0f);
    }

    list->PushClipRect(ImVec2(pos.x, boxMax.y), ImVec2(pos.x + cell, boxMax.y + label + 4), true);
    list->AddText(ImVec2(pos.x + 2, boxMax.y + 2), ImGui::GetColorU32(ImGuiCol_Text), name.c_str());
    list->PopClipRect();

    if (hovered)
    {
        int w = sprite ? sprite->width : texture ? int(texture->Width()) : 0;
        int h = sprite ? sprite->height : texture ? int(texture->Height()) : 0;
        ImGui::SetTooltip("%s\n%s\n%dx%d", name.c_str(), index.RelativeFile(name).c_str(), w, h);
    }

    ImGui::PopID();
}

bool ImageBrowser::Draw(const fe::ImageIndex& index, std::string& picked)
{
    if (!m_open)
    {
        return false;
    }

    ImGui::SetNextWindowSize(ImVec2(760, 560), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(Window(), &m_open))
    {
        ImGui::End();
        return false;
    }

    if (m_focusFilter)
    {
        ImGui::SetKeyboardFocusHere(), m_focusFilter = false;
    }

    ImGui::SetNextItemWidth(260);
    ImGui::InputTextWithHint("##filter", "search by name", m_filter, sizeof(m_filter));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160);
    ImGui::SliderFloat("##size", &m_cell, 64.0f, 256.0f, "size %.0f");
    Filter(index);
    ImGui::SameLine();
    ImGui::TextDisabled("%d / %d", static_cast<int>(m_filtered.size()), static_cast<int>(index.Names().size()));

    if (index.Names().empty())
    {
        ImGui::TextDisabled("Set the game folder to list the game's images.");
    }

    bool chosen = false;
    ImGui::BeginChild("grid");
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float width = ImGui::GetContentRegionAvail().x;
    int columns = std::max(1, static_cast<int>((width + spacing) / (m_cell + spacing)));
    int rows = (static_cast<int>(m_filtered.size()) + columns - 1) / columns;
    float rowHeight = m_cell + ImGui::GetTextLineHeight() + 4 + ImGui::GetStyle().ItemSpacing.y;
    int budget = kLoadsPerFrame;
    ImGuiListClipper clipper;
    clipper.Begin(rows, rowHeight);

    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
        {
            for (int column = 0; column < columns; ++column)
            {
                size_t i = static_cast<size_t>(row) * columns + column;

                if (i >= m_filtered.size())
                {
                    break;
                }

                if (column)
                {
                    ImGui::SameLine();
                }

                DrawCell(index, m_filtered[i], m_cell, budget, picked, chosen);
            }
        }
    }

    ImGui::EndChild();
    ImGui::End();
    return chosen;
}

}
