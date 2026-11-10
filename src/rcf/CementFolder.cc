#include "rcf/CementFolder.hh"

#include "rcf/FileTime.hh"

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>

namespace tk
{

namespace
{

struct ManifestEntry
{
    std::string name;
    uint32_t alignment = kCementAlignment;
    uint32_t reserved = 0;
};

bool ReadManifest(const std::filesystem::path& path,
    std::vector<ManifestEntry>& entries,
    uint32_t& alignment,
    std::string& error)
{
    std::ifstream in(path, std::ios::binary);

    if (!in)
    {
        return true;
    }

    std::string line;
    size_t number = 0;

    while (std::getline(in, line))
    {
        ++number;

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream fields(line);
        std::string first;
        fields >> first;

        if (first == CEMENTFOLDER_MANIFEST_HEADER)
        {
            std::string version, key;
            fields >> version >> key >> alignment;
            continue;
        }

        ManifestEntry entry;
        uint32_t timestamp = 0;
        try
        {
            entry.alignment = static_cast<uint32_t>(std::stoul(first));
        }

        catch (...)
        {
            error = path.filename().u8string() + " line " + std::to_string(number) +
                ": expected \"<alignment> <reserved> <time> <name>\"";
            return false;
        }

        fields >> entry.reserved >> timestamp;
        fields >> std::ws;
        std::getline(fields, entry.name);
        entry.name = CementNormalizeName(entry.name);

        if (!entry.name.empty())
        {
            entries.push_back(std::move(entry));
        }
    }

    return true;
}

}

std::filesystem::path CementEntryPath(const std::filesystem::path& root, const std::string& name)
{
    std::filesystem::path path = root;
    size_t start = 0;

    while (start <= name.size())
    {
        size_t end = name.find('\\', start);

        if (end == std::string::npos)
        {
            end = name.size();
        }

        if (end > start)
        {
            path /= std::filesystem::path(name.substr(start, end - start));
        }

        start = end + 1;
    }

    return path;
}

bool ExtractCementSources(const std::vector<CementSource>& sources,
    const std::filesystem::path& root,
    const std::string& manifestFor,
    uint32_t alignment,
    const CementProgress& progress,
    std::string& error)
{
    uint64_t total = 0;

    for (const CementSource& source : sources)
    {
        total += source.size;
    }

    uint64_t done = 0;

    for (const CementSource& source : sources)
    {
        std::filesystem::path target = CementEntryPath(root, source.name);
        std::error_code ec;
        std::filesystem::create_directories(target.parent_path(), ec);
        {
            CementFileSink out(target);

            if (!out.IsOpen())
            {
                error = "Cannot create " + target.u8string();
                return false;
            }

            if (!CopyCementSource(source, out, progress, done, total, error))
            {
                return false;
            }

            if (!out.Close())
            {
                error = "Cannot write " + target.u8string();
                return false;
            }
        }

        SetFileUnixTime(target, source.timestamp);
    }

    if (manifestFor.empty())
    {
        return true;
    }

    std::ofstream manifest(root / CEMENTFOLDER_CEMENT_MANIFEST_NAME, std::ios::binary | std::ios::trunc);
    manifest << "# ToolKit cement manifest: file order and fields of " << manifestFor << "\n"
             << "# <alignment> <reserved> <original time> <name>; packing uses each file's write time.\n"
             << CEMENTFOLDER_MANIFEST_HEADER << " " << int(kCementVersionMajor) << "." << int(kCementVersionMinor)
             << " alignment " << alignment << "\n";
    for (const CementSource& source : sources)
    {
        manifest << source.alignment << " " << source.reserved << " " << source.timestamp << " " << source.name << "\n";
    }

    if (!manifest)
    {
        error = "Cannot write " + (root / CEMENTFOLDER_CEMENT_MANIFEST_NAME).u8string();
        return false;
    }

    return true;
}

bool ExtractCement(CementArchive& archive,
    const std::vector<const CementEntry*>& entries,
    const std::filesystem::path& root,
    bool writeManifest,
    const CementProgress& progress,
    std::string& error)
{
    std::vector<CementSource> sources;

    for (const CementEntry* entry : entries)
    {
        sources.push_back(CementSource::FromArchive(archive.Path(), *entry));
    }

    return ExtractCementSources(sources,
        root,
        writeManifest ? archive.Path().filename().u8string() : std::string(),
        archive.Alignment(),
        progress,
        error);
}

bool CollectCementFolder(const std::filesystem::path& root,
    std::vector<CementSource>& sources,
    uint32_t& alignment,
    std::string& error)
{
    std::error_code ec;

    if (!std::filesystem::is_directory(root, ec))
    {
        error = root.u8string() + " is not a folder";
        return false;
    }

    alignment = kCementAlignment;
    std::vector<ManifestEntry> manifest;

    if (!ReadManifest(root / CEMENTFOLDER_CEMENT_MANIFEST_NAME, manifest, alignment, error))
    {
        return false;
    }

    std::map<uint32_t, std::pair<std::string, std::filesystem::path>> files;

    for (auto it = std::filesystem::recursive_directory_iterator(root, ec);
        !ec && it != std::filesystem::recursive_directory_iterator();
        it.increment(ec))
    {
        if (!it->is_regular_file())
        {
            continue;
        }

        std::filesystem::path relative = std::filesystem::relative(it->path(), root, ec);

        if (relative == CEMENTFOLDER_CEMENT_MANIFEST_NAME)
        {
            continue;
        }

        std::string name = CementNormalizeName(relative.string());
        uint32_t hash = CementHash(name);
        auto [slot, inserted] = files.emplace(hash, std::make_pair(name, it->path()));

        if (!inserted)
        {
            error = "\"" + name + "\" and \"" + slot->second.first + "\" have the same cement hash";
            return false;
        }
    }

    if (ec)
    {
        error = "Cannot list " + root.u8string() + ": " + ec.message();
        return false;
    }

    sources.clear();

    for (const ManifestEntry& entry : manifest)
    {
        auto it = files.find(CementHash(entry.name));

        if (it == files.end())
        {
            continue;
        }

        CementSource source = CementSource::FromFile(entry.name, it->second.second);
        source.alignment = entry.alignment;
        source.reserved = entry.reserved;
        sources.push_back(std::move(source));
        files.erase(it);
    }

    std::vector<std::pair<std::string, std::filesystem::path>> added;

    for (auto& [hash, file] : files)
    {
        added.push_back(std::move(file));
    }

    std::sort(added.begin(),
        added.end(),
        [](const auto& a, const auto& b)
        {
            return a.first < b.first;
        });
    for (auto& [name, path] : added)
    {
        CementSource source = CementSource::FromFile(name, path);
        source.alignment = alignment;
        sources.push_back(std::move(source));
    }

    return true;
}

}
