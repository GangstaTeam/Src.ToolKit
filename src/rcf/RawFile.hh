#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace tk
{

enum ERawFileSize : size_t
{
    kMaxRequest = 64u << 20,
};

class RawFile
{
public:
    enum class Mode
    {
        Read,
        Create
    };

    RawFile() = default;
    RawFile(const std::filesystem::path& path, Mode mode) { Open(path, mode); }
    ~RawFile() { Close(); }
    RawFile(const RawFile&) = delete;
    RawFile& operator=(const RawFile&) = delete;

    bool Open(const std::filesystem::path& path, Mode mode);
    bool Close();
    bool IsOpen() const { return m_handle != nullptr; }

    uint64_t Size() const;
    bool Seek(uint64_t offset);
    bool Read(void* data, size_t size);
    size_t ReadSome(void* data, size_t size);
    bool Write(const void* data, size_t size);

private:
    void* m_handle = nullptr;
};

}
