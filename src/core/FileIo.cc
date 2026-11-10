#include "core/FileIo.hh"

#include <fstream>
#include <iterator>

namespace tk
{

std::optional<Bytes> ReadFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);

    if (!file)
    {
        return std::nullopt;
    }

    return Bytes(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

bool WriteFile(const std::filesystem::path& path, const Bytes& bytes)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);

    if (!file)
    {
        return false;
    }

    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(file);
}

std::optional<std::string> ReadText(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);

    if (!file)
    {
        return std::nullopt;
    }

    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

}
