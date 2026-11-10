#include "rcf/CementFormat.hh"

#include <vector>

namespace tk
{

uint32_t CementHash(std::string_view name)
{
    size_t i = !name.empty() && name[0] == '\\' ? 1 : 0;
    uint32_t hash = 0;

    for (; i < name.size() && name[i] != '\0'; ++i)
    {
        int c = static_cast<signed char>(name[i]);

        if (c < 'a')
        {
            c += 0x20;
        }

        hash = static_cast<uint32_t>(c) + 31 * hash;
    }

    return hash;
}

std::string CementNormalizeName(std::string_view name)
{
    std::vector<std::string> parts;
    std::string part;
    auto flush = [&]
    {
        if (part == "..")
        {
            if (!parts.empty())
            {
                parts.pop_back();
            }
        }
        else if (!part.empty() && part != ".")
        {
            parts.push_back(part);
        }

        part.clear();
    };

    for (char c : name)
    {
        if (c == '/' || c == '\\')
        {
            flush();
        }
        else
        {
            part += c;
        }
    }

    flush();
    std::string result;

    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i)
        {
            result += '\\';
        }

        result += parts[i];
    }

    return result;
}

uint64_t CementAlign(uint64_t value, uint32_t alignment)
{
    if (alignment <= 1)
    {
        return value;
    }

    return (value + alignment - 1) / alignment * alignment;
}

}
