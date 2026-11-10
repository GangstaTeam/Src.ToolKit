#include "core/ByteWriter.hh"

#include <cstring>

namespace tk
{

void ByteWriter::U8(uint8_t value)
{
    m_bytes.push_back(value);
}

void ByteWriter::U16(uint16_t value)
{
    Append(reinterpret_cast<const uint8_t*>(&value), 2);
}

void ByteWriter::U32(uint32_t value)
{
    Append(reinterpret_cast<const uint8_t*>(&value), 4);
}

void ByteWriter::F32(float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, 4);
    U32(bits);
}

void ByteWriter::Append(const Bytes& bytes)
{
    m_bytes.insert(m_bytes.end(), bytes.begin(), bytes.end());
}

void ByteWriter::Append(const uint8_t* data, size_t size)
{
    m_bytes.insert(m_bytes.end(), data, data + size);
}

void ByteWriter::PatchU32(size_t offset, uint32_t value)
{
    std::memcpy(m_bytes.data() + offset, &value, 4);
}

}
