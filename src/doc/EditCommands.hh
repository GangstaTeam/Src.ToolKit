#pragma once

#include "doc/Document.hh"
#include "p3d/Schema.hh"

#include <memory>

namespace tk
{

class EditCommands
{
public:
    EditCommands(Document& document, const Schema& schema) : m_document(document), m_schema(schema) {}

    bool HasSelection() const { return m_document.Selection() != nullptr; }
    bool CanPaste() const { return m_clipboard != nullptr; }

    void Copy();
    void Cut();
    void Paste();
    void Duplicate();
    void Delete();
    void MoveUp();
    void MoveDown();
    void AddChild(uint32_t id);
    void AddSibling(uint32_t id);

private:
    Chunk* InsertionParent() const;

    Document& m_document;
    const Schema& m_schema;
    std::unique_ptr<Chunk> m_clipboard;
};

}
