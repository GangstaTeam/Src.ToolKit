#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace tk::fe
{

class ImageIndex
{
public:
    void Build(const std::filesystem::path& gameFolder);
    void Clear();

    const std::filesystem::path* Find(const std::string& image) const;
    std::string RelativeFile(const std::string& image) const;
    const std::vector<std::filesystem::path>& Files(const std::string& image) const;
    std::string Relative(const std::filesystem::path& file) const;
    const std::vector<std::string>& Names() const { return m_names; }

private:
    std::filesystem::path m_gameFolder;
    std::map<std::string, std::vector<std::filesystem::path>> m_files;
    std::vector<std::string> m_names;
};

}
