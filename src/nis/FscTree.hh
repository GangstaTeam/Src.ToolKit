#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace tk::fsc
{

struct Node
{
    std::string key;
    std::string value;
    std::vector<Node> children;
    bool block = false;

    static Node Block(std::string key);
    static Node Value(std::string key, std::string raw);

    Node* Find(std::string_view name);
    const Node* Find(std::string_view name) const;
    bool Has(std::string_view name) const { return Find(name) != nullptr; }

    std::string String(std::string_view name) const;
    double Number(std::string_view name, double fallback = 0.0) const;
    long long Int(std::string_view name, long long fallback = 0) const;

    Node& SetRaw(std::string_view name, std::string raw);
    void SetString(std::string_view name, std::string_view text);
    void SetNumber(std::string_view name, double number);
    void SetInt(std::string_view name, long long number);
};

bool Parse(const std::string& text, std::vector<Node>& roots, std::string& error);
std::string Write(const std::vector<Node>& roots);
std::string Write(const Node& node);

std::string Quote(std::string_view text);
std::string Unquote(std::string_view raw);
std::string FormatNumber(double value);

}
