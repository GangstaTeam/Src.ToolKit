#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#define CEMENTFORMAT_CEMENT_SIGNATURE "ATG CORE CEMENT LIBRARY"

namespace tk
{

enum ECementFormatByte : uint8_t
{
    kCementVersionMajor = 2,
    kCementVersionMinor = 1,
};

enum ECementFormatValue : uint32_t
{
    kCementHeaderSize = 0x3C,
    kCementHashEntrySize = 12,
    kCementNameEntryFixed = 16,
    kCementNamePadding = 3,
    kCementAlignment = 0x800,
};

#pragma pack(push, 1)
struct CementHeader
{
    char signature[32];
    uint8_t versionMajor;
    uint8_t versionMinor;
    uint8_t bigEndian;
    uint8_t valid;
    uint32_t hashEntriesPos;
    uint32_t hashEntriesSize;
    uint32_t nameTablePos;
    uint32_t nameTableSize;
    uint32_t driveBufferSize;
    uint32_t fileCount;
};

struct CementHashEntry
{
    uint32_t hash;
    uint32_t offset;
    uint32_t size;
};
#pragma pack(pop)

static_assert(sizeof(CementHeader) == kCementHeaderSize);
static_assert(sizeof(CementHashEntry) == kCementHashEntrySize);

uint32_t CementHash(std::string_view name);

std::string CementNormalizeName(std::string_view name);

uint64_t CementAlign(uint64_t value, uint32_t alignment);

}
