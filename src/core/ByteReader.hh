#pragma once

#include "core/Bytes.hh"

#include <cstddef>
#include <cstdint>
#include <string>

namespace tk
{

class ByteReader
{
public:
    ByteReader(const uint8_t* data, size_t size);
    explicit ByteReader(const Bytes& bytes);

    uint8_t U8();
    uint16_t U16();
    uint32_t U32();
    float F32();
    Bytes Take(size_t count);

    size_t Position() const { return m_position; }
    size_t Remaining() const { return m_failed ? 0 : m_size - m_position; }
    bool Failed() const { return m_failed; }
    const uint8_t* Cursor() const { return m_data + m_position; }

private:
    bool Need(size_t count);

    const uint8_t* m_data;
    size_t m_size;
    size_t m_position = 0;
    bool m_failed = false;
};

}
