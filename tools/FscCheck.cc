#include "core/FileIo.hh"
#include "nis/FscTree.hh"
#include "nis/NisScene.hh"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

using namespace tk;

namespace
{

int Usage()
{
    std::printf("usage: fsccheck [-v] <file.fsc | folder>\n"
                "\n"
                "Parses every .fsc, checks it writes back byte for byte and reports NIS scene problems\n"
                "(missing fields, unknown characters, animations missing from the scene's streams, broken branches).\n"
                "-v also prints warnings and notes, not only errors.\n");
    return 1;
}

const char* SeverityName(nis::Severity severity)
{
    switch (severity)
    {
    case nis::Severity::Error:
        return "error";
    case nis::Severity::Warning:
        return "warning";
    case nis::Severity::Info:
        return "note";
    }

    return "";
}

}

int main(int argc, char** argv)
{
    bool verbose = false;
    std::filesystem::path target;

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "-v") == 0)
        {
            verbose = true;
        }
        else
        {
            target = std::filesystem::u8path(argv[i]);
        }
    }

    if (target.empty())
    {
        return Usage();
    }

    std::vector<std::filesystem::path> files;

    if (std::filesystem::is_directory(target))
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(target))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".fsc")
            {
                files.push_back(entry.path());
            }
        }
    }
    else
    {
        files.push_back(target);
    }

    size_t parseFailures = 0, mismatches = 0, errors = 0, warnings = 0;

    for (const auto& path : files)
    {
        std::optional<std::string> text = ReadText(path);
        std::vector<fsc::Node> roots;
        std::string error;

        if (!text || !fsc::Parse(*text, roots, error))
        {
            ++parseFailures;
            std::printf("%s: %s\n", path.u8string().c_str(), text ? error.c_str() : "cannot read");
            continue;
        }

        if (fsc::Write(roots) != *text)
        {
            ++mismatches;
            std::printf("%s: ROUNDTRIP MISMATCH\n", path.u8string().c_str());
        }

        nis::SceneAssets assets = nis::LoadAssets(path);

        for (const nis::Issue& issue : nis::Validate(roots, assets))
        {
            issue.severity == nis::Severity::Error ? ++errors : ++warnings;

            if (issue.severity != nis::Severity::Error && !verbose)
            {
                continue;
            }

            std::string where;

            if (const fsc::Node* node = nis::Resolve(roots, issue.node))
            {
                where = " [" + node->String("name") + "]";
            }

            std::printf("%s%s: %s: %s\n",
                path.filename().u8string().c_str(),
                where.c_str(),
                SeverityName(issue.severity),
                issue.message.c_str());
        }
    }

    std::printf("files %zu, parse failures %zu, roundtrip mismatches %zu, errors %zu, warnings/notes %zu\n",
        files.size(),
        parseFailures,
        mismatches,
        errors,
        warnings);
    return parseFailures || mismatches || errors ? 2 : 0;
}
