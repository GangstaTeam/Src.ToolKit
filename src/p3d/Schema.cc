#include "p3d/Schema.hh"

#include <cstdio>

namespace tk
{

namespace
{

StructDef FloatStruct(const std::string& name, std::initializer_list<const char*> members)
{
    StructDef def{name, {}};

    for (const char* member : members)
    {
        def.fields.push_back({"float", member});
    }

    return def;
}

StructDef MatrixStruct()
{
    StructDef def{"tlMatrix", {}};

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            def.fields.push_back({"float", "m" + std::to_string(row) + std::to_string(col)});
        }
    }

    return def;
}

}

Schema::Schema()
{
    AddBuiltinStructs();
}

void Schema::AddBuiltinStructs()
{
    AddStruct(FloatStruct("tlPoint", {"X", "Y", "Z"}));
    AddStruct(FloatStruct("tlPoint2D", {"X", "Y"}));
    AddStruct(FloatStruct("tlUV", {"U", "V"}));
    AddStruct(FloatStruct("tlQuat", {"W", "X", "Y", "Z"}));
    AddStruct(FloatStruct("tlSphere", {"X", "Y", "Z", "Radius"}));
    AddStruct(FloatStruct("tlBox", {"MinX", "MinY", "MinZ", "MaxX", "MaxY", "MaxZ"}));
    AddStruct(MatrixStruct());
    AddStruct({"tlCompressedQuat", {{"UWORD", "W"}, {"UWORD", "X"}, {"UWORD", "Y"}, {"UWORD", "Z"}}});
}

void Schema::AddChunk(ChunkDef chunk)
{
    m_chunks[chunk.id] = std::move(chunk);
}

void Schema::AddStruct(StructDef def)
{
    m_structs[def.name] = std::move(def);
}

const ChunkDef* Schema::FindChunk(uint32_t id) const
{
    auto it = m_chunks.find(id);
    return it == m_chunks.end() ? nullptr : &it->second;
}

const StructDef* Schema::FindStruct(const std::string& name) const
{
    auto it = m_structs.find(name);
    return it == m_structs.end() ? nullptr : &it->second;
}

std::string Schema::ChunkName(uint32_t id) const
{
    if (const ChunkDef* def = FindChunk(id))
    {
        return def->name;
    }

    char text[16];
    std::snprintf(text, sizeof(text), "0x%X", id);
    return text;
}

}
