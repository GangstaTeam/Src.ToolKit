#include "rig/Gltf.hh"

#include "core/ByteReader.hh"
#include "core/FileIo.hh"

#include <cstring>

namespace tk
{

namespace
{

Bytes DecodeBase64(const std::string& text)
{
    static const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    Bytes out;
    uint32_t bits = 0;
    int count = 0;

    for (char c : text)
    {
        size_t value = alphabet.find(c);

        if (value == std::string::npos)
        {
            continue;
        }

        bits = (bits << 6) | static_cast<uint32_t>(value);
        count += 6;

        if (count >= 8)
        {
            count -= 8;
            out.push_back(static_cast<uint8_t>(bits >> count));
        }
    }

    return out;
}

std::optional<Bytes> LoadUri(const std::filesystem::path& directory, const std::string& uri)
{
    if (uri.rfind("data:", 0) == 0)
    {
        size_t comma = uri.find(',');

        if (comma == std::string::npos)
        {
            return std::nullopt;
        }

        return DecodeBase64(uri.substr(comma + 1));
    }

    return ReadFile(directory / std::filesystem::u8path(uri));
}

bool SplitGlb(const Bytes& bytes, std::string& jsonText, Bytes& bin)
{
    ByteReader reader(bytes);

    if (reader.U32() != kGlbMagic)
    {
        return false;
    }

    reader.U32();
    reader.U32();

    while (reader.Remaining() >= 8)
    {
        uint32_t length = reader.U32(), type = reader.U32();
        Bytes data = reader.Take(length);

        if (type == kJsonChunk)
        {
            jsonText.assign(data.begin(), data.end());
        }

        if (type == kBinChunk)
        {
            bin = std::move(data);
        }
    }

    return !reader.Failed() && !jsonText.empty();
}

size_t ComponentCount(const std::string& type)
{
    if (type == "SCALAR")
    {
        return 1;
    }

    if (type == "VEC2")
    {
        return 2;
    }

    if (type == "VEC3")
    {
        return 3;
    }

    if (type == "VEC4" || type == "MAT2")
    {
        return 4;
    }

    if (type == "MAT3")
    {
        return 9;
    }

    if (type == "MAT4")
    {
        return 16;
    }

    return 0;
}

size_t ComponentSize(int componentType)
{
    if (componentType == 5120 || componentType == 5121)
    {
        return 1;
    }

    if (componentType == 5122 || componentType == 5123)
    {
        return 2;
    }

    return 4;
}

float ReadComponent(const uint8_t* p, int componentType, bool normalized)
{
    switch (componentType)
    {
    case 5120:
        return normalized ? static_cast<int8_t>(*p) / 127.0f : static_cast<int8_t>(*p);
    case 5121:
        return normalized ? *p / 255.0f : *p;
    case 5122:
    {
        int16_t v;
        std::memcpy(&v, p, 2);
        return normalized ? v / 32767.0f : v;
    }
    case 5123:
    {
        uint16_t v;
        std::memcpy(&v, p, 2);
        return normalized ? v / 65535.0f : v;
    }
    case 5125:
    {
        uint32_t v;
        std::memcpy(&v, p, 4);
        return static_cast<float>(v);
    }
    default:
    {
        float v;
        std::memcpy(&v, p, 4);
        return v;
    }
    }
}

}

std::optional<GltfFile> LoadGltf(const std::filesystem::path& path, std::string& error)
{
    auto bytes = ReadFile(path);

    if (!bytes)
    {
        error = "cannot read " + path.u8string();
        return std::nullopt;
    }

    std::string jsonText;
    Bytes bin;

    if (!SplitGlb(*bytes, jsonText, bin))
    {
        jsonText.assign(bytes->begin(), bytes->end());
    }

    auto json = Json::Parse(jsonText);

    if (!json)
    {
        error = path.filename().u8string() + ": not a valid glTF file";
        return std::nullopt;
    }

    GltfFile file{std::move(*json), {}, path.parent_path()};
    const Json& buffers = file.json["buffers"];

    for (size_t i = 0; i < buffers.Size(); ++i)
    {
        if (!buffers[i].Has("uri"))
        {
            file.buffers.push_back(bin);
            continue;
        }

        auto data = LoadUri(file.directory, buffers[i]["uri"].String());

        if (!data)
        {
            error = "missing buffer " + buffers[i]["uri"].String();
            return std::nullopt;
        }

        file.buffers.push_back(std::move(*data));
    }

    return file;
}

Accessor ReadAccessor(const GltfFile& file, int index)
{
    const Json& accessor = file.json["accessors"][static_cast<size_t>(index)];
    Accessor out;
    out.count = static_cast<size_t>(accessor["count"].Int(0));
    out.components = ComponentCount(accessor["type"].String());
    out.values.assign(out.count * out.components, 0.0f);

    if (!accessor.Has("bufferView"))
    {
        return out;
    }

    const Json& view = file.json["bufferViews"][static_cast<size_t>(accessor["bufferView"].Int())];
    size_t bufferIndex = static_cast<size_t>(view["buffer"].Int(0));

    if (bufferIndex >= file.buffers.size())
    {
        return out;
    }

    const Bytes& buffer = file.buffers[bufferIndex];
    int componentType = accessor["componentType"].Int(5126);
    bool normalized = accessor["normalized"].Bool();
    size_t componentSize = ComponentSize(componentType);
    size_t stride = static_cast<size_t>(view["byteStride"].Int(0));

    if (stride == 0)
    {
        stride = componentSize * out.components;
    }

    size_t base = static_cast<size_t>(view["byteOffset"].Int(0) + accessor["byteOffset"].Int(0));

    for (size_t i = 0; i < out.count; ++i)
    {
        for (size_t c = 0; c < out.components; ++c)
        {
            size_t offset = base + i * stride + c * componentSize;

            if (offset + componentSize > buffer.size())
            {
                return out;
            }

            out.values[i * out.components + c] = ReadComponent(buffer.data() + offset, componentType, normalized);
        }
    }

    return out;
}

Bytes ReadImage(const GltfFile& file, int index)
{
    const Json& image = file.json["images"][static_cast<size_t>(index)];

    if (image.Has("uri"))
    {
        return LoadUri(file.directory, image["uri"].String()).value_or(Bytes());
    }

    if (!image.Has("bufferView"))
    {
        return {};
    }

    const Json& view = file.json["bufferViews"][static_cast<size_t>(image["bufferView"].Int())];
    size_t bufferIndex = static_cast<size_t>(view["buffer"].Int(0));

    if (bufferIndex >= file.buffers.size())
    {
        return {};
    }

    const Bytes& buffer = file.buffers[bufferIndex];
    size_t offset = static_cast<size_t>(view["byteOffset"].Int(0)),
           length = static_cast<size_t>(view["byteLength"].Int(0));
    if (offset + length > buffer.size())
    {
        return {};
    }

    return Bytes(buffer.begin() + static_cast<std::ptrdiff_t>(offset),
        buffer.begin() + static_cast<std::ptrdiff_t>(offset + length));
}

}
