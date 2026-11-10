#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>

namespace tk
{

std::optional<uint32_t> GetFileUnixTime(const std::filesystem::path& path);
bool SetFileUnixTime(const std::filesystem::path& path, uint32_t unixTime);
uint32_t CurrentUnixTime();

}
