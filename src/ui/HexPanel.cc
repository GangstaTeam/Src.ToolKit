#include "ui/HexPanel.hh"

#include <imgui.h>
#include <imgui_memory_editor.h>

namespace tk
{

namespace
{

bool* g_editedFlag = nullptr;

void WriteByte(ImU8* data, size_t offset, ImU8 value)
{
    data[offset] = value;

    if (g_editedFlag)
    {
        *g_editedFlag = true;
    }
}

}

void HexPanel::Draw()
{
    if (!ImGui::Begin("Hex"))
    {
        ImGui::End();
        return;
    }

    Chunk* chunk = m_document.Selection();

    if (chunk != m_cachedChunk || m_document.Revision() != m_cachedRevision)
    {
        m_cachedChunk = chunk;
        m_cachedRevision = m_document.Revision();
        m_buffer = chunk ? chunk->Payload() : Bytes{};
    }

    if (!chunk)
    {
        ImGui::TextDisabled("Select a chunk to see its payload.");
        ImGui::End();
        return;
    }

    static MemoryEditor editor;
    editor.WriteFn = &WriteByte;
    m_changed = false;
    g_editedFlag = &m_changed;
    editor.DrawContents(m_buffer.data(), m_buffer.size());
    g_editedFlag = nullptr;

    if (m_changed)
    {
        m_document.SetPayload(chunk, m_buffer);
    }

    ImGui::End();
}

}
