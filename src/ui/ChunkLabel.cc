#include "ui/ChunkLabel.hh"

#include "p3d/ChunkIo.hh"

#include <cstring>

namespace tk
{

std::string ObjectName(const Schema& schema, const Chunk& chunk)
{
    const ChunkDef* def = schema.FindChunk(chunk.Id());

    if (!def || def->fields.empty() || def->fields[0].type != "string")
    {
        return {};
    }

    const Bytes& payload = chunk.Payload();

    if (payload.empty() || payload[0] + 1u > payload.size())
    {
        return {};
    }

    std::string name(reinterpret_cast<const char*>(payload.data() + 1), payload[0]);
    name.resize(std::strlen(name.c_str()));
    return name;
}

std::string ChunkLabel(const Schema& schema, const Chunk& chunk)
{
    if (chunk.Id() == kP3dFileId && !chunk.Parent())
    {
        return "P3D File";
    }

    std::string label = schema.ChunkName(chunk.Id());
    std::string name = ObjectName(schema, chunk);

    if (!name.empty())
    {
        label += "  '" + name + "'";
    }

    return label;
}

}
