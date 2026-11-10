#include "p3d/FieldReader.hh"

#include <algorithm>
#include <cstring>

namespace tk
{

FieldReader::FieldReader(const Schema& schema, const Bytes& payload) : m_schema(schema), m_reader(payload)
{
}

Bytes FieldReader::Rest()
{
    return m_reader.Take(m_reader.Remaining());
}

bool FieldReader::ReadFields(const std::vector<FieldDef>& defs, std::vector<Field>& out, std::string& error)
{
    for (const FieldDef& def : defs)
    {
        Field field;

        if (!ReadField(def, out, field, error))
        {
            return false;
        }

        out.push_back(std::move(field));
    }

    return true;
}

bool FieldReader::ReadField(const FieldDef& def, const std::vector<Field>& scope, Field& out, std::string& error)
{
    out.name = def.name;
    out.type = def.type;
    out.isArray = def.isArray;
    out.countField = def.countField;

    if (def.isArray)
    {
        return ReadArray(def, scope, out, error);
    }

    if (!ReadValue(def.type, out, error))
    {
        if (error.empty())
        {
            error = "payload ends inside field '" + def.name + "'";
        }

        return false;
    }

    return true;
}

bool FieldReader::ReadArray(const FieldDef& def, const std::vector<Field>& scope, Field& out, std::string& error)
{
    const Field* count = FindField(scope, def.countField);

    if (!count)
    {
        error = "array '" + def.name + "' refers to unknown count '" + def.countField + "'";
        return false;
    }

    size_t minSize = std::max<size_t>(MinSize(def.type), 1);

    if (count->integer > m_reader.Remaining() / minSize)
    {
        error = "array '" + def.name + "' count " + std::to_string(count->integer) + " exceeds the payload";
        return false;
    }

    out.items.resize(count->integer);

    for (uint32_t i = 0; i < count->integer; ++i)
    {
        Field& item = out.items[i];
        item.name = "[" + std::to_string(i) + "]";
        item.type = def.type;

        if (!ReadValue(def.type, item, error))
        {
            if (error.empty())
            {
                error = "payload ends inside array '" + def.name + "'";
            }

            return false;
        }
    }

    return true;
}

size_t FieldReader::MinSize(const std::string& type) const
{
    Primitive primitive = PrimitiveFromType(type);

    if (primitive != Primitive::None)
    {
        return PrimitiveMinSize(primitive);
    }

    const StructDef* def = m_schema.FindStruct(type);

    if (!def)
    {
        return 0;
    }

    size_t total = 0;

    for (const FieldDef& member : def->fields)
    {
        if (!member.isArray)
        {
            total += MinSize(member.type);
        }
    }

    return total;
}

bool FieldReader::ReadValue(const std::string& type, Field& out, std::string& error)
{
    out.primitive = PrimitiveFromType(type);

    if (out.primitive != Primitive::None)
    {
        return ReadPrimitive(out);
    }

    const StructDef* def = m_schema.FindStruct(type);

    if (!def)
    {
        error = "unknown type '" + type + "'";
        return false;
    }

    return ReadFields(def->fields, out.items, error);
}

bool FieldReader::ReadPrimitive(Field& out)
{
    switch (out.primitive)
    {
    case Primitive::U8:
        out.integer = m_reader.U8();
        break;
    case Primitive::U16:
        out.integer = m_reader.U16();
        break;
    case Primitive::I16:
        out.integer = static_cast<uint32_t>(static_cast<int16_t>(m_reader.U16()));
        break;
    case Primitive::U32:
    case Primitive::I32:
    case Primitive::FourCC:
    case Primitive::Colour:
        out.integer = m_reader.U32();
        break;
    case Primitive::F32:
        out.real = m_reader.F32();
        break;
    case Primitive::String:
    case Primitive::LongString:
    {
        out.storedLength = out.primitive == Primitive::String ? m_reader.U8() : m_reader.U32();
        Bytes chars = m_reader.Take(out.storedLength);
        out.text.assign(chars.begin(), chars.end());
        out.text.resize(std::strlen(out.text.c_str()));
        break;
    }
    case Primitive::Raw:
        out.raw = m_reader.Take(m_reader.Remaining());
        break;
    case Primitive::None:
        break;
    }

    return !m_reader.Failed();
}

}
