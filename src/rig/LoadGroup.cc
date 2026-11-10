#include "rig/LoadGroup.hh"

#include "core/ByteWriter.hh"
#include "core/FileIo.hh"
#include "p3d/ChunkIds.hh"
#include "p3d/ChunkIo.hh"
#include "rig/ChunkText.hh"

#include <algorithm>
#include <cctype>

namespace tk
{

namespace
{

void WriteName(ByteWriter& writer, const std::string& text)
{
    size_t stored = (text.size() + 3) / 4 * 4;
    writer.U8(static_cast<uint8_t>(stored));
    writer.Append(reinterpret_cast<const uint8_t*>(text.data()), text.size());

    for (size_t i = text.size(); i < stored; ++i)
    {
        writer.U8(0);
    }
}

bool SameFolder(const std::filesystem::path& path, const char* name)
{
    std::string a = path.filename().u8string(), b = name;
    return std::equal(a.begin(),
        a.end(),
        b.begin(),
        b.end(),
        [](char x, char y)
        {
            return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
        });
}

}

std::filesystem::path GameFolders::Package(const std::string& name) const
{
    return root / "packages" / "characters" / "pc" / std::filesystem::u8path(name + ".p3d");
}

std::filesystem::path GameFolders::LoadGroup() const
{
    return root / "art" / "characters" / "pc" / "pc_loadgroup.p3d";
}

std::optional<GameFolders> GameFoldersFromPackage(const std::filesystem::path& package)
{
    std::filesystem::path pc = package.parent_path(), characters = pc.parent_path(),
                          packages = characters.parent_path();
    if (!SameFolder(pc, "pc") || !SameFolder(characters, "characters") || !SameFolder(packages, "packages"))
    {
        return std::nullopt;
    }

    GameFolders folders{packages.parent_path()};
    std::error_code ec;

    if (!std::filesystem::exists(folders.LoadGroup(), ec))
    {
        return std::nullopt;
    }

    return folders;
}

bool HasLoadObject(const Chunk& loadGroup, const std::string& name)
{
    for (size_t i = 0; i < loadGroup.ChildCount(); ++i)
    {
        const Chunk& entry = *loadGroup.Child(i);

        if (entry.Id() == chunk::LoadObject && LeadingName(entry) == name)
        {
            return true;
        }
    }

    return false;
}

void AddMainCharacter(Chunk& loadGroup, const std::string& name)
{
    ByteWriter payload;
    WriteName(payload, name);
    payload.U16(kMainCharacter);
    payload.U8(0);
    payload.U32(1);
    WriteName(payload, "packages/characters/pc/" + name + ".p3d");
    loadGroup.AppendChild(std::make_unique<Chunk>(chunk::LoadObject, payload.Take()));
}

bool RegisterMainCharacter(const GameFolders& folders, const std::string& name, std::string& error)
{
    std::filesystem::path path = folders.LoadGroup();
    auto bytes = ReadFile(path);
    ParseResult parsed = bytes ? ParseP3d(*bytes) : ParseResult{};

    if (!parsed.root)
    {
        error = "cannot read " + path.u8string();
        return false;
    }

    if (HasLoadObject(*parsed.root, name))
    {
        return true;
    }

    std::filesystem::path backup = path;
    backup += ".orig";
    std::error_code ec;

    if (!std::filesystem::exists(backup, ec))
    {
        std::filesystem::copy_file(path, backup, ec);
    }

    AddMainCharacter(*parsed.root, name);

    if (!WriteFile(path, SerializeP3d(*parsed.root)))
    {
        error = "cannot write " + path.u8string();
        return false;
    }

    return true;
}

}
