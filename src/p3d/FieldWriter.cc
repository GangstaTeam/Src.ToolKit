#include "p3d/FieldWriter.hh"

namespace tk
{

uint32_t PaddedStringLength(size_t textLength)
{
    size_t length = textLength + 1;

    while ((length + 1) % 4 != 0)
    {
        ++length;
    }

    return static_cast<uint32_t>(length);
}

void FieldWriter::WriteFields(const std::vector<Field>& fields)
{
    for (const Field& field : fields)
    {
        WriteField(field);
    }
}

void FieldWriter::WriteField(const Field& field)
{
    if (field.isArray || field.primitive == Primitive::None)
    {
        for (const Field& item : field.items)
        {
            WriteField(item);
        }

        return;
    }

    WritePrimitive(field);
}

void FieldWriter::WritePrimitive(const Field& field)
{
    switch (field.primitive)
    {
    case Primitive::U8:
        m_writer.U8(static_cast<uint8_t>(field.integer));
        break;
    case Primitive::U16:
    case Primitive::I16:
        m_writer.U16(static_cast<uint16_t>(field.integer));
        break;
    case Primitive::U32:
    case Primitive::I32:
    case Primitive::FourCC:
    case Primitive::Colour:
        m_writer.U32(field.integer);
        break;
    case Primitive::F32:
        m_writer.F32(field.real);
        break;
    case Primitive::String:
    case Primitive::LongString:
        WriteString(field);
        break;
    case Primitive::Raw:
        m_writer.Append(field.raw);
        break;
    case Primitive::None:
        break;
    }
}

void FieldWriter::WriteString(const Field& field)
{
    uint32_t length =
        field.text.size() <= field.storedLength ? field.storedLength : PaddedStringLength(field.text.size());
    if (field.primitive == Primitive::String)
    {
        m_writer.U8(static_cast<uint8_t>(length));
    }
    else
    {
        m_writer.U32(length);
    }

    Bytes chars(length, 0);
    std::copy(field.text.begin(), field.text.end(), chars.begin());
    m_writer.Append(chars);
}

}
