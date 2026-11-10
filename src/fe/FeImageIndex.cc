#include "fe/FeImageIndex.hh"

#include "core/FileIo.hh"
#include "p3d/ChunkIds.hh"
#include "p3d/ChunkIo.hh"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace tk::fe
{

namespace
{

std::string Lower(std::string text)
{
    for (char& c : text)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return text;
}

uint32_t U32(const Bytes& bytes, size_t offset)
{
    uint32_t value = 0;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

std::vector<std::string> TopLevelImages(const Bytes& bytes)
{
    std::vector<std::string> names;

    if (bytes.size() < Chunk::kHeaderSize || U32(bytes, 0) != kP3dFileId)
    {
        return names;
    }

    size_t offset = U32(bytes, 4);

    while (offset + Chunk::kHeaderSize <= bytes.size())
    {
        uint32_t id = U32(bytes, offset);
        uint32_t header = U32(bytes, offset + 4);
        uint32_t total = U32(bytes, offset + 8);

        if (total < Chunk::kHeaderSize || header < Chunk::kHeaderSize || offset + total > bytes.size())
        {
            break;
        }

        if ((id == chunk::Sprite || id == chunk::Texture) && header > Chunk::kHeaderSize)
        {
            size_t length = bytes[offset + Chunk::kHeaderSize];
            size_t start = offset + Chunk::kHeaderSize + 1;

            if (start + length <= offset + header)
            {
                std::string name(reinterpret_cast<const char*>(bytes.data() + start), length);
                size_t nul = name.find('\0');

                if (nul != std::string::npos)
                {
                    name.resize(nul);
                }

                if (!name.empty())
                {
                    names.push_back(name);
                }
            }
        }

        offset += total;
    }

    return names;
}

}

void ImageIndex::Clear()
{
    m_gameFolder.clear();
    m_files.clear();
    m_names.clear();
}

void ImageIndex::Build(const std::filesystem::path& gameFolder)
{
    Clear();
    m_gameFolder = gameFolder;
    std::error_code ec;
    std::filesystem::path frontend = gameFolder / "frontend";

    if (gameFolder.empty() || !std::filesystem::is_directory(frontend, ec))
    {
        return;
    }

    std::vector<std::filesystem::path> files;

    for (auto it = std::filesystem::recursive_directory_iterator(frontend, ec);
        !ec && it != std::filesystem::recursive_directory_iterator();
        it.increment(ec))
    {
        if (it->is_regular_file(ec) && Lower(it->path().extension().u8string()) == ".p3d")
        {
            files.push_back(it->path());
        }
    }

    std::sort(files.begin(), files.end());

    for (const auto& file : files)
    {
        auto bytes = ReadFile(file);

        if (!bytes)
        {
            continue;
        }

        for (const std::string& name : TopLevelImages(*bytes))
        {
            std::vector<std::filesystem::path>& holders = m_files[Lower(name)];

            if (holders.empty())
            {
                m_names.push_back(name);
            }

            holders.push_back(file);
        }
    }

    std::sort(m_names.begin(),
        m_names.end(),
        [](const std::string& a, const std::string& b)
        {
            return Lower(a) < Lower(b);
        });
}

const std::filesystem::path* ImageIndex::Find(const std::string& image) const
{
    auto it = m_files.find(Lower(image));
    return it == m_files.end() || it->second.empty() ? nullptr : &it->second.front();
}

const std::vector<std::filesystem::path>& ImageIndex::Files(const std::string& image) const
{
    static const std::vector<std::filesystem::path> kNone;
    auto it = m_files.find(Lower(image));
    return it == m_files.end() ? kNone : it->second;
}

std::string ImageIndex::Relative(const std::filesystem::path& file) const
{
    return file.lexically_relative(m_gameFolder).generic_u8string();
}

std::string ImageIndex::RelativeFile(const std::string& image) const
{
    const std::filesystem::path* file = Find(image);
    return file ? Relative(*file) : std::string();
}

}
