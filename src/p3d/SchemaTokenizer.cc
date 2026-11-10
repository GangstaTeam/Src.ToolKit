#include "p3d/SchemaTokenizer.hh"

#include <cctype>

namespace tk
{

namespace
{

bool IsWordChar(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == ':';
}

size_t SkipComment(const std::string& text, size_t i)
{
    if (text.compare(i, 2, "//") == 0)
    {
        size_t end = text.find('\n', i);
        return end == std::string::npos ? text.size() : end;
    }

    if (text.compare(i, 2, "/*") == 0)
    {
        size_t end = text.find("*/", i + 2);
        return end == std::string::npos ? text.size() : end + 2;
    }

    if (text[i] == '#')
    {
        size_t end = text.find(')', i);
        return end == std::string::npos ? text.size() : end + 1;
    }

    return i;
}

}

std::vector<std::string> TokenizeSchema(const std::string& text)
{
    std::vector<std::string> tokens;
    size_t i = 0;

    while (i < text.size())
    {
        size_t skipped = SkipComment(text, i);

        if (skipped != i)
        {
            i = skipped;
        }
        else if (std::isspace(static_cast<unsigned char>(text[i])))
        {
            ++i;
        }
        else if (IsWordChar(text[i]))
        {
            size_t start = i;

            while (i < text.size() && IsWordChar(text[i]))
            {
                ++i;
            }

            tokens.push_back(text.substr(start, i - start));
        }
        else
        {
            tokens.push_back(std::string(1, text[i++]));
        }
    }

    return tokens;
}

}
