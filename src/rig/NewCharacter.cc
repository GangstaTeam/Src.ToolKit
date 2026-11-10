#include "rig/NewCharacter.hh"

#include "core/FileIo.hh"
#include "p3d/ChunkIds.hh"
#include "p3d/ChunkIo.hh"
#include "rig/ChunkText.hh"
#include "rig/PackageRename.hh"
#include "rig/PackageWriter.hh"

#include <set>

namespace tk
{

bool IsOriginalCharacter(const GameFolders& folders, const std::string& name)
{
    std::filesystem::path original = folders.LoadGroup();
    original += ".orig";
    std::error_code ec;
    auto bytes = ReadFile(std::filesystem::exists(original, ec) ? original : folders.LoadGroup());
    ParseResult parsed = bytes ? ParseP3d(*bytes) : ParseResult{};
    return parsed.root && HasLoadObject(*parsed.root, name);
}

bool SaveNewCharacter(const Chunk& package, const std::string& name, const GameFolders& folders, std::string& report)
{
    if (!IsValidPackageName(name))
    {
        report = "Use only letters, digits and _ for the name.";
        return false;
    }

    if (IsOriginalCharacter(folders, name))
    {
        report = name + " is one of the game's own characters. Pick another name.";
        return false;
    }

    std::unique_ptr<Chunk> copy = package.Clone();
    PackageTarget target = FindPackageTarget(*copy);

    if (!target.Valid())
    {
        report = "The open file is not a character package.";
        return false;
    }

    std::set<std::string> olds = {target.skinName, target.skeletonName};

    for (size_t i = 0; i < copy->ChildCount(); ++i)
    {
        if (copy->Child(i)->Id() == chunk::CharacterTemplate)
        {
            olds.insert(LeadingName(*copy->Child(i)));
        }
    }

    olds.erase(name);
    std::string from;
    size_t renamed = 0;

    for (const std::string& old : olds)
    {
        if (old.empty())
        {
            continue;
        }

        renamed += RenamePackage(*copy, old, name);
        from += (from.empty() ? "" : ", ") + old;
    }

    std::filesystem::path path = folders.Package(name);

    if (!WriteFile(path, SerializeP3d(*copy)))
    {
        report = "cannot write " + path.u8string();
        return false;
    }

    if (!RegisterMainCharacter(folders, name, report))
    {
        return false;
    }

    report = "Saved " + path.filename().u8string() + " (" + std::to_string(renamed) + " names changed from " + from +
        ") and added it to pc_loadgroup.p3d.";
    return true;
}

}
