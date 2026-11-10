#pragma once

#include <cstddef>
#include <cstdint>

#include "p3d/Chunk.hh"

#include <filesystem>
#include <optional>
#include <string>

namespace tk
{

enum ELoadGroupShort : uint16_t
{
    kMainCharacter = 6,
};

struct GameFolders
{
    std::filesystem::path root;

    std::filesystem::path Package(const std::string& name) const;
    std::filesystem::path LoadGroup() const;
};

std::optional<GameFolders> GameFoldersFromPackage(const std::filesystem::path& package);
void AddMainCharacter(Chunk& loadGroup, const std::string& name);
bool HasLoadObject(const Chunk& loadGroup, const std::string& name);
bool RegisterMainCharacter(const GameFolders& folders, const std::string& name, std::string& error);

}
