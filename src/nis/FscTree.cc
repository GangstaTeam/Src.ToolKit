#include "nis/FscTree.hh"

#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace tk::fsc
{

namespace
{

struct Token
{
    std::string text;
    size_t line = 0;
};

bool Tokenize(const std::string& text, std::vector<Token>& tokens, std::string& error)
{
    size_t line = 1;
    size_t i = 0;

    while (i < text.size())
    {
        char c = text[i];

        if (c == '\n')
        {
            ++line;
        }

        if (std::isspace(static_cast<unsigned char>(c)))
        {
            ++i;
            continue;
        }

        if (c == '/' && i + 1 < text.size() && text[i + 1] == '/')
        {
            while (i < text.size() && text[i] != '\n')
            {
                ++i;
            }

            continue;
        }

        if (c == '{' || c == '}' || c == ';')
        {
            tokens.push_back({std::string(1, c), line});
            ++i;
            continue;
        }

        size_t start = i;

        if (c == '"')
        {
            size_t close = text.find('"', i + 1);

            if (close == std::string::npos)
            {
                error = "line " + std::to_string(line) + ": unterminated string";
                return false;
            }

            for (size_t k = i; k < close; ++k)
            {
                if (text[k] == '\n')
                {
                    ++line;
                }
            }

            i = close + 1;
        }
        else
        {
            while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i])) && text[i] != '{' &&
                text[i] != '}' && text[i] != ';' && text[i] != '"')
            {
                ++i;
            }
        }

        tokens.push_back({text.substr(start, i - start), line});
    }

    return true;
}

class Parser
{
public:
    explicit Parser(const std::vector<Token>& tokens) : m_tokens(tokens) {}

    bool Items(std::vector<Node>& items, bool nested, std::string& error)
    {
        while (m_pos < m_tokens.size())
        {
            const Token& key = m_tokens[m_pos];

            if (key.text == "}")
            {
                if (nested)
                {
                    return true;
                }

                return Fail(key, "unexpected '}'", error);
            }

            if (key.text == "{" || key.text == ";")
            {
                return Fail(key, "expected a name before '" + key.text + "'", error);
            }

            ++m_pos;

            if (m_pos < m_tokens.size() && m_tokens[m_pos].text == "{")
            {
                ++m_pos;
                Node block = Node::Block(key.text);

                if (!Items(block.children, true, error))
                {
                    return false;
                }

                if (m_pos >= m_tokens.size())
                {
                    return Fail(key, "block '" + key.text + "' is never closed", error);
                }

                ++m_pos;

                if (m_pos < m_tokens.size() && m_tokens[m_pos].text == ";")
                {
                    ++m_pos;
                }

                items.push_back(std::move(block));
                continue;
            }

            std::string raw;

            while (m_pos < m_tokens.size() && m_tokens[m_pos].text != ";")
            {
                const std::string& part = m_tokens[m_pos].text;

                if (part == "{" || part == "}")
                {
                    return Fail(m_tokens[m_pos], "missing ';' after '" + key.text + "'", error);
                }

                if (!raw.empty())
                {
                    raw += ' ';
                }

                raw += part;
                ++m_pos;
            }

            if (m_pos >= m_tokens.size())
            {
                return Fail(key, "missing ';' after '" + key.text + "'", error);
            }

            ++m_pos;
            items.push_back(Node::Value(key.text, std::move(raw)));
        }

        if (nested)
        {
            error = "unexpected end of file inside a block";
            return false;
        }

        return true;
    }

private:
    static bool Fail(const Token& at, const std::string& message, std::string& error)
    {
        error = "line " + std::to_string(at.line) + ": " + message;
        return false;
    }

    const std::vector<Token>& m_tokens;
    size_t m_pos = 0;
};

void WriteNode(const Node& node, size_t depth, std::string& out)
{
    std::string indent(depth, '\t');

    if (!node.block)
    {
        out += indent + node.key + ' ' + node.value + ";\r\n";
        return;
    }

    out += "\r\n" + indent + node.key + "\r\n" + indent + "{\r\n";

    for (const Node& child : node.children)
    {
        WriteNode(child, depth + 1, out);
    }

    out += indent + "};\r\n";
}

}

Node Node::Block(std::string key)
{
    Node node;
    node.key = std::move(key);
    node.block = true;
    return node;
}

Node Node::Value(std::string key, std::string raw)
{
    Node node;
    node.key = std::move(key);
    node.value = std::move(raw);
    return node;
}

Node* Node::Find(std::string_view name)
{
    for (Node& child : children)
    {
        if (child.key == name)
        {
            return &child;
        }
    }

    return nullptr;
}

const Node* Node::Find(std::string_view name) const
{
    for (const Node& child : children)
    {
        if (child.key == name)
        {
            return &child;
        }
    }

    return nullptr;
}

std::string Node::String(std::string_view name) const
{
    const Node* child = Find(name);
    return child && !child->block ? Unquote(child->value) : std::string();
}

double Node::Number(std::string_view name, double fallback) const
{
    const Node* child = Find(name);

    if (!child || child->block || child->value.empty())
    {
        return fallback;
    }

    char* end = nullptr;
    double parsed = std::strtod(child->value.c_str(), &end);
    return end && *end == '\0' ? parsed : fallback;
}

long long Node::Int(std::string_view name, long long fallback) const
{
    double number = Number(name, static_cast<double>(fallback));
    return static_cast<long long>(number < 0 ? number - 0.5 : number + 0.5);
}

Node& Node::SetRaw(std::string_view name, std::string raw)
{
    if (Node* child = Find(name); child && !child->block)
    {
        child->value = std::move(raw);
        return *child;
    }

    children.push_back(Value(std::string(name), std::move(raw)));
    return children.back();
}

void Node::SetString(std::string_view name, std::string_view text)
{
    SetRaw(name, Quote(text));
}

void Node::SetNumber(std::string_view name, double number)
{
    SetRaw(name, FormatNumber(number));
}

void Node::SetInt(std::string_view name, long long number)
{
    SetRaw(name, std::to_string(number));
}

bool Parse(const std::string& text, std::vector<Node>& roots, std::string& error)
{
    std::vector<Token> tokens;

    if (!Tokenize(text, tokens, error))
    {
        return false;
    }

    std::vector<Node> parsed;
    Parser parser(tokens);

    if (!parser.Items(parsed, false, error))
    {
        return false;
    }

    roots = std::move(parsed);
    return true;
}

std::string Write(const std::vector<Node>& roots)
{
    std::string out;

    for (const Node& root : roots)
    {
        WriteNode(root, 0, out);
    }

    return out;
}

std::string Write(const Node& node)
{
    std::string out;
    WriteNode(node, 0, out);
    return out;
}

std::string Quote(std::string_view text)
{
    std::string out = "\"";

    for (char c : text)
    {
        out += c == '"' ? '^' : c;
    }

    return out + '"';
}

std::string Unquote(std::string_view raw)
{
    if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"')
    {
        return std::string(raw.substr(1, raw.size() - 2));
    }

    return std::string(raw);
}

std::string FormatNumber(double value)
{
    if (value == 0.0)
    {
        return "0";
    }

    char text[32];
    std::snprintf(text, sizeof(text), "%g", value);
    return text;
}

}
