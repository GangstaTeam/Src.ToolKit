#pragma once

#include "doc/Document.hh"
#include "p3d/PayloadCodec.hh"
#include "p3d/Schema.hh"
#include "ui/FieldEditor.hh"

#include <cstdint>

namespace tk
{

class PropertiesPanel
{
public:
    PropertiesPanel(Document& document, const Schema& schema, const char* title = "Properties");

    void Draw();

private:
    void Refresh(Chunk* chunk);
    void DrawHeader(Chunk* chunk);
    void DrawStatus();

    Document& m_document;
    const Schema& m_schema;
    const char* m_title;
    FieldEditor m_editor;
    const Chunk* m_cachedChunk = nullptr;
    uint64_t m_cachedRevision = 0;
    const ChunkDef* m_def = nullptr;
    DecodedPayload m_decoded;
};

}
