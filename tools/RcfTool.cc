#include "rcf/CementArchive.hh"
#include "rcf/CementFolder.hh"
#include "rcf/CementWriter.hh"

#include <cctype>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace tk;

namespace
{

int Usage()
{
    std::printf("usage:\n"
                "  rcftool list <archive.rcf> [filter]\n"
                "  rcftool extract <archive.rcf> <out folder> [filter]\n"
                "  rcftool pack <folder> <archive.rcf>\n"
                "  rcftool put <archive.rcf> <file> [name in archive]\n"
                "  rcftool remove <archive.rcf> <name in archive>\n"
                "  rcftool verify <archive.rcf | folder of .rcf>\n"
                "  rcftool hash <name>\n"
                "\n"
                "extract writes cement.manifest; pack reads it back, so an unedited folder repacks byte-exact.\n");
    return 1;
}

CementProgress ConsoleProgress()
{
    auto last = std::make_shared<int>(-1);
    return [last](uint64_t done, uint64_t total)
    {
        int percent = total ? static_cast<int>(done * 100 / total) : 100;

        if (percent != *last)
        {
            *last = percent;
            std::fprintf(stderr, "\r%3d%%", percent);

            if (percent == 100)
            {
                std::fprintf(stderr, "\n");
            }
        }

        return true;
    };
}

bool Matches(const std::string& name, const std::string& filter)
{
    if (filter.empty())
    {
        return true;
    }

    auto lower = [](std::string s)
    {
        for (char& c : s)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        return s;
    };
    return lower(name).find(lower(filter)) != std::string::npos;
}

bool OpenArchive(CementArchive& archive, const char* path)
{
    std::string error;

    if (archive.Open(path, error))
    {
        return true;
    }

    std::fprintf(stderr, "%s\n", error.c_str());
    return false;
}

std::string FormatTime(uint32_t unixTime)
{
    std::time_t time = unixTime;
    char text[32] = "?";

    if (std::tm* utc = std::gmtime(&time))
    {
        std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", utc);
    }

    return text;
}

int List(const char* path, const std::string& filter)
{
    CementArchive archive;

    if (!OpenArchive(archive, path))
    {
        return 1;
    }

    const CementHeader& h = archive.Header();
    std::printf("%s: cement %d.%d, %u files, alignment 0x%X\n",
        path,
        h.versionMajor,
        h.versionMinor,
        h.fileCount,
        archive.Alignment());
    std::printf("%-10s %-10s %-10s %-19s %s\n", "hash", "offset", "size", "time (UTC)", "name");

    for (const CementEntry& e : archive.Entries())
    {
        if (!Matches(e.name, filter))
        {
            continue;
        }

        std::printf("0x%08X 0x%08X %10u %s %s\n",
            e.hash,
            e.offset,
            e.size,
            FormatTime(e.timestamp).c_str(),
            e.name.c_str());
    }

    return 0;
}

int Extract(const char* path, const char* out, const std::string& filter)
{
    CementArchive archive;

    if (!OpenArchive(archive, path))
    {
        return 1;
    }

    std::vector<const CementEntry*> entries;

    for (const CementEntry& e : archive.Entries())
    {
        if (Matches(e.name, filter))
        {
            entries.push_back(&e);
        }
    }

    std::string error;

    if (!ExtractCement(archive, entries, out, filter.empty(), ConsoleProgress(), error))
    {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }

    std::printf("extracted %zu files to %s\n", entries.size(), out);
    return 0;
}

int Pack(const char* folder, const char* out)
{
    std::vector<CementSource> sources;
    uint32_t alignment = kCementAlignment;
    std::string error;

    if (!CollectCementFolder(folder, sources, alignment, error) ||
        !SaveCement(sources, alignment, out, ConsoleProgress(), error))
    {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }

    std::printf("packed %zu files into %s\n", sources.size(), out);
    return 0;
}

int Put(const char* path, const char* file, const char* nameArg)
{
    CementArchive archive;

    if (!OpenArchive(archive, path))
    {
        return 1;
    }

    std::string name = CementNormalizeName(nameArg ? nameArg : std::filesystem::path(file).filename().string());
    std::vector<CementSource> sources = SourcesFromArchive(archive);
    CementSource replacement = CementSource::FromFile(name, file);
    uint32_t hash = CementHash(name);
    bool replaced = false;

    for (CementSource& source : sources)
    {
        if (CementHash(source.name) != hash)
        {
            continue;
        }

        replacement.name = source.name;
        replacement.alignment = source.alignment;
        replacement.reserved = source.reserved;
        source = replacement;
        replaced = true;
    }

    if (!replaced)
    {
        replacement.alignment = archive.Alignment();
        sources.push_back(replacement);
    }

    uint32_t alignment = archive.Alignment();
    archive.Close();
    std::string error;

    if (!SaveCement(sources, alignment, path, ConsoleProgress(), error))
    {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }

    std::printf("%s %s\n", replaced ? "replaced" : "added", replacement.name.c_str());
    return 0;
}

int Remove(const char* path, const char* nameArg)
{
    CementArchive archive;

    if (!OpenArchive(archive, path))
    {
        return 1;
    }

    uint32_t hash = CementHash(CementNormalizeName(nameArg));
    std::vector<CementSource> sources;

    for (const CementEntry& e : archive.Entries())
    {
        if (e.hash != hash)
        {
            sources.push_back(CementSource::FromArchive(archive.Path(), e));
        }
    }

    if (sources.size() == archive.Entries().size())
    {
        std::fprintf(stderr, "%s is not in %s\n", nameArg, path);
        return 1;
    }

    uint32_t alignment = archive.Alignment();
    archive.Close();
    std::string error;

    if (!SaveCement(sources, alignment, path, ConsoleProgress(), error))
    {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }

    std::printf("removed %s\n", nameArg);
    return 0;
}

bool VerifyOne(const std::filesystem::path& path)
{
    CementArchive archive;
    std::string error;

    if (!archive.Open(path, error))
    {
        std::printf("FAIL  %s: %s\n", path.u8string().c_str(), error.c_str());
        return false;
    }

    std::vector<CementSource> sources = SourcesFromArchive(archive);
    CementCompareSink sink(path);

    if (!WriteCement(sources, archive.Alignment(), sink, nullptr, error))
    {
        std::printf("FAIL  %s: %s\n", path.u8string().c_str(), error.c_str());
        return false;
    }

    if (!sink.MatchedToEnd())
    {
        std::printf("DIFF  %s: rebuilt archive differs at 0x%llX\n", path.u8string().c_str(), sink.MismatchOffset());
        return false;
    }

    std::printf("OK    %s (%zu files)\n", path.u8string().c_str(), archive.Entries().size());
    return true;
}

int Verify(const char* target)
{
    std::vector<std::filesystem::path> paths;

    if (std::filesystem::is_directory(target))
    {
        for (const auto& item : std::filesystem::directory_iterator(target))
        {
            if (item.is_regular_file() && item.path().extension() == ".rcf")
            {
                paths.push_back(item.path());
            }
        }
    }
    else
    {
        paths.push_back(target);
    }

    size_t failed = 0;

    for (const auto& path : paths)
    {
        failed += !VerifyOne(path);
    }

    std::printf("%zu archives, %zu byte-exact, %zu failed\n", paths.size(), paths.size() - failed, failed);
    return failed ? 1 : 0;
}

}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        return Usage();
    }

    std::string command = argv[1];

    if (command == "list")
    {
        return List(argv[2], argc > 3 ? argv[3] : "");
    }

    if (command == "extract" && argc >= 4)
    {
        return Extract(argv[2], argv[3], argc > 4 ? argv[4] : "");
    }

    if (command == "pack" && argc >= 4)
    {
        return Pack(argv[2], argv[3]);
    }

    if (command == "put" && argc >= 4)
    {
        return Put(argv[2], argv[3], argc > 4 ? argv[4] : nullptr);
    }

    if (command == "remove" && argc >= 4)
    {
        return Remove(argv[2], argv[3]);
    }

    if (command == "verify")
    {
        return Verify(argv[2]);
    }

    if (command == "hash")
    {
        std::string name = CementNormalizeName(argv[2]);
        std::printf("0x%08X  %s\n", CementHash(name), name.c_str());
        return 0;
    }

    return Usage();
}
