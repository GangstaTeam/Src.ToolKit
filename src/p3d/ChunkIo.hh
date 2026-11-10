#pragma once

#include <cstddef>
#include <cstdint>

#include "core/Bytes.hh"
#include "p3d/Chunk.hh"

#include <memory>
#include <string>

namespace tk
{

enum EChunkIoValue : uint32_t
{
    kP3dFileId = 0xFF443350,
    kP3dCompressedId = 0x5A443350,
};

struct ParseResult
{
    std::unique_ptr<Chunk> root;
    std::string error;
};

ParseResult ParseP3d(const Bytes& bytes);
Bytes SerializeP3d(const Chunk& root);
std::unique_ptr<Chunk> MakeEmptyFile();

}
