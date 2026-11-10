#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace tk
{

struct FieldDef
{
    std::string type;
    std::string name;
    bool isArray = false;
    std::string countField;
};

struct StructDef
{
    std::string name;
    std::vector<FieldDef> fields;
};

struct ChunkDef
{
    uint32_t id = 0;
    std::string name;
    std::vector<FieldDef> fields;
    std::vector<uint32_t> subchunks;
};

class Schema
{
public:
    Schema();

    void AddChunk(ChunkDef chunk);
    void AddStruct(StructDef def);

    const ChunkDef* FindChunk(uint32_t id) const;
    const StructDef* FindStruct(const std::string& name) const;
    std::string ChunkName(uint32_t id) const;
    const std::map<uint32_t, ChunkDef>& Chunks() const { return m_chunks; }

private:
    void AddBuiltinStructs();

    std::map<uint32_t, ChunkDef> m_chunks;
    std::map<std::string, StructDef> m_structs;
};

}
