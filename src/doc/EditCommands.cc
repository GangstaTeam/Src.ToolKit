#include "doc/EditCommands.hh"

#include "doc/ChunkFactory.hh"

namespace tk
{

Chunk* EditCommands::InsertionParent() const
{
    Chunk* selection = m_document.Selection();
    return selection ? selection : m_document.Root();
}

void EditCommands::Copy()
{
    if (Chunk* selection = m_document.Selection())
    {
        m_clipboard = selection->Clone();
    }
}

void EditCommands::Cut()
{
    Copy();
    Delete();
}

void EditCommands::Paste()
{
    if (!m_clipboard)
    {
        return;
    }

    Chunk* parent = InsertionParent();
    m_document.Select(m_document.Insert(parent, parent->ChildCount(), m_clipboard->Clone()));
}

void EditCommands::Duplicate()
{
    Chunk* selection = m_document.Selection();

    if (!selection || !selection->Parent())
    {
        return;
    }

    m_document.Select(m_document.Insert(selection->Parent(), selection->IndexInParent() + 1, selection->Clone()));
}

void EditCommands::Delete()
{
    if (Chunk* selection = m_document.Selection())
    {
        m_document.Remove(selection);
    }
}

void EditCommands::MoveUp()
{
    Chunk* selection = m_document.Selection();

    if (!selection || !selection->Parent() || selection->IndexInParent() == 0)
    {
        return;
    }

    m_document.Move(selection, selection->Parent(), selection->IndexInParent() - 1);
}

void EditCommands::MoveDown()
{
    Chunk* selection = m_document.Selection();

    if (!selection || !selection->Parent())
    {
        return;
    }

    size_t index = selection->IndexInParent();

    if (index + 1 >= selection->Parent()->ChildCount())
    {
        return;
    }

    m_document.Move(selection, selection->Parent(), index + 2);
}

void EditCommands::AddChild(uint32_t id)
{
    Chunk* parent = InsertionParent();
    m_document.Select(m_document.Insert(parent, parent->ChildCount(), CreateChunk(m_schema, id)));
}

void EditCommands::AddSibling(uint32_t id)
{
    Chunk* selection = m_document.Selection();

    if (!selection || !selection->Parent())
    {
        AddChild(id);
        return;
    }

    m_document.Select(
        m_document.Insert(selection->Parent(), selection->IndexInParent() + 1, CreateChunk(m_schema, id)));
}

}
