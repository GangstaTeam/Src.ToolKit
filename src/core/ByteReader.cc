#include "core/ByteReader.hh"

#include <cstring>

namespace tk
{

ByteReader::ByteReader(const uint8_t* data, size_t size) : m_data(data), m_size(size)
{
}

ByteReader::ByteReader(const Bytes& bytes) : ByteReader(bytes.data(), bytes.size())
{
}

bool ByteReader::Need(size_t count)
{
    if (m_failed || m_size - m_position < count)
    {
        m_failed = true;
        return false;
    }

    return true;
}

uint8_t ByteReader::U8()
{
    if (!Need(1))
    {
        return 0;
    }

    return m_data[m_position++];
}

uint16_t ByteReader::U16()
{
    uint16_t value = 0;

    if (Need(2))
    {
        std::memcpy(&value, m_data + m_position, 2);
        m_position += 2;
    }

    return value;
}

uint32_t ByteReader::U32()
{
    uint32_t value = 0;

    if (Need(4))
    {
        std::memcpy(&value, m_data + m_position, 4);
        m_position += 4;
    }

    return value;
}

float ByteReader::F32()
{
    uint32_t bits = U32();
    float value;
    std::memcpy(&value, &bits, 4);
    return value;
}

Bytes ByteReader::Take(size_t count)
{
    if (!Need(count))
    {
        return {};
    }

    Bytes out(m_data + m_position, m_data + m_position + count);
    m_position += count;
    return out;
}

}
