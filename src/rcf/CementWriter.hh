#pragma once

#include "core/Bytes.hh"
#include "rcf/CementArchive.hh"
#include "rcf/RawFile.hh"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tk
{

enum ECementWriterSize : size_t
{
    kSinkBuffer = 4u << 20,
};

struct CementSource
{
    enum class Kind
    {
        ArchiveSlice,
        DiskFile,
        Memory
    };

    std::string name;
    uint32_t timestamp = 0;
    uint32_t alignment = kCementAlignment;
    uint32_t reserved = 0;

    Kind kind = Kind::DiskFile;
    std::filesystem::path path;
    uint32_t offset = 0;
    uint32_t size = 0;
    std::shared_ptr<const Bytes> bytes;

    static CementSource FromArchive(const std::filesystem::path& archive, const CementEntry& entry);
    static CementSource FromFile(std::string name, const std::filesystem::path& file);
    static CementSource FromMemory(std::string name, uint32_t timestamp, std::shared_ptr<const Bytes> bytes);
};

class CementSink
{
public:
    virtual ~CementSink() = default;
    virtual bool Write(const char* data, size_t size) = 0;
};

class CementFileSink : public CementSink
{
public:
    explicit CementFileSink(const std::filesystem::path& path) : m_file(path, RawFile::Mode::Create) {}
    bool IsOpen() const { return m_file.IsOpen(); }
    bool Write(const char* data, size_t size) override;
    bool Close();

private:
    bool Flush();

    RawFile m_file;
    std::vector<char> m_buffer;
    bool m_failed = false;
};

class CementCompareSink : public CementSink
{
public:
    explicit CementCompareSink(const std::filesystem::path& path) : m_file(path, RawFile::Mode::Read) {}
    bool IsOpen() const { return m_file.IsOpen(); }
    bool Write(const char* data, size_t size) override;
    bool MatchedToEnd();
    uint64_t MismatchOffset() const { return m_mismatch; }

private:
    RawFile m_file;
    std::vector<char> m_buffer;
    size_t m_bufferPos = 0;
    size_t m_bufferEnd = 0;
    uint64_t m_position = 0;
    uint64_t m_mismatch = UINT64_MAX;
};

struct CementLayout
{
    uint32_t hashEntriesPos = 0;
    uint32_t nameTablePos = 0;
    uint32_t nameTableSize = 0;
    uint32_t dataPos = 0;
    uint64_t totalSize = 0;
    std::vector<uint32_t> offsets;
};

bool LayoutCement(std::vector<CementSource>& sources, uint32_t alignment, CementLayout& layout, std::string& error);
bool WriteCement(std::vector<CementSource>& sources,
    uint32_t alignment,
    CementSink& sink,
    const CementProgress& progress,
    std::string& error);

bool SaveCement(std::vector<CementSource>& sources,
    uint32_t alignment,
    const std::filesystem::path& target,
    const CementProgress& progress,
    std::string& error,
    const std::function<void()>& beforeReplace = nullptr);

bool CopyCementSource(const CementSource& source,
    CementSink& sink,
    const CementProgress& progress,
    uint64_t& done,
    uint64_t total,
    std::string& error);

std::vector<CementSource> SourcesFromArchive(const CementArchive& archive);

}
