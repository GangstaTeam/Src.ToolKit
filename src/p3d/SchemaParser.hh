#pragma once

#include "p3d/Schema.hh"

#include <filesystem>
#include <string>

namespace tk
{

bool ParseSchemaText(const std::string& text, Schema& schema, std::string& error);
bool LoadSchemaDirectory(const std::filesystem::path& directory, Schema& schema, std::string& error);

}
