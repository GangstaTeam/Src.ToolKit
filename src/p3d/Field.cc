#include "p3d/Field.hh"

#include <unordered_map>

namespace tk
{

Primitive PrimitiveFromType(const std::string& type)
{
    static const std::unordered_map<std::string, Primitive> kTypes = {
        {"UBYTE", Primitive::U8},
        {"BYTE", Primitive::U8},
        {"UWORD", Primitive::U16},
        {"WORD", Primitive::I16},
        {"ULONG", Primitive::U32},
        {"LONG", Primitive::I32},
        {"float", Primitive::F32},
        {"string", Primitive::String},
        {"longstring", Primitive::LongString},
        {"FOURCC", Primitive::FourCC},
        {"COLOUR", Primitive::Colour},
        {"tlColour", Primitive::Colour},
        {"raw", Primitive::Raw},
    };
    auto it = kTypes.find(type);
    return it == kTypes.end() ? Primitive::None : it->second;
}

size_t PrimitiveMinSize(Primitive primitive)
{
    switch (primitive)
    {
    case Primitive::U8:
        return 1;
    case Primitive::U16:
    case Primitive::I16:
        return 2;
    case Primitive::String:
        return 1;
    case Primitive::Raw:
    case Primitive::None:
        return 0;
    default:
        return 4;
    }
}

bool IsInteger(Primitive primitive)
{
    switch (primitive)
    {
    case Primitive::U8:
    case Primitive::U16:
    case Primitive::I16:
    case Primitive::U32:
    case Primitive::I32:
    case Primitive::FourCC:
    case Primitive::Colour:
        return true;
    default:
        return false;
    }
}

Field* FindField(std::vector<Field>& fields, const std::string& name)
{
    for (Field& field : fields)
    {
        if (field.name == name)
        {
            return &field;
        }
    }

    return nullptr;
}

const Field* FindField(const std::vector<Field>& fields, const std::string& name)
{
    for (const Field& field : fields)
    {
        if (field.name == name)
        {
            return &field;
        }
    }

    return nullptr;
}

}
