#pragma once

#include "core/ByteWriter.hh"
#include "p3d/Field.hh"

#include <vector>

namespace tk
{

class FieldWriter
{
public:
    void WriteFields(const std::vector<Field>& fields);
    void Append(const Bytes& bytes) { m_writer.Append(bytes); }
    Bytes Take() { return m_writer.Take(); }

private:
    void WriteField(const Field& field);
    void WritePrimitive(const Field& field);
    void WriteString(const Field& field);

    ByteWriter m_writer;
};

uint32_t PaddedStringLength(size_t textLength);

}
