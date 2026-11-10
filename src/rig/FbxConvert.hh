#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace tk
{

std::optional<std::filesystem::path> FindConverter();
bool ConvertFbx(const std::filesystem::path& source, const std::filesystem::path& target, std::string& error);

}
