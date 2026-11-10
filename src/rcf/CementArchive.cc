#include "rcf/CementArchive.hh"

#include <algorithm>
#include <cstring>

namespace tk
{

namespace
{

uint32_t ReadU32(const uint8_t* p)
{
    uint32_t value;
    std::memcpy(&value, p, 4);
    return value;
}

}

bool CementArchive::Open(const std::filesystem::path& path, std::string& error)
{
    Close();

    if (!m_file.Open(path, RawFile::Mode::Read))
    {
        error = "Cannot open " + path.u8string();
        return false;
    }

    m_path = path;
    m_fileSize = m_file.Size();

    auto fail = [&](const std::string& message)
    {
        error = path.filename().u8string() + ": " + message;
        Close();
        return false;
    };

    if (m_fileSize < kCementHeaderSize || !m_file.Read(&m_header, kCementHeaderSize))
    {
        return fail("too small for a cement header");
    }

    if (std::strncmp(m_header.signature, CEMENTFORMAT_CEMENT_SIGNATURE, sizeof(m_header.signature)) != 0)
    {
        return fail("not a cement library (no \"ATG CORE CEMENT LIBRARY\" signature)");
    }

    if (m_header.versionMajor != 2 || m_header.versionMinor >= 2 || !m_header.valid || m_header.bigEndian)
    {
        return fail("unsupported cement version " + std::to_string(m_header.versionMajor) + "." +
            std::to_string(m_header.versionMinor) + (m_header.bigEndian ? " (big endian)" : ""));
    }

    if (m_header.versionMinor != kCementVersionMinor)
    {
        return fail("cement 2.0 libraries (20-byte hash entries) are not supported");
    }

    if (m_header.hashEntriesSize != m_header.fileCount * kCementHashEntrySize ||
        uint64_t(m_header.hashEntriesPos) + m_header.hashEntriesSize > m_fileSize ||
        uint64_t(m_header.nameTablePos) + m_header.nameTableSize > m_fileSize)
    {
        return fail("header tables are out of range");
    }

    std::vector<CementHashEntry> hashes(m_header.fileCount);

    if (!m_file.Seek(m_header.hashEntriesPos) || !m_file.Read(hashes.data(), m_header.hashEntriesSize))
    {
        return fail("cannot read hash table");
    }

    for (size_t i = 0; i < hashes.size(); ++i)
    {
        if (uint64_t(hashes[i].offset) + hashes[i].size > m_fileSize)
        {
            return fail("file data out of range");
        }

        if (i && hashes[i - 1].hash >= hashes[i].hash)
        {
            return fail("hash table is not sorted by hash");
        }
    }

    Bytes names(m_header.nameTableSize);

    if (!m_file.Seek(m_header.nameTablePos) || !m_file.Read(names.data(), names.size()))
    {
        return fail("cannot read name table");
    }

    if (names.size() < 8)
    {
        return fail("name table too small");
    }

    m_alignment = ReadU32(names.data());

    std::vector<bool> named(hashes.size());
    size_t pos = 8;

    for (uint32_t i = 0; i < m_header.fileCount; ++i)
    {
        if (pos + kCementNameEntryFixed > names.size())
        {
            return fail("name table truncated");
        }

        CementEntry entry;
        entry.timestamp = ReadU32(&names[pos]);
        entry.alignment = ReadU32(&names[pos + 4]);
        entry.reserved = ReadU32(&names[pos + 8]);
        uint32_t length = ReadU32(&names[pos + 12]);
        pos += kCementNameEntryFixed;

        if (length == 0 || pos + length + kCementNamePadding > names.size())
        {
            return fail("name table truncated");
        }

        entry.name.assign(reinterpret_cast<const char*>(&names[pos]), length - 1);
        pos += length + kCementNamePadding;
        entry.hash = CementHash(entry.name);
        auto it = std::lower_bound(hashes.begin(),
            hashes.end(),
            entry.hash,
            [](const CementHashEntry& e, uint32_t h)
            {
                return e.hash < h;
            });
        if (it == hashes.end() || it->hash != entry.hash)
        {
            return fail("name \"" + entry.name + "\" has no hash entry");
        }

        size_t index = static_cast<size_t>(it - hashes.begin());

        if (named[index])
        {
            return fail("name \"" + entry.name + "\" is listed twice");
        }

        named[index] = true;
        entry.offset = it->offset;
        entry.size = it->size;
        m_entries.push_back(std::move(entry));
    }

    m_byHash.resize(m_entries.size());

    for (size_t i = 0; i < m_byHash.size(); ++i)
    {
        m_byHash[i] = i;
    }

    std::sort(m_byHash.begin(),
        m_byHash.end(),
        [&](size_t a, size_t b)
        {
            return m_entries[a].hash < m_entries[b].hash;
        });
    return true;
}

void CementArchive::Close()
{
    m_file.Close();
    m_path.clear();
    m_header = {};
    m_alignment = kCementAlignment;
    m_fileSize = 0;
    m_entries.clear();
    m_byHash.clear();
}

const CementEntry* CementArchive::FindHash(uint32_t hash) const
{
    auto it = std::lower_bound(m_byHash.begin(),
        m_byHash.end(),
        hash,
        [&](size_t index, uint32_t h)
        {
            return m_entries[index].hash < h;
        });
    if (it == m_byHash.end() || m_entries[*it].hash != hash)
    {
        return nullptr;
    }

    return &m_entries[*it];
}

const CementEntry* CementArchive::Find(std::string_view name) const
{
    return FindHash(CementHash(CementNormalizeName(name)));
}

bool CementArchive::Read(const CementEntry& entry, Bytes& out, std::string& error)
{
    out.resize(entry.size);

    if (m_file.Seek(entry.offset) && m_file.Read(out.data(), entry.size))
    {
        return true;
    }

    error = "Cannot read " + entry.name;
    return false;
}

bool CementArchive::CopyTo(const CementEntry& entry,
    RawFile& out,
    const CementProgress& progress,
    uint64_t& done,
    uint64_t total,
    std::string& error)
{
    std::vector<char> block(std::min<size_t>(kCopyBlock, std::max<uint32_t>(entry.size, 1)));

    if (!m_file.Seek(entry.offset))
    {
        error = "Cannot read " + entry.name;
        return false;
    }

    uint32_t left = entry.size;

    while (left)
    {
        size_t count = std::min<size_t>(left, block.size());

        if (!m_file.Read(block.data(), count))
        {
            error = "Cannot read " + entry.name;
            return false;
        }

        if (!out.Write(block.data(), count))
        {
            error = "Cannot write " + entry.name;
            return false;
        }

        left -= static_cast<uint32_t>(count);
        done += count;

        if (progress && !progress(done, total))
        {
            error = "Cancelled";
            return false;
        }
    }

    return true;
}

}
