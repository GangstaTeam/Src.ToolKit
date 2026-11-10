#pragma once

#include "doc/Document.hh"
#include "doc/EditCommands.hh"
#include "p3d/Schema.hh"
#include "ui/AddChunkPopup.hh"

#include <functional>

#define CHUNKTREEPANEL_DRAG_TYPE "TK_CHUNK"

namespace tk
{

class ChunkTreePanel
{
public:
    ChunkTreePanel(Document& document, EditCommands& commands, const Schema& schema, AddChunkPopup& addPopup);

    void Draw();

private:
    void DrawNode(Chunk* chunk);
    void DrawContextMenu(Chunk* chunk);
    void HandleDragDrop(Chunk* chunk);
    bool MatchesFilter(const Chunk* chunk) const;

    Document& m_document;
    EditCommands& m_commands;
    const Schema& m_schema;
    AddChunkPopup& m_addPopup;
    char m_filter[128] = {};
    std::function<void()> m_pending;
};

}
