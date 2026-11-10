#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tk
{

class Json
{
public:
    enum class Type
    {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    };

    static std::optional<Json> Parse(std::string_view text);

    Type GetType() const { return m_type; }
    bool IsNull() const { return m_type == Type::Null; }
    bool Has(const char* key) const;
    size_t Size() const { return m_type == Type::Object ? m_members.size() : m_items.size(); }

    const Json& operator[](const char* key) const;
    const Json& operator[](size_t index) const;
    const Json& operator[](int index) const { return (*this)[static_cast<size_t>(index)]; }

    double Number(double fallback = 0.0) const { return m_type == Type::Number ? m_number : fallback; }
    int Int(int fallback = -1) const { return m_type == Type::Number ? static_cast<int>(m_number) : fallback; }
    bool Bool(bool fallback = false) const { return m_type == Type::Bool ? m_bool : fallback; }
    const std::string& String() const { return m_string; }

private:
    friend class JsonParser;

    Type m_type = Type::Null;
    double m_number = 0.0;
    bool m_bool = false;
    std::string m_string;
    std::vector<Json> m_items;
    std::vector<std::pair<std::string, Json>> m_members;
};

}
