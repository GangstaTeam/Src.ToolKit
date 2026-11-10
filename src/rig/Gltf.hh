#pragma once

#include <cstddef>
#include <cstdint>

#include "core/Bytes.hh"
#include "rig/Json.hh"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tk
{

enum EGltfValue : uint32_t
{
    kGlbMagic = 0x46546C67,
    kJsonChunk = 0x4E4F534A,
    kBinChunk = 0x004E4942,
};

struct GltfFile
{
    Json json;
    std::vector<Bytes> buffers;
    std::filesystem::path directory;
};

struct Accessor
{
    std::vector<float> values;
    size_t count = 0;
    size_t components = 0;
};

std::optional<GltfFile> LoadGltf(const std::filesystem::path& path, std::string& error);
Accessor ReadAccessor(const GltfFile& file, int index);
Bytes ReadImage(const GltfFile& file, int image);

}
