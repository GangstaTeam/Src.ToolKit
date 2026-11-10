#include "rig/Json.hh"

#include <cstdlib>
#include <cstring>

namespace tk
{

class JsonParser
{
public:
    explicit JsonParser(std::string_view text) : m_text(text) {}

    bool ParseDocument(Json& out)
    {
        if (!ParseValue(out, 0))
        {
            return false;
        }

        SkipSpace();
        return m_position == m_text.size();
    }

private:
    void SkipSpace()
    {
        while (m_position < m_text.size() && std::strchr(" \t\r\n", m_text[m_position]) && m_text[m_position])
        {
            ++m_position;
        }
    }

    bool Consume(char c)
    {
        SkipSpace();

        if (m_position >= m_text.size() || m_text[m_position] != c)
        {
            return false;
        }

        ++m_position;
        return true;
    }

    bool Literal(std::string_view word)
    {
        if (m_text.substr(m_position, word.size()) != word)
        {
            return false;
        }

        m_position += word.size();
        return true;
    }

    bool ParseValue(Json& out, int depth)
    {
        SkipSpace();

        if (m_position >= m_text.size() || depth > 256)
        {
            return false;
        }

        char c = m_text[m_position];

        if (c == '{')
        {
            return ParseObject(out, depth);
        }

        if (c == '[')
        {
            return ParseArray(out, depth);
        }

        if (c == '"')
        {
            out.m_type = Json::Type::String;
            return ParseString(out.m_string);
        }

        if (Literal("true") || Literal("false"))
        {
            out.m_type = Json::Type::Bool;
            out.m_bool = c == 't';
            return true;
        }

        if (Literal("null"))
        {
            return true;
        }

        return ParseNumber(out);
    }

    bool ParseNumber(Json& out)
    {
        std::string token;

        while (m_position < m_text.size() && std::strchr("+-0123456789.eE", m_text[m_position]) && m_text[m_position])
        {
            token += m_text[m_position++];
        }

        if (token.empty())
        {
            return false;
        }

        char* end = nullptr;
        out.m_number = std::strtod(token.c_str(), &end);
        out.m_type = Json::Type::Number;
        return end && *end == '\0';
    }

    static void AppendUtf8(std::string& out, unsigned code)
    {
        if (code < 0x80)
        {
            out += static_cast<char>(code);
        }
        else if (code < 0x800)
        {
            out += static_cast<char>(0xC0 | (code >> 6));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
        else
        {
            out += static_cast<char>(0xE0 | (code >> 12));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
    }

    bool ParseEscape(std::string& out)
    {
        if (m_position >= m_text.size())
        {
            return false;
        }

        char e = m_text[m_position++];
        const char* simple = std::strchr("\"\\/bfnrt", e);

        if (simple && e)
        {
            out += "\"\\/\b\f\n\r\t"[simple - "\"\\/bfnrt"];
            return true;
        }

        if (e != 'u' || m_position + 4 > m_text.size())
        {
            return false;
        }

        AppendUtf8(out,
            static_cast<unsigned>(std::strtoul(std::string(m_text.substr(m_position, 4)).c_str(), nullptr, 16)));
        m_position += 4;
        return true;
    }

    bool ParseString(std::string& out)
    {
        ++m_position;

        while (m_position < m_text.size())
        {
            char c = m_text[m_position++];

            if (c == '"')
            {
                return true;
            }

            if (c != '\\')
            {
                out += c;
            }
            else if (!ParseEscape(out))
            {
                return false;
            }
        }

        return false;
    }

    bool ParseArray(Json& out, int depth)
    {
        ++m_position;
        out.m_type = Json::Type::Array;

        if (Consume(']'))
        {
            return true;
        }

        do
        {
            out.m_items.emplace_back();

            if (!ParseValue(out.m_items.back(), depth + 1))
            {
                return false;
            }
        } while (Consume(','));
        return Consume(']');
    }

    bool ParseObject(Json& out, int depth)
    {
        ++m_position;
        out.m_type = Json::Type::Object;

        if (Consume('}'))
        {
            return true;
        }

        do
        {
            SkipSpace();
            std::string key;

            if (m_position >= m_text.size() || m_text[m_position] != '"' || !ParseString(key) || !Consume(':'))
            {
                return false;
            }

            out.m_members.emplace_back(std::move(key), Json());

            if (!ParseValue(out.m_members.back().second, depth + 1))
            {
                return false;
            }
        } while (Consume(','));
        return Consume('}');
    }

    std::string_view m_text;
    size_t m_position = 0;
};

namespace
{

const Json& NullJson()
{
    static const Json null;
    return null;
}

}

std::optional<Json> Json::Parse(std::string_view text)
{
    Json root;

    if (!JsonParser(text).ParseDocument(root))
    {
        return std::nullopt;
    }

    return root;
}

bool Json::Has(const char* key) const
{
    return !(*this)[key].IsNull();
}

const Json& Json::operator[](const char* key) const
{
    for (const auto& [name, value] : m_members)
    {
        if (name == key)
        {
            return value;
        }
    }

    return NullJson();
}

const Json& Json::operator[](size_t index) const
{
    return index < m_items.size() ? m_items[index] : NullJson();
}

}
