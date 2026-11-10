#include "ui/PropertiesPanel.hh"

#include "ui/ChunkLabel.hh"

#include <imgui.h>

namespace tk
{

PropertiesPanel::PropertiesPanel(Document& document, const Schema& schema, const char* title)
    : m_document(document), m_schema(schema), m_title(title), m_editor(schema)
{
}

void PropertiesPanel::Refresh(Chunk* chunk)
{
    if (chunk == m_cachedChunk && m_document.Revision() == m_cachedRevision)
    {
        return;
    }

    m_cachedChunk = chunk;
    m_cachedRevision = m_document.Revision();
    m_def = chunk ? m_schema.FindChunk(chunk->Id()) : nullptr;
    m_decoded = m_def ? DecodePayload(m_schema, *m_def, chunk->Payload()) : DecodedPayload{};
}

void PropertiesPanel::Draw()
{
    if (!ImGui::Begin(m_title))
    {
        ImGui::End();
        return;
    }

    Chunk* chunk = m_document.Selection();
    Refresh(chunk);

    if (!chunk)
    {
        ImGui::TextDisabled("Select a chunk in the tree.");
    }
    else
    {
        DrawHeader(chunk);
        DrawStatus();

        if (m_def && m_decoded.ok && m_editor.Draw(m_decoded.fields))
        {
            m_document.SetPayload(chunk, EncodePayload(m_decoded));
        }
    }

    ImGui::End();
}

void PropertiesPanel::DrawHeader(Chunk* chunk)
{
    ImGui::TextUnformatted(ChunkLabel(m_schema, *chunk).c_str());
    uint32_t id = chunk->Id();
    ImGui::SetNextItemWidth(160);
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue;

    if (ImGui::InputScalar("Chunk id", ImGuiDataType_U32, &id, nullptr, nullptr, "%X", flags))
    {
        m_document.SetId(chunk, id);
    }

    ImGui::TextDisabled("payload %zu bytes   chunk %zu bytes   %zu children",
        chunk->Payload().size(),
        chunk->TotalSize(),
        chunk->ChildCount());
    if (!chunk->Trailing().empty())
    {
        ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1),
            "%zu trailing byte(s) after the children",
            chunk->Trailing().size());
    }

    ImGui::Separator();
}

void PropertiesPanel::DrawStatus()
{
    ImVec4 warning(1.0f, 0.7f, 0.3f, 1.0f);

    if (!m_def)
    {
        ImGui::TextColored(warning, "No schema for this chunk id - use the Hex panel.");
    }
    else if (!m_decoded.ok)
    {
        ImGui::TextColored(warning, "Schema '%s' does not match: %s", m_def->name.c_str(), m_decoded.error.c_str());
    }
    else if (!m_decoded.leftover.empty())
    {
        ImGui::TextColored(warning, "%s (kept as-is)", m_decoded.error.c_str());
    }
}

}
