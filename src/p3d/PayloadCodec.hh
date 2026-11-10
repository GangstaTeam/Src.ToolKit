#pragma once

#include "core/Bytes.hh"
#include "p3d/Field.hh"
#include "p3d/Schema.hh"

#include <string>
#include <vector>

namespace tk
{

struct DecodedPayload
{
    std::vector<Field> fields;
    Bytes leftover;
    bool ok = true;
    std::string error;
};

DecodedPayload DecodePayload(const Schema& schema, const ChunkDef& def, const Bytes& payload);
Bytes EncodePayload(const DecodedPayload& decoded);
void SyncArrayCounts(std::vector<Field>& fields);
Field MakeDefaultField(const Schema& schema, const FieldDef& def);

}
