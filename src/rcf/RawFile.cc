#include "rcf/RawFile.hh"

#include <windows.h>

#include <algorithm>

namespace tk
{

bool RawFile::Open(const std::filesystem::path& path, Mode mode)
{
    Close();
    HANDLE handle = mode == Mode::Read
        ? CreateFileW(path.c_str(),
              GENERIC_READ,
              FILE_SHARE_READ | FILE_SHARE_DELETE,
              nullptr,
              OPEN_EXISTING,
              FILE_FLAG_SEQUENTIAL_SCAN,
              nullptr)
        : CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    m_handle = handle;
    return true;
}

bool RawFile::Close()
{
    if (!m_handle)
    {
        return true;
    }

    bool ok = CloseHandle(m_handle) != 0;
    m_handle = nullptr;
    return ok;
}

uint64_t RawFile::Size() const
{
    LARGE_INTEGER size;
    return m_handle && GetFileSizeEx(m_handle, &size) ? static_cast<uint64_t>(size.QuadPart) : 0;
}

bool RawFile::Seek(uint64_t offset)
{
    LARGE_INTEGER position;
    position.QuadPart = static_cast<LONGLONG>(offset);
    return m_handle && SetFilePointerEx(m_handle, position, nullptr, FILE_BEGIN);
}

size_t RawFile::ReadSome(void* data, size_t size)
{
    size_t total = 0;

    while (total < size)
    {
        DWORD request = static_cast<DWORD>(std::min<size_t>(size - total, kMaxRequest));
        DWORD got = 0;

        if (!ReadFile(m_handle, static_cast<char*>(data) + total, request, &got, nullptr) || got == 0)
        {
            break;
        }

        total += got;
    }

    return total;
}

bool RawFile::Read(void* data, size_t size)
{
    return m_handle && ReadSome(data, size) == size;
}

bool RawFile::Write(const void* data, size_t size)
{
    if (!m_handle)
    {
        return false;
    }

    size_t total = 0;

    while (total < size)
    {
        DWORD request = static_cast<DWORD>(std::min<size_t>(size - total, kMaxRequest));
        DWORD written = 0;

        if (!WriteFile(m_handle, static_cast<const char*>(data) + total, request, &written, nullptr) ||
            written != request)
        {
            return false;
        }

        total += written;
    }

    return true;
}

}
