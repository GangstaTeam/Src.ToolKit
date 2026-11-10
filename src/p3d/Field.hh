#pragma once

#include "core/Bytes.hh"

#include <cstdint>
#include <string>
#include <vector>

namespace tk
{

enum class Primitive
{
    None,
    U8,
    U16,
    I16,
    U32,
    I32,
    F32,
    String,
    LongString,
    FourCC,
    Colour,
    Raw
};

Primitive PrimitiveFromType(const std::string& type);
size_t PrimitiveMinSize(Primitive primitive);
bool IsInteger(Primitive primitive);

struct Field
{
    std::string name;
    std::string type;
    Primitive primitive = Primitive::None;
    bool isArray = false;
    std::string countField;

    uint32_t integer = 0;
    float real = 0.0f;
    std::string text;
    Bytes raw;
    uint32_t storedLength = 0;

    std::vector<Field> items;
};

Field* FindField(std::vector<Field>& fields, const std::string& name);
const Field* FindField(const std::vector<Field>& fields, const std::string& name);

}
