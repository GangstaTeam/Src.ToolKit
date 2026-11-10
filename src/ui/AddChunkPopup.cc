#include "ui/AddChunkPopup.hh"

#include <imgui.h>

#include <cstdio>
#include <cstdlib>

namespace tk
{

void AddChunkPopup::Open(const char* title, uint32_t parentId, std::function<void(uint32_t)> onPick)
{
    m_title = title;
    m_parentId = parentId;
    m_onPick = std::move(onPick);
    m_filter[0] = 0;
    m_openRequested = true;
}

bool AddChunkPopup::Matches(const ChunkDef& def) const
{
    if (!m_filter[0])
    {
        return true;
    }

    char id[16];
    std::snprintf(id, sizeof(id), "%X", def.id);
    return def.name.find(m_filter) != std::string::npos || std::string(id).find(m_filter) != std::string::npos;
}

bool AddChunkPopup::DrawEntry(uint32_t id)
{
    char label[160];
    std::snprintf(label, sizeof(label), "%-40s 0x%X", m_schema.ChunkName(id).c_str(), id);

    if (!ImGui::Selectable(label, false, ImGuiSelectableFlags_DontClosePopups))
    {
        return false;
    }

    m_onPick(id);
    ImGui::CloseCurrentPopup();
    return true;
}

void AddChunkPopup::Draw()
{
    if (m_openRequested)
    {
        ImGui::OpenPopup(m_title.c_str());
        m_openRequested = false;
    }

    ImGui::SetNextWindowSize(ImVec2(560, 520), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal(m_title.c_str(), nullptr))
    {
        return;
    }

    if (ImGui::IsWindowAppearing())
    {
        ImGui::SetKeyboardFocusHere();
    }

    ImGui::InputTextWithHint("##filter", "search name or id", m_filter, sizeof(m_filter));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(110);
    ImGui::InputTextWithHint("##custom", "0x...", m_customId, sizeof(m_customId), ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();

    if (ImGui::Button("Add id") && m_customId[0])
    {
        m_onPick(static_cast<uint32_t>(std::strtoul(m_customId, nullptr, 16)));
        ImGui::CloseCurrentPopup();
    }

    ImGui::BeginChild("list", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
    const ChunkDef* parent = m_schema.FindChunk(m_parentId);

    if (parent && !parent->subchunks.empty())
    {
        ImGui::SeparatorText("Allowed children");

        for (uint32_t id : parent->subchunks)
        {
            if (const ChunkDef* def = m_schema.FindChunk(id); def && Matches(*def) && DrawEntry(id))
            {
                break;
            }
        }

        ImGui::SeparatorText("All chunk types");
    }

    for (const auto& [id, def] : m_schema.Chunks())
    {
        if (Matches(def) && DrawEntry(id))
        {
            break;
        }
    }

    ImGui::EndChild();

    if (ImGui::Button("Cancel"))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

}
