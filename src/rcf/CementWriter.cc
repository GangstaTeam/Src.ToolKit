#include "rcf/CementWriter.hh"

#include "rcf/FileTime.hh"

#include <windows.h>

#include <algorithm>
#include <cstring>
#include <map>

namespace tk
{

namespace
{

bool WriteZeros(CementSink& sink, uint64_t count)
{
    static const char zeros[kCementAlignment] = {};

    while (count)
    {
        size_t chunk = static_cast<size_t>(std::min<uint64_t>(count, sizeof(zeros)));

        if (!sink.Write(zeros, chunk))
        {
            return false;
        }

        count -= chunk;
    }

    return true;
}

bool WriteU32(CementSink& sink, uint32_t value)
{
    return sink.Write(reinterpret_cast<const char*>(&value), 4);
}

bool ValidName(const std::string& name)
{
    return !name.empty() && name[0] != '\\' && name.find('/') == std::string::npos &&
        name.find('\0') == std::string::npos;
}

}

bool CopyCementSource(const CementSource& source,
    CementSink& sink,
    const CementProgress& progress,
    uint64_t& done,
    uint64_t total,
    std::string& error)
{
    if (source.kind == CementSource::Kind::Memory)
    {
        if (!sink.Write(reinterpret_cast<const char*>(source.bytes->data()), source.bytes->size()))
        {
            error = "Cannot write " + source.name;
            return false;
        }

        done += source.size;

        if (progress && !progress(done, total))
        {
            error = "Cancelled";
            return false;
        }

        return true;
    }

    RawFile in(source.path, RawFile::Mode::Read);

    if (!in.IsOpen() || (source.kind == CementSource::Kind::ArchiveSlice && !in.Seek(source.offset)))
    {
        error = "Cannot open " + source.path.u8string();
        return false;
    }

    std::vector<char> block(std::min<size_t>(kCopyBlock, std::max<uint32_t>(source.size, 1)));
    uint32_t left = source.size;

    while (left)
    {
        size_t count = std::min<size_t>(left, block.size());

        if (!in.Read(block.data(), count))
        {
            error = "Cannot read " + source.name + " (file changed while packing?)";
            return false;
        }

        if (!sink.Write(block.data(), count))
        {
            error = "Cannot write " + source.name;
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

CementSource CementSource::FromArchive(const std::filesystem::path& archive, const CementEntry& entry)
{
    CementSource source;
    source.name = entry.name;
    source.timestamp = entry.timestamp;
    source.alignment = entry.alignment;
    source.reserved = entry.reserved;
    source.kind = Kind::ArchiveSlice;
    source.path = archive;
    source.offset = entry.offset;
    source.size = entry.size;
    return source;
}

CementSource CementSource::FromFile(std::string name, const std::filesystem::path& file)
{
    CementSource source;
    source.name = std::move(name);
    source.timestamp = GetFileUnixTime(file).value_or(CurrentUnixTime());
    source.kind = Kind::DiskFile;
    source.path = file;
    std::error_code ec;
    uint64_t size = std::filesystem::file_size(file, ec);
    source.size = !ec && size <= UINT32_MAX ? static_cast<uint32_t>(size) : 0;
    return source;
}

CementSource CementSource::FromMemory(std::string name, uint32_t timestamp, std::shared_ptr<const Bytes> bytes)
{
    CementSource source;
    source.name = std::move(name);
    source.timestamp = timestamp;
    source.kind = Kind::Memory;
    source.size = static_cast<uint32_t>(bytes->size());
    source.bytes = std::move(bytes);
    return source;
}

bool CementFileSink::Write(const char* data, size_t size)
{
    if (m_failed)
    {
        return false;
    }

    if (m_buffer.size() + size > kSinkBuffer && !Flush())
    {
        return false;
    }

    if (size >= kSinkBuffer)
    {
        m_failed = !m_file.Write(data, size);
        return !m_failed;
    }

    m_buffer.insert(m_buffer.end(), data, data + size);
    return true;
}

bool CementFileSink::Flush()
{
    if (!m_buffer.empty() && !m_file.Write(m_buffer.data(), m_buffer.size()))
    {
        m_failed = true;
    }

    m_buffer.clear();
    return !m_failed;
}

bool CementFileSink::Close()
{
    bool ok = Flush();
    return m_file.Close() && ok;
}

bool CementCompareSink::Write(const char* data, size_t size)
{
    while (size && m_mismatch == UINT64_MAX)
    {
        if (m_bufferPos == m_bufferEnd)
        {
            m_buffer.resize(kSinkBuffer);
            m_bufferPos = 0;
            m_bufferEnd = m_file.ReadSome(m_buffer.data(), m_buffer.size());

            if (m_bufferEnd == 0)
            {
                m_mismatch = m_position;
                break;
            }
        }

        size_t count = std::min(size, m_bufferEnd - m_bufferPos);
        const char* original = m_buffer.data() + m_bufferPos;

        for (size_t i = 0; i < count; ++i)
        {
            if (original[i] != data[i])
            {
                m_mismatch = m_position + i;
                return true;
            }
        }

        m_bufferPos += count;
        m_position += count;
        data += count;
        size -= count;
    }

    return true;
}

bool CementCompareSink::MatchedToEnd()
{
    if (m_mismatch != UINT64_MAX)
    {
        return false;
    }

    char extra;

    if (m_bufferPos < m_bufferEnd || m_file.ReadSome(&extra, 1))
    {
        m_mismatch = m_position;
        return false;
    }

    return true;
}

bool LayoutCement(std::vector<CementSource>& sources, uint32_t alignment, CementLayout& layout, std::string& error)
{
    std::map<uint32_t, const std::string*> hashes;
    uint64_t nameTableSize = 8;

    for (CementSource& source : sources)
    {
        if (!ValidName(source.name))
        {
            error = "Invalid name \"" + source.name + "\" (use backslashes, no leading '\\')";
            return false;
        }

        uint32_t hash = CementHash(source.name);
        auto [it, inserted] = hashes.emplace(hash, &source.name);

        if (!inserted)
        {
            error = "\"" + source.name + "\" and \"" + *it->second + "\" have the same cement hash";
            return false;
        }

        if (source.kind == CementSource::Kind::DiskFile)
        {
            std::error_code ec;
            uint64_t size = std::filesystem::file_size(source.path, ec);

            if (ec || size > UINT32_MAX)
            {
                error = "Cannot read " + source.path.u8string();
                return false;
            }

            source.size = static_cast<uint32_t>(size);
        }

        nameTableSize += kCementNameEntryFixed + source.name.size() + 1 + kCementNamePadding;
    }

    uint64_t hashSize = uint64_t(sources.size()) * kCementHashEntrySize;
    uint64_t namePos = CementAlign(kCementHeaderSize + hashSize, alignment);
    uint64_t cursor = CementAlign(namePos + nameTableSize, alignment);
    layout.hashEntriesPos = kCementHeaderSize;
    layout.nameTablePos = static_cast<uint32_t>(namePos);
    layout.nameTableSize = static_cast<uint32_t>(nameTableSize);
    layout.dataPos = static_cast<uint32_t>(cursor);
    layout.offsets.clear();

    for (const CementSource& source : sources)
    {
        cursor = CementAlign(cursor, source.alignment);
        layout.offsets.push_back(static_cast<uint32_t>(cursor));
        cursor += source.size;

        if (cursor > UINT32_MAX)
        {
            error = "Archive would be larger than 4 GB, which the 32-bit cement offsets cannot address";
            return false;
        }
    }

    layout.totalSize = sources.empty() ? namePos + nameTableSize : cursor;
    return true;
}

bool WriteCement(std::vector<CementSource>& sources,
    uint32_t alignment,
    CementSink& sink,
    const CementProgress& progress,
    std::string& error)
{
    CementLayout layout;

    if (!LayoutCement(sources, alignment, layout, error))
    {
        return false;
    }

    auto failWrite = [&]
    {
        error = "Write failed (disk full?)";
        return false;
    };

    CementHeader header = {};
    std::memcpy(header.signature, CEMENTFORMAT_CEMENT_SIGNATURE, sizeof(CEMENTFORMAT_CEMENT_SIGNATURE));
    header.versionMajor = kCementVersionMajor;
    header.versionMinor = kCementVersionMinor;
    header.bigEndian = 0;
    header.valid = 1;
    header.hashEntriesPos = layout.hashEntriesPos;
    header.hashEntriesSize = static_cast<uint32_t>(sources.size() * kCementHashEntrySize);
    header.nameTablePos = layout.nameTablePos;
    header.nameTableSize = layout.nameTableSize;
    header.driveBufferSize = 0;
    header.fileCount = static_cast<uint32_t>(sources.size());

    if (!sink.Write(reinterpret_cast<const char*>(&header), sizeof(header)))
    {
        return failWrite();
    }

    std::vector<CementHashEntry> table;

    for (size_t i = 0; i < sources.size(); ++i)
    {
        table.push_back({CementHash(sources[i].name), layout.offsets[i], sources[i].size});
    }

    std::sort(table.begin(),
        table.end(),
        [](const CementHashEntry& a, const CementHashEntry& b)
        {
            return a.hash < b.hash;
        });
    if (!table.empty() &&
        !sink.Write(reinterpret_cast<const char*>(table.data()), table.size() * sizeof(CementHashEntry)))
    {
        return failWrite();
    }

    uint64_t position = kCementHeaderSize + table.size() * sizeof(CementHashEntry);

    if (!WriteZeros(sink, layout.nameTablePos - position))
    {
        return failWrite();
    }

    if (!WriteU32(sink, alignment) || !WriteU32(sink, 0))
    {
        return failWrite();
    }

    for (const CementSource& source : sources)
    {
        uint32_t length = static_cast<uint32_t>(source.name.size() + 1);

        if (!WriteU32(sink, source.timestamp) || !WriteU32(sink, source.alignment) ||
            !WriteU32(sink, source.reserved) || !WriteU32(sink, length) || !sink.Write(source.name.c_str(), length) ||
            !WriteZeros(sink, kCementNamePadding))
        {
            return failWrite();
        }
    }

    position = uint64_t(layout.nameTablePos) + layout.nameTableSize;

    uint64_t total = 0;

    for (const CementSource& source : sources)
    {
        total += source.size;
    }

    uint64_t done = 0;

    if (progress && !progress(0, total))
    {
        error = "Cancelled";
        return false;
    }

    for (size_t i = 0; i < sources.size(); ++i)
    {
        if (!WriteZeros(sink, layout.offsets[i] - position))
        {
            return failWrite();
        }

        if (!CopyCementSource(sources[i], sink, progress, done, total, error))
        {
            return false;
        }

        position = uint64_t(layout.offsets[i]) + sources[i].size;
    }

    return true;
}

bool SaveCement(std::vector<CementSource>& sources,
    uint32_t alignment,
    const std::filesystem::path& target,
    const CementProgress& progress,
    std::string& error,
    const std::function<void()>& beforeReplace)
{
    std::filesystem::path temp = target;
    temp += L".tmp";
    {
        CementFileSink sink(temp);

        if (!sink.IsOpen())
        {
            error = "Cannot create " + temp.u8string();
            return false;
        }

        bool ok = WriteCement(sources, alignment, sink, progress, error);

        if (!sink.Close() && ok)
        {
            error = "Write failed (disk full?)";
            ok = false;
        }

        if (!ok)
        {
            std::error_code ignored;
            std::filesystem::remove(temp, ignored);
            return false;
        }
    }

    if (beforeReplace)
    {
        beforeReplace();
    }

    if (!MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        error = "Cannot replace " + target.u8string() + " (is the game or another program using it?)";
        std::error_code ignored;
        std::filesystem::remove(temp, ignored);
        return false;
    }

    return true;
}

std::vector<CementSource> SourcesFromArchive(const CementArchive& archive)
{
    std::vector<CementSource> sources;

    for (const CementEntry& entry : archive.Entries())
    {
        sources.push_back(CementSource::FromArchive(archive.Path(), entry));
    }

    return sources;
}

}
