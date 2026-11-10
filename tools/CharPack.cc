#include "core/FileIo.hh"
#include "p3d/ChunkIo.hh"
#include "rig/NewCharacter.hh"

#include <cstdio>
#include <filesystem>
#include <string>

using namespace tk;

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::printf("usage: charpack <character package.p3d> <NewName> [game folder]\n");
        return 1;
    }

    std::filesystem::path source = std::filesystem::u8path(argv[1]);
    std::optional<GameFolders> folders = argc > 3
        ? std::optional<GameFolders>(GameFolders{std::filesystem::u8path(argv[3])})
        : GameFoldersFromPackage(std::filesystem::absolute(source));
    if (!folders)
    {
        std::printf("cannot find the game folder from %s; pass it as third argument\n", source.u8string().c_str());
        return 1;
    }

    auto bytes = ReadFile(source);
    ParseResult parsed = bytes ? ParseP3d(*bytes) : ParseResult{};

    if (!parsed.root)
    {
        std::printf("cannot read %s\n", source.u8string().c_str());
        return 1;
    }

    std::string report;
    bool ok = SaveNewCharacter(*parsed.root, argv[2], *folders, report);
    std::printf("%s\n", report.c_str());
    return ok ? 0 : 1;
}
