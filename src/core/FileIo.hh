#pragma once

#include "core/Bytes.hh"

#include <filesystem>
#include <optional>
#include <string>

namespace tk
{

std::optional<Bytes> ReadFile(const std::filesystem::path& path);
bool WriteFile(const std::filesystem::path& path, const Bytes& bytes);
std::optional<std::string> ReadText(const std::filesystem::path& path);

}
