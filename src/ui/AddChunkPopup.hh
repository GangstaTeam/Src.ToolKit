#pragma once

#include "p3d/Schema.hh"

#include <cstdint>
#include <functional>
#include <string>

namespace tk
{

class AddChunkPopup
{
public:
    explicit AddChunkPopup(const Schema& schema) : m_schema(schema) {}

    void Open(const char* title, uint32_t parentId, std::function<void(uint32_t)> onPick);
    void Draw();

private:
    bool DrawEntry(uint32_t id);
    bool Matches(const ChunkDef& def) const;

    const Schema& m_schema;
    std::string m_title;
    uint32_t m_parentId = 0;
    std::function<void(uint32_t)> m_onPick;
    char m_filter[128] = {};
    char m_customId[16] = {};
    bool m_openRequested = false;
};

}
