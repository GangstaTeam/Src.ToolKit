#pragma once

#include "core/Bytes.hh"
#include "rcf/CementFormat.hh"
#include "rcf/RawFile.hh"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace tk
{

enum ECementArchiveSize : size_t
{
    kCopyBlock = 4u << 20,
};

using CementProgress = std::function<bool(uint64_t done, uint64_t total)>;

struct CementEntry
{
    std::string name;
    uint32_t hash = 0;
    uint32_t offset = 0;
    uint32_t size = 0;
    uint32_t timestamp = 0;
    uint32_t alignment = kCementAlignment;
    uint32_t reserved = 0;
};

class CementArchive
{
public:
    bool Open(const std::filesystem::path& path, std::string& error);
    void Close();

    bool IsOpen() const { return m_file.IsOpen(); }
    const std::filesystem::path& Path() const { return m_path; }
    const CementHeader& Header() const { return m_header; }
    uint32_t Alignment() const { return m_alignment; }
    uint64_t FileSize() const { return m_fileSize; }

    const std::vector<CementEntry>& Entries() const { return m_entries; }
    const CementEntry* Find(std::string_view name) const;
    const CementEntry* FindHash(uint32_t hash) const;

    bool Read(const CementEntry& entry, Bytes& out, std::string& error);
    bool CopyTo(const CementEntry& entry,
        RawFile& out,
        const CementProgress& progress,
        uint64_t& done,
        uint64_t total,
        std::string& error);

private:
    RawFile m_file;
    std::filesystem::path m_path;
    CementHeader m_header = {};
    uint32_t m_alignment = kCementAlignment;
    uint64_t m_fileSize = 0;
    std::vector<CementEntry> m_entries;
    std::vector<size_t> m_byHash;
};

}
