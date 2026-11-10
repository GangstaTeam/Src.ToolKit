#pragma once

#include <cstddef>
#include <cstdint>

#include "nis/FscTree.hh"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace tk::nis
{

enum ENisScene : int
{
    kFramesPerSecond = 30,
};

using NodePath = std::vector<size_t>;

struct SceneNode
{
    NodePath path;
    std::string name;
    std::string bank;
    size_t depth = 0;
    bool isBank = false;
};

std::vector<SceneNode> ListNodes(const std::vector<fsc::Node>& roots);
fsc::Node* Resolve(std::vector<fsc::Node>& roots, const NodePath& path);
const fsc::Node* Resolve(const std::vector<fsc::Node>& roots, const NodePath& path);
NodePath ResolveBranch(const std::vector<fsc::Node>& roots, const NodePath& from, const std::string& branch);

bool IsTrack(const fsc::Node& child);
bool IsBranch(const fsc::Node& child);
std::vector<size_t> TrackIndices(const fsc::Node& fightNode);
void UpdateCounts(fsc::Node& block);
std::string SceneName(const std::vector<fsc::Node>& roots);
void RenameScene(std::vector<fsc::Node>& roots, const std::string& name);

struct AnimationInfo
{
    std::string name;
    std::string type;
    float frames = 0.0f;
    int segments = 0;
};

struct SceneAssets
{
    std::filesystem::path folder;
    bool found = false;
    bool loader = false;
    int streams = 0;
    std::map<std::string, AnimationInfo> animations;
    std::string error;
    int Frames() const;
    const AnimationInfo* FindAnimation(const std::string& name) const;
};

SceneAssets LoadAssets(const std::filesystem::path& fscPath);
SceneAssets LoadSceneFolder(const std::filesystem::path& folder);

struct CastMember
{
    std::string name;
    std::string detail;
};

struct Cast
{
    std::vector<CastMember> characters;
    std::vector<CastMember> vehicles;
    std::vector<std::string> objects;
    bool HasCharacter(const std::string& name) const;
    bool HasVehicle(const std::string& name) const;
};

Cast CollectCast(const std::vector<fsc::Node>& roots);

struct Timing
{
    bool timed = false;
    int begin = 0;
    int length = 0;
};

Timing TrackTiming(const fsc::Node& track, const SceneAssets& assets);
void SetTrackStart(fsc::Node& track, int frame);

struct Subtitle
{
    std::string line;
    int milliseconds = 0;
};

bool ParseSubtitle(const std::string& call, Subtitle& subtitle);
std::string TrackSubject(const fsc::Node& track);
std::string TrackSummary(const fsc::Node& track);

enum class Severity
{
    Error,
    Warning,
    Info
};

struct Issue
{
    Severity severity = Severity::Warning;
    NodePath node;
    int track = -1;
    std::string message;
};

std::vector<Issue> Validate(const std::vector<fsc::Node>& roots, const SceneAssets& assets);

}
