#include "fe/FeTextBible.hh"

#include "core/ByteReader.hh"
#include "core/FileIo.hh"
#include "fe/FeModel.hh"
#include "p3d/ChunkIds.hh"
#include "p3d/ChunkIo.hh"

#include <algorithm>

namespace tk::fe
{

namespace
{

std::string PString(ByteReader& reader)
{
    uint8_t length = reader.U8();
    Bytes bytes = reader.Take(length);
    std::string text(bytes.begin(), bytes.end());
    size_t nul = text.find('\0');

    if (nul != std::string::npos)
    {
        text.resize(nul);
    }

    return text;
}

std::u16string Widen(const std::string& text)
{
    std::u16string out;

    for (unsigned char c : text)
    {
        out += static_cast<char16_t>(c);
    }

    return out;
}

std::string Narrow(const std::u16string& text)
{
    std::string out;

    for (char16_t c : text)
    {
        out += static_cast<char>(c < 0x80 ? c : '?');
    }

    return out;
}

}

uint32_t MakeKey(const std::string& text)
{
    uint32_t hash = 0;

    for (char ch : text)
    {
        int c = static_cast<signed char>(ch);

        if (c < 'a')
        {
            c += 0x20;
        }

        hash = ((hash * 65599u) & 0x7FFFFFFFu) ^ static_cast<uint32_t>(c);
    }

    return hash | 0x80000000u;
}

void TextBibles::Clear()
{
    m_languages.clear();
}

size_t TextBibles::Count() const
{
    size_t count = 0;

    for (const Language& language : m_languages)
    {
        count += language.hashes.size();
    }

    return count;
}

void TextBibles::AddFrom(const Chunk& root, char language)
{
    for (size_t i = 0; i < root.ChildCount(); ++i)
    {
        const Chunk& bible = *root.Child(i);

        if (bible.Id() != chunk::TextBible)
        {
            continue;
        }

        for (size_t l = 0; l < bible.ChildCount(); ++l)
        {
            const Chunk& chunk = *bible.Child(l);

            if (chunk.Id() != chunk::Language)
            {
                continue;
            }

            ByteReader r(chunk.Payload());
            PString(r);
            char id = static_cast<char>(r.U8());
            uint32_t count = r.U32();
            r.U32();
            uint32_t size = r.U32();

            if (id != language || r.Failed())
            {
                continue;
            }

            Language entry;

            for (uint32_t s = 0; s < count; ++s)
            {
                entry.hashes.push_back(r.U32());
            }

            for (uint32_t s = 0; s < count; ++s)
            {
                entry.offsets.push_back(r.U32());
            }

            entry.buffer = r.Take(size);

            if (!r.Failed())
            {
                m_languages.push_back(std::move(entry));
            }
        }
    }
}

size_t TextBibles::LoadFolder(const std::filesystem::path& folder, char language)
{
    std::error_code ec;
    size_t loaded = 0;
    std::string suffix = std::string("_") + language + ".p3d";

    for (const auto& item : std::filesystem::directory_iterator(folder, ec))
    {
        std::string name = item.path().filename().string();
        std::string lower = name;
        std::transform(lower.begin(),
            lower.end(),
            lower.begin(),
            [](unsigned char c)
            {
                return char(std::tolower(c));
            });
        std::string lowerSuffix = suffix;
        std::transform(lowerSuffix.begin(),
            lowerSuffix.end(),
            lowerSuffix.begin(),
            [](unsigned char c)
            {
                return char(std::tolower(c));
            });
        if (lower.find("textbible") == std::string::npos || lower.size() < lowerSuffix.size() ||
            lower.compare(lower.size() - lowerSuffix.size(), lowerSuffix.size(), lowerSuffix) != 0)
        {
            continue;
        }

        auto bytes = ReadFile(item.path());

        if (!bytes)
        {
            continue;
        }

        ParseResult parsed = ParseP3d(*bytes);

        if (!parsed.root)
        {
            continue;
        }

        AddFrom(*parsed.root, language);
        ++loaded;
    }

    return loaded;
}

bool TextBibles::Find(uint32_t key, std::u16string& text) const
{
    for (const Language& language : m_languages)
    {
        auto it = std::lower_bound(language.hashes.begin(), language.hashes.end(), key);

        if (it == language.hashes.end() || *it != key)
        {
            continue;
        }

        size_t offset = language.offsets[static_cast<size_t>(it - language.hashes.begin())];
        text.clear();

        for (size_t i = offset; i + 1 < language.buffer.size(); i += 2)
        {
            char16_t c = static_cast<char16_t>(language.buffer[i] | (language.buffer[i + 1] << 8));

            if (!c)
            {
                break;
            }

            text += c;
        }

        return true;
    }

    return false;
}

void TextBibles::ExpandInto(const std::u16string& text, std::u16string& out, int depth) const
{
    size_t i = 0;

    while (i < text.size())
    {
        size_t open = text.find(u'[', i);
        size_t close = open == std::u16string::npos ? std::u16string::npos : text.find(u']', open);

        if (close == std::u16string::npos)
        {
            out.append(text, i, std::u16string::npos);
            return;
        }

        out.append(text, i, open - i);
        std::u16string key = text.substr(open + 1, close - open - 1);
        std::u16string value;

        if (depth < 8 && Find(MakeKey(Narrow(key)), value))
        {
            ExpandInto(value, out, depth + 1);
        }
        else
        {
            out.append(text, open, close - open + 1);
        }

        i = close + 1;
    }
}

std::u16string TextBibles::Expand(const std::string& text) const
{
    std::u16string out;
    ExpandInto(Widen(text), out, 0);
    return out;
}

}
