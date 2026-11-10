#include "doc/ChunkFactory.hh"

#include "p3d/PayloadCodec.hh"

namespace tk
{

std::unique_ptr<Chunk> CreateChunk(const Schema& schema, uint32_t id)
{
    auto chunk = std::make_unique<Chunk>(id);
    const ChunkDef* def = schema.FindChunk(id);

    if (!def)
    {
        return chunk;
    }

    DecodedPayload payload;

    for (const FieldDef& field : def->fields)
    {
        payload.fields.push_back(MakeDefaultField(schema, field));
    }

    chunk->SetPayload(EncodePayload(payload));
    return chunk;
}

}
