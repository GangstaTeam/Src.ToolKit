#pragma once

#include "core/Bytes.hh"

#include <cstddef>
#include <cstdint>

namespace tk
{

class ByteWriter
{
public:
    void U8(uint8_t value);
    void U16(uint16_t value);
    void U32(uint32_t value);
    void F32(float value);
    void Append(const Bytes& bytes);
    void Append(const uint8_t* data, size_t size);
    void PatchU32(size_t offset, uint32_t value);

    size_t Size() const { return m_bytes.size(); }
    const Bytes& Data() const { return m_bytes; }
    Bytes Take() { return std::move(m_bytes); }

private:
    Bytes m_bytes;
};

}
