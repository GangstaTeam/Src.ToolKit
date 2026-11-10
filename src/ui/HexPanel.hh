#pragma once

#include "doc/Document.hh"

#include <cstdint>

namespace tk
{

class HexPanel
{
public:
    explicit HexPanel(Document& document) : m_document(document) {}

    void Draw();

private:
    Document& m_document;
    const Chunk* m_cachedChunk = nullptr;
    uint64_t m_cachedRevision = 0;
    Bytes m_buffer;
    bool m_changed = false;
};

}
