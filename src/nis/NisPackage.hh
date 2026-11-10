#pragma once

#include <cstddef>
#include <cstdint>

#include "core/Bytes.hh"
#include "nis/FscTree.hh"

#include <filesystem>
#include <string>
#include <vector>

namespace tk::nis
{

enum ENisPackage : int
{
    kFramesPerStream = 130,
};

std::filesystem::path NisFolder(const std::filesystem::path& gameFolder);
std::filesystem::path DialogueFolder(const std::filesystem::path& gameFolder);
std::filesystem::path DetectGameFolder(const std::filesystem::path& file);
std::vector<std::string> ListSceneFolders(const std::filesystem::path& gameFolder);

struct StreamPartition
{
    int index = 0;
    int count = 0;
    int frames = 0;
};

bool ReadPartition(const std::filesystem::path& stream, StreamPartition& partition);

struct LoaderInfo
{
    std::string scene;
    std::string staticScene;
    std::string dialogue;
    int streams = 0;
    int frames = 0;
};

Bytes MakeLoader(const LoaderInfo& info);
bool ReadLoader(const Bytes& bytes, LoaderInfo& info);

int RenameDialogue(Bytes& bytes, const std::string& from, const std::string& to, std::string& error);

struct BuildOptions
{
    std::string scene;
    std::string assetsFrom;
    bool dialogue = true;
    bool listInCallnis = true;
    bool overwrite = false;
};

struct BuildPlan
{
    std::vector<std::filesystem::path> writes;
    std::vector<std::string> notes;
    std::string error;
    bool inPlace = false;
};

BuildPlan PlanBuild(const std::filesystem::path& gameFolder, const BuildOptions& options);
BuildPlan
Build(const std::filesystem::path& gameFolder, const BuildOptions& options, const std::vector<fsc::Node>& roots);

bool IsValidSceneName(const std::string& name);

}
