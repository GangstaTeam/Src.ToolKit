#include "rcf/FileTime.hh"

#include <windows.h>

#include <chrono>
#include <ctime>

namespace tk
{

namespace
{

using FileTimeTicks = std::chrono::duration<uint64_t, std::ratio<1, 10000000>>;

uint64_t ToTicks(const FILETIME& time)
{
    return (uint64_t(time.dwHighDateTime) << 32) | time.dwLowDateTime;
}

uint64_t UnixEpochTicks()
{
    SYSTEMTIME epoch = {};
    epoch.wYear = 1970;
    epoch.wMonth = 1;
    epoch.wDay = 1;

    FILETIME time = {};
    SystemTimeToFileTime(&epoch, &time);
    return ToTicks(time);
}

}

std::optional<uint32_t> GetFileUnixTime(const std::filesystem::path& path)
{
    WIN32_FILE_ATTRIBUTE_DATA data;

    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data))
    {
        return std::nullopt;
    }

    uint64_t ticks = ToTicks(data.ftLastWriteTime);
    uint64_t epoch = UnixEpochTicks();

    if (ticks < epoch)
    {
        return 0u;
    }

    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(FileTimeTicks(ticks - epoch));
    return static_cast<uint32_t>(seconds.count());
}

bool SetFileUnixTime(const std::filesystem::path& path, uint32_t unixTime)
{
    HANDLE file = CreateFileW(path.c_str(),
        FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    auto sinceEpoch = std::chrono::duration_cast<FileTimeTicks>(std::chrono::seconds(unixTime));
    uint64_t ticks = UnixEpochTicks() + sinceEpoch.count();
    FILETIME time = {static_cast<DWORD>(ticks), static_cast<DWORD>(ticks >> 32)};
    bool ok = SetFileTime(file, nullptr, nullptr, &time) != 0;
    CloseHandle(file);
    return ok;
}

uint32_t CurrentUnixTime()
{
    return static_cast<uint32_t>(std::time(nullptr));
}

}
