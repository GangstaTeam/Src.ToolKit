#include "ui/ChunkTreePanel.hh"

#include "ui/ChunkLabel.hh"

#include <imgui.h>

#include <cctype>

namespace tk
{

namespace
{

bool ContainsInsensitive(const std::string& text, const char* needle)
{
    std::string a = text, b = needle;

    for (char& c : a)
    {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }

    for (char& c : b)
    {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }

    return a.find(b) != std::string::npos;
}

}

ChunkTreePanel::ChunkTreePanel(Document& document,
    EditCommands& commands,
    const Schema& schema,
    AddChunkPopup& addPopup)
    : m_document(document), m_commands(commands), m_schema(schema), m_addPopup(addPopup)
{
}

void ChunkTreePanel::Draw()
{
    if (!ImGui::Begin("Chunks"))
    {
        ImGui::End();
        return;
    }

    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##filter", "filter by name / object", m_filter, sizeof(m_filter));
    ImGui::BeginChild("tree");
    DrawNode(m_document.Root());
    ImGui::EndChild();

    if (m_pending)
    {
        m_pending();
        m_pending = nullptr;
    }

    ImGui::End();
}

bool ChunkTreePanel::MatchesFilter(const Chunk* chunk) const
{
    if (!m_filter[0] || ContainsInsensitive(ChunkLabel(m_schema, *chunk), m_filter))
    {
        return true;
    }

    for (size_t i = 0; i < chunk->ChildCount(); ++i)
    {
        if (MatchesFilter(chunk->Child(i)))
        {
            return true;
        }
    }

    return false;
}

void ChunkTreePanel::DrawNode(Chunk* chunk)
{
    if (!MatchesFilter(chunk))
    {
        return;
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (chunk->ChildCount() == 0)
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (chunk == m_document.Selection())
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (!chunk->Parent() || m_filter[0])
    {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    bool open = ImGui::TreeNodeEx(chunk, flags, "%s", ChunkLabel(m_schema, *chunk).c_str());

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        m_document.Select(chunk);
    }

    DrawContextMenu(chunk);
    HandleDragDrop(chunk);

    if (!open)
    {
        return;
    }

    for (size_t i = 0; i < chunk->ChildCount(); ++i)
    {
        DrawNode(chunk->Child(i));
    }

    ImGui::TreePop();
}

void ChunkTreePanel::DrawContextMenu(Chunk* chunk)
{
    if (!ImGui::BeginPopupContextItem())
    {
        return;
    }

    m_document.Select(chunk);

    if (ImGui::MenuItem("Add child..."))
    {
        m_addPopup.Open("Add child chunk",
            chunk->Id(),
            [this](uint32_t id)
            {
                m_commands.AddChild(id);
            });
    }

    bool isRoot = chunk->Parent() == nullptr;

    if (ImGui::MenuItem("Add sibling...", nullptr, false, !isRoot))
    {
        uint32_t parentId = chunk->Parent()->Id();
        m_addPopup.Open("Add sibling chunk",
            parentId,
            [this](uint32_t id)
            {
                m_commands.AddSibling(id);
            });
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Cut", "Ctrl+X", false, !isRoot))
    {
        m_pending = [this]
        {
            m_commands.Cut();
        };
    }

    if (ImGui::MenuItem("Copy", "Ctrl+C", false, !isRoot))
    {
        m_commands.Copy();
    }

    if (ImGui::MenuItem("Paste as child", "Ctrl+V", false, m_commands.CanPaste()))
    {
        m_pending = [this]
        {
            m_commands.Paste();
        };
    }

    if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, !isRoot))
    {
        m_pending = [this]
        {
            m_commands.Duplicate();
        };
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Move up", "Alt+Up", false, !isRoot))
    {
        m_pending = [this]
        {
            m_commands.MoveUp();
        };
    }

    if (ImGui::MenuItem("Move down", "Alt+Down", false, !isRoot))
    {
        m_pending = [this]
        {
            m_commands.MoveDown();
        };
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Delete", "Del", false, !isRoot))
    {
        m_pending = [this]
        {
            m_commands.Delete();
        };
    }

    ImGui::EndPopup();
}

void ChunkTreePanel::HandleDragDrop(Chunk* chunk)
{
    if (chunk->Parent() && ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload(CHUNKTREEPANEL_DRAG_TYPE, &chunk, sizeof(chunk));
        ImGui::TextUnformatted(ChunkLabel(m_schema, *chunk).c_str());
        ImGui::EndDragDropSource();
    }

    if (!ImGui::BeginDragDropTarget())
    {
        return;
    }

    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(CHUNKTREEPANEL_DRAG_TYPE))
    {
        Chunk* dragged = *static_cast<Chunk* const*>(payload->Data);
        m_pending = [this, dragged, chunk]
        {
            m_document.Move(dragged, chunk, chunk->ChildCount());
            m_document.Select(dragged);
        };
    }

    ImGui::EndDragDropTarget();
}

}
