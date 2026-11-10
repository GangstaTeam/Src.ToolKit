#include "p3d/PayloadCodec.hh"

#include "p3d/FieldReader.hh"
#include "p3d/FieldWriter.hh"

namespace tk
{

DecodedPayload DecodePayload(const Schema& schema, const ChunkDef& def, const Bytes& payload)
{
    DecodedPayload out;
    FieldReader reader(schema, payload);
    out.ok = reader.ReadFields(def.fields, out.fields, out.error);
    out.leftover = reader.Rest();

    if (out.ok && !out.leftover.empty())
    {
        out.error = std::to_string(out.leftover.size()) + " byte(s) not covered by the schema";
    }

    return out;
}

Bytes EncodePayload(const DecodedPayload& decoded)
{
    FieldWriter writer;
    writer.WriteFields(decoded.fields);
    writer.Append(decoded.leftover);
    return writer.Take();
}

void SyncArrayCounts(std::vector<Field>& fields)
{
    for (Field& field : fields)
    {
        if (field.isArray)
        {
            if (Field* count = FindField(fields, field.countField))
            {
                count->integer = static_cast<uint32_t>(field.items.size());
            }

            for (Field& item : field.items)
            {
                SyncArrayCounts(item.items);
            }
        }
        else
        {
            SyncArrayCounts(field.items);
        }
    }
}

Field MakeDefaultField(const Schema& schema, const FieldDef& def)
{
    Field field;
    field.name = def.name;
    field.type = def.type;
    field.isArray = def.isArray;
    field.countField = def.countField;
    field.primitive = PrimitiveFromType(def.type);

    if (def.isArray || field.primitive != Primitive::None)
    {
        return field;
    }

    if (const StructDef* structDef = schema.FindStruct(def.type))
    {
        for (const FieldDef& member : structDef->fields)
        {
            field.items.push_back(MakeDefaultField(schema, member));
        }
    }

    return field;
}

}
