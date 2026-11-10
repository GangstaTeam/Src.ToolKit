#pragma once

#include "p3d/Chunk.hh"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace tk::fe
{

uint32_t MakeKey(const std::string& text);

class TextBibles
{
public:
    void Clear();
    void AddFrom(const Chunk& root, char language);
    size_t LoadFolder(const std::filesystem::path& folder, char language);

    bool Find(uint32_t key, std::u16string& text) const;
    std::u16string Expand(const std::string& text) const;
    size_t Count() const;

private:
    struct Language
    {
        std::vector<uint32_t> hashes;
        std::vector<uint32_t> offsets;
        std::vector<uint8_t> buffer;
    };
    void ExpandInto(const std::u16string& text, std::u16string& out, int depth) const;

    std::vector<Language> m_languages;
};

}
