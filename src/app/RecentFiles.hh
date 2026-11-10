#pragma once

#include <filesystem>
#include <vector>

#define RECENTFILES_PREFIX "File="

namespace tk
{

class RecentFiles
{
public:
    void RegisterSettingsHandler();
    void Add(const std::filesystem::path& path);
    void Append(const std::filesystem::path& path);
    void Remove(const std::filesystem::path& path);
    void Clear();
    const std::vector<std::filesystem::path>& Items() const { return m_items; }

private:
    enum ELimit : size_t
    {
        kLimit = 10,
    };

    std::vector<std::filesystem::path> m_items;
};

}
