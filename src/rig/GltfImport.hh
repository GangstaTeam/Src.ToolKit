#pragma once

#include <cstddef>
#include <cstdint>

#include "rig/SourceModel.hh"

#include <filesystem>
#include <optional>
#include <string>

namespace tk
{

enum EGltfImport : int
{
    kTriangles = 4,
};

std::optional<SourceModel> ImportGltf(const std::filesystem::path& path, std::string& error);

}
