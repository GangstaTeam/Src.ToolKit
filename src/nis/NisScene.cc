#include "nis/NisScene.hh"

#include "core/ByteReader.hh"
#include "core/FileIo.hh"
#include "nis/NisCatalog.hh"
#include "p3d/ChunkIds.hh"
#include "p3d/ChunkIo.hh"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>

namespace tk::nis
{

namespace
{

void ListInto(const std::vector<fsc::Node>& items,
    NodePath& path,
    size_t depth,
    const std::string& bank,
    std::vector<SceneNode>& out)
{
    for (size_t i = 0; i < items.size(); ++i)
    {
        const fsc::Node& item = items[i];

        if (!IsBranch(item))
        {
            continue;
        }

        path.push_back(i);
        SceneNode node;
        node.path = path;
        node.name = item.String("name");
        node.bank = bank;
        node.depth = depth;
        node.isBank = item.key == "fightBank";
        out.push_back(node);

        if (node.isBank)
        {
            ListInto(item.children, path, depth + 1, node.name, out);
        }

        path.pop_back();
    }
}

std::string Lower(std::string text)
{
    std::transform(text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
    return text;
}

int MsToFrames(double ms)
{
    return static_cast<int>(std::lround(ms * kFramesPerSecond / 1000.0));
}

std::string BaseAnimationName(const std::string& segment, int stream)
{
    std::string suffix = std::to_string(stream);

    if (segment.size() > suffix.size() && segment.compare(segment.size() - suffix.size(), suffix.size(), suffix) == 0)
    {
        return segment.substr(0, segment.size() - suffix.size());
    }

    return segment;
}

int StreamIndex(const std::filesystem::path& path)
{
    std::string stem = path.stem().u8string();
    return std::atoi(stem.c_str() + std::min<size_t>(stem.size(), 6));
}

std::string ReadPString(ByteReader& reader)
{
    uint8_t length = reader.U8();
    Bytes bytes = reader.Take(length);
    std::string text(bytes.begin(), bytes.end());
    size_t zero = text.find('\0');
    return zero == std::string::npos ? text : text.substr(0, zero);
}

void ReadStream(const std::filesystem::path& path, SceneAssets& assets)
{
    std::optional<Bytes> bytes = ReadFile(path);

    if (!bytes)
    {
        return;
    }

    ParseResult parsed = ParseP3d(*bytes);

    if (!parsed.root)
    {
        assets.error = path.filename().u8string() + ": " + parsed.error;
        return;
    }

    ++assets.streams;

    for (size_t i = 0; i < parsed.root->ChildCount(); ++i)
    {
        const Chunk* chunk = parsed.root->Child(i);

        if (chunk->Id() != chunk::Animation)
        {
            continue;
        }

        ByteReader reader(chunk->Payload());
        reader.U32();
        std::string name = ReadPString(reader);
        uint32_t type = reader.U32();
        float frames = reader.F32();

        if (reader.Failed())
        {
            continue;
        }

        char fourcc[5] = {};

        for (int k = 0; k < 4; ++k)
        {
            fourcc[k] = static_cast<char>((type >> (8 * k)) & 0xFF);
        }

        std::string base = BaseAnimationName(name, StreamIndex(path));
        AnimationInfo& info = assets.animations[Lower(base)];
        info.name = base;
        info.type = fourcc;
        info.frames += frames;
        ++info.segments;
    }
}

float AnimationFrames(const SceneAssets& assets, const std::string& name)
{
    const AnimationInfo* info = assets.FindAnimation(name);
    return info ? info->frames : 0.0f;
}

std::string Raw(const fsc::Node& node, const char* key)
{
    const fsc::Node* child = node.Find(key);
    return child && !child->block ? child->value : std::string();
}

std::string Vector(const fsc::Node& track, const char* key)
{
    const fsc::Node* vector = track.Find(key);

    if (!vector || !vector->block)
    {
        return {};
    }

    return Raw(*vector, "x") + ", " + Raw(*vector, "y") + ", " + Raw(*vector, "z");
}

bool Contains(const std::vector<CastMember>& members, const std::string& name)
{
    for (const CastMember& member : members)
    {
        if (member.name == name)
        {
            return true;
        }
    }

    return false;
}

}

std::vector<SceneNode> ListNodes(const std::vector<fsc::Node>& roots)
{
    std::vector<SceneNode> out;
    NodePath path;
    ListInto(roots, path, 0, {}, out);
    return out;
}

fsc::Node* Resolve(std::vector<fsc::Node>& roots, const NodePath& path)
{
    std::vector<fsc::Node>* items = &roots;
    fsc::Node* node = nullptr;

    for (size_t index : path)
    {
        if (index >= items->size())
        {
            return nullptr;
        }

        node = &(*items)[index];
        items = &node->children;
    }

    return node;
}

const fsc::Node* Resolve(const std::vector<fsc::Node>& roots, const NodePath& path)
{
    return Resolve(const_cast<std::vector<fsc::Node>&>(roots), path);
}

NodePath ResolveBranch(const std::vector<fsc::Node>& roots, const NodePath& from, const std::string& branch)
{
    NodePath path = from;
    size_t start = 0;

    while (start <= branch.size())
    {
        size_t slash = branch.find('/', start);
        std::string part = branch.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        start = slash == std::string::npos ? branch.size() + 1 : slash + 1;

        if (part.empty() || part == ".")
        {
            continue;
        }

        if (part == "..")
        {
            if (path.empty())
            {
                return {};
            }

            path.pop_back();
            continue;
        }

        const std::vector<fsc::Node>* items = &roots;

        if (!path.empty())
        {
            const fsc::Node* current = Resolve(roots, path);

            if (!current)
            {
                return {};
            }

            items = &current->children;
        }

        bool found = false;

        for (size_t i = 0; i < items->size() && !found; ++i)
        {
            if (IsBranch((*items)[i]) && (*items)[i].String("name") == part)
            {
                path.push_back(i);
                found = true;
            }
        }

        if (!found)
        {
            return {};
        }
    }

    return path == from ? NodePath() : path;
}

bool IsTrack(const fsc::Node& child)
{
    return child.block && child.key != "conditionGroup";
}

bool IsBranch(const fsc::Node& child)
{
    return child.block && (child.key == "fightBank" || child.key == "fightNode");
}

std::vector<size_t> TrackIndices(const fsc::Node& fightNode)
{
    std::vector<size_t> indices;

    for (size_t i = 0; i < fightNode.children.size(); ++i)
    {
        if (IsTrack(fightNode.children[i]))
        {
            indices.push_back(i);
        }
    }

    return indices;
}

void UpdateCounts(fsc::Node& block)
{
    if (block.key == "fightNode")
    {
        block.SetInt("trackCount", static_cast<long long>(TrackIndices(block).size()));
    }
    else if (block.key == "fightBank")
    {
        long long branches = std::count_if(block.children.begin(), block.children.end(), IsBranch);
        block.SetInt("branchCount", branches);
    }
}

std::string SceneName(const std::vector<fsc::Node>& roots)
{
    for (const fsc::Node& root : roots)
    {
        if (root.key == "fightBank")
        {
            return root.String("name");
        }
    }

    return {};
}

void RenameScene(std::vector<fsc::Node>& roots, const std::string& name)
{
    std::string old = SceneName(roots);
    std::vector<fsc::Node*> pending;

    for (fsc::Node& root : roots)
    {
        pending.push_back(&root);
    }

    while (!pending.empty())
    {
        fsc::Node* node = pending.back();
        pending.pop_back();

        if (node->key != "fightBank" || node->String("name") != old)
        {
            continue;
        }

        node->SetString("name", name);

        for (fsc::Node& child : node->children)
        {
            pending.push_back(&child);
        }
    }
}

int SceneAssets::Frames() const
{
    float longest = 0.0f;

    for (const auto& entry : animations)
    {
        longest = std::max(longest, entry.second.frames);
    }

    return static_cast<int>(std::ceil(longest));
}

const AnimationInfo* SceneAssets::FindAnimation(const std::string& name) const
{
    auto match = animations.find(Lower(name));
    return match == animations.end() ? nullptr : &match->second;
}

SceneAssets LoadAssets(const std::filesystem::path& fscPath)
{
    std::filesystem::path stem = fscPath.stem();
    std::error_code ec;

    for (const std::filesystem::path& base : {fscPath.parent_path().parent_path(), fscPath.parent_path()})
    {
        if (std::filesystem::is_directory(base / stem, ec))
        {
            return LoadSceneFolder(base / stem);
        }
    }

    return {};
}

SceneAssets LoadSceneFolder(const std::filesystem::path& folder)
{
    SceneAssets assets;
    std::error_code ec;

    if (!std::filesystem::is_directory(folder, ec))
    {
        return assets;
    }

    assets.folder = folder;
    assets.loader = std::filesystem::exists(folder.parent_path() / (folder.filename().wstring() + L".p3d"), ec);
    assets.found = true;
    std::vector<std::filesystem::path> streams;

    for (const auto& entry : std::filesystem::directory_iterator(assets.folder, ec))
    {
        std::string name = Lower(entry.path().filename().u8string());

        if (name.rfind("stream", 0) == 0 && entry.path().extension() == ".p3d")
        {
            streams.push_back(entry.path());
        }
    }

    std::sort(streams.begin(), streams.end());

    for (const auto& stream : streams)
    {
        ReadStream(stream, assets);
    }

    return assets;
}

bool Cast::HasCharacter(const std::string& name) const
{
    return Contains(characters, name);
}

bool Cast::HasVehicle(const std::string& name) const
{
    return Contains(vehicles, name);
}

Cast CollectCast(const std::vector<fsc::Node>& roots)
{
    Cast cast;
    cast.characters.push_back({"MainCharacter", "player (Tony)"});
    std::vector<const fsc::Node*> pending;

    for (const fsc::Node& root : roots)
    {
        pending.push_back(&root);
    }

    std::set<std::string> objects;

    while (!pending.empty())
    {
        const fsc::Node* node = pending.back();
        pending.pop_back();

        for (auto it = node->children.rbegin(); it != node->children.rend(); ++it)
        {
            if (it->block)
            {
                pending.push_back(&*it);
            }
        }

        if (node->key == "NISResourceLoader")
        {
            std::string name = node->String("Name");

            if (!name.empty() && !cast.HasCharacter(name))
            {
                cast.characters.push_back({name, node->String("TemplateName")});
            }
        }
        else if (node->key == "NISActivateVehicle")
        {
            std::string name = node->String("VehicleName");

            if (!name.empty() && !cast.HasVehicle(name))
            {
                cast.vehicles.push_back({name, node->String("VehicleType")});
            }
        }
        else if (node->key == "NISRenderableTrack")
        {
            objects.insert(node->String("RenderableName"));
        }
        else if (node->key == "NISStatePropSpawn")
        {
            objects.insert(node->String("StatePropName"));
        }
        else if (node->key == "NISObjectPosition" || node->key == "NISCharacterAttach")
        {
            objects.insert(node->String("ObjectName"));
        }
        else if (node->key == "NISStatePropState")
        {
            objects.insert(node->String("StatePropName"));
        }
        else if (node->key == "NISDeleteScriptObject")
        {
            objects.insert(node->String("Name"));
        }
    }

    objects.erase("");
    cast.objects.assign(objects.begin(), objects.end());
    return cast;
}

Timing TrackTiming(const fsc::Node& track, const SceneAssets& assets)
{
    Timing timing;

    if (track.Has("FrameBegin"))
    {
        timing.timed = true;
        timing.begin = static_cast<int>(track.Int("FrameBegin"));
    }
    else if (track.Has("TimeBegin"))
    {
        timing.timed = true;
        timing.begin = MsToFrames(track.Number("TimeBegin"));
    }

    const std::string& type = track.key;

    if (type == "NISFade")
    {
        timing.length =
            static_cast<int>(track.Int("FadeInFrames") + track.Int("DurationFrames") + track.Int("FadeOutFrames"));
    }
    else if (type == "NISWeaponFire")
    {
        timing.length = static_cast<int>(track.Int("FrameEnd") - track.Int("FrameBegin"));
    }
    else if (type == "SoundFade")
    {
        timing.length = MsToFrames(track.Number("fadeTime"));
    }
    else if (type == "NISScriptFunctionCallTrack")
    {
        Subtitle subtitle;

        if (ParseSubtitle(track.String("ScriptFunctionCall"), subtitle))
        {
            timing.length = MsToFrames(subtitle.milliseconds);
        }
    }
    else if (type == "NISSequenceCue" || type == "NISCameraAnimation")
    {
        timing.length = static_cast<int>(AnimationFrames(assets, track.String("CameraAnimationName")));
    }
    else if (track.Has("AnimationName"))
    {
        timing.length = static_cast<int>(AnimationFrames(assets, track.String("AnimationName")));
    }

    timing.length = std::max(timing.length, 0);
    return timing;
}

void SetTrackStart(fsc::Node& track, int frame)
{
    frame = std::max(frame, 0);

    if (track.Has("FrameBegin"))
    {
        if (track.Has("FrameEnd"))
        {
            track.SetInt("FrameEnd", track.Int("FrameEnd") + frame - track.Int("FrameBegin"));
        }

        track.SetInt("FrameBegin", frame);
    }
    else if (track.Has("TimeBegin"))
    {
        track.SetInt("TimeBegin", static_cast<long long>(std::lround(frame * 1000.0 / kFramesPerSecond)));
    }
}

bool ParseSubtitle(const std::string& call, Subtitle& subtitle)
{
    const std::string function = "SubtitleManager_LocalisedDialogueEvent";
    size_t at = call.find(function);

    if (at == std::string::npos)
    {
        return false;
    }

    size_t open = call.find('(', at), close = call.rfind(')');

    if (open == std::string::npos || close == std::string::npos || close < open)
    {
        return false;
    }

    std::vector<std::string> args;
    std::string current;

    for (size_t i = open + 1; i < close; ++i)
    {
        if (call[i] == ',')
        {
            args.push_back(current);
            current.clear();
        }
        else
        {
            current += call[i];
        }
    }

    args.push_back(current);

    if (args.size() < 3)
    {
        return false;
    }

    std::string line = args[0];
    line.erase(std::remove_if(line.begin(),
                   line.end(),
                   [](char c)
                   {
                       return c == '^' || c == ' ';
                   }),
        line.end());
    size_t slash = line.find_last_of('\\');
    subtitle.line = slash == std::string::npos ? line : line.substr(slash + 1);
    subtitle.milliseconds = std::atoi(args[2].c_str());
    return true;
}

std::string TrackSubject(const fsc::Node& track)
{
    for (const char* key :
        {"CharacterName", "Character", "Name", "VehicleName", "ObjectName", "RenderableName", "StatePropName"})
    {
        if (track.Has(key))
        {
            return track.String(key);
        }
    }

    if (track.key == "SoundFade")
    {
        return track.String("mixName");
    }

    if (track.key == "sequence")
    {
        return track.String("branch");
    }

    if (track.key == "NISScriptFunctionCallTrack")
    {
        Subtitle subtitle;
        std::string call = track.String("ScriptFunctionCall");

        if (ParseSubtitle(call, subtitle))
        {
            return "subtitle";
        }

        return call.substr(0, call.find('('));
    }

    return {};
}

std::string TrackSummary(const fsc::Node& track)
{
    const std::string& type = track.key;
    char text[256];

    if (type == "NISSequenceCue" || type == "NISCameraAnimation")
    {
        return track.String("CameraAnimationName");
    }

    if (track.Has("AnimationName"))
    {
        return track.String("AnimationName");
    }

    if (type == "NISCharacterPosition" || type == "NISObjectPosition")
    {
        return Vector(track, "Position") + "   " + Raw(track, "Facing") + "\xC2\xB0";
    }

    if (type == "NISHoldCameraPosition")
    {
        return Vector(track, "Position");
    }

    if (type == "NISSwapModel")
    {
        return track.String("newModel");
    }

    if (type == "NISResourceLoader")
    {
        return track.String("TemplateName");
    }

    if (type == "NISWeaponAttach")
    {
        std::string weapon = track.String("WeaponTemplateName");
        return weapon.empty() ? "(holster)" : weapon;
    }

    if (type == "NISWeaponFire")
    {
        std::snprintf(text,
            sizeof(text),
            "frames %lld-%lld%s",
            track.Int("FrameBegin"),
            track.Int("FrameEnd"),
            track.Int("IsSecondaryWeapon") ? " secondary" : "");
        return text;
    }

    if (type == "NISFade")
    {
        std::snprintf(text,
            sizeof(text),
            "in %lld / hold %lld / out %lld   rgb %lld %lld %lld",
            track.Int("FadeInFrames"),
            track.Int("DurationFrames"),
            track.Int("FadeOutFrames"),
            track.Int("Red"),
            track.Int("Green"),
            track.Int("Blue"));
        return text;
    }

    if (type == "SoundFade")
    {
        std::snprintf(text,
            sizeof(text),
            "%s %lld ms",
            track.Int("isFadeIn") ? "fade in" : "fade out",
            track.Int("fadeTime"));
        return text;
    }

    if (type == "NISScriptFunctionCallTrack")
    {
        Subtitle subtitle;
        std::string call = track.String("ScriptFunctionCall");

        if (!ParseSubtitle(call, subtitle))
        {
            return call;
        }

        std::snprintf(text, sizeof(text), "%s  (%d ms)", subtitle.line.c_str(), subtitle.milliseconds);
        return text;
    }

    if (type == "sequence")
    {
        return "-> " + track.String("branch");
    }

    if (type == "NISCharacterEffect")
    {
        return track.String("EffectType") + " @ " + track.String("JointName");
    }

    if (type == "NISCharacterDamage")
    {
        return Raw(track, "Damage") + " @ " + track.String("JointName");
    }

    if (type == "NISStatePropState")
    {
        return track.String("StateName");
    }

    if (type == "NISStatePropSpawn")
    {
        return track.String("StatePropTemplateName");
    }

    if (type == "NISActivateVehicle")
    {
        return track.String("VehicleType");
    }

    if (type == "NISRenderableTrack")
    {
        return track.String("CompositeDrawableName");
    }

    if (type == "NISCharacterVisibility")
    {
        return track.Int("IsVisible") ? "show" : "hide";
    }

    if (type == "NISCharacterAttach")
    {
        return (track.Int("Detach") ? "detach from " : "attach to ") + track.String("CharacterJointName");
    }

    if (type == "NISDriveWorldStreamer")
    {
        return track.Int("Enable") ? "on" : "off";
    }

    if (type == "NISVehicleProperty")
    {
        return "thrust " + Raw(track, "ThrustFactor");
    }

    return {};
}

std::vector<Issue> Validate(const std::vector<fsc::Node>& roots, const SceneAssets& assets)
{
    std::vector<Issue> issues;
    Cast cast = CollectCast(roots);
    bool cue = false, play = false;

    for (const SceneNode& sceneNode : ListNodes(roots))
    {
        const fsc::Node& block = *Resolve(roots, sceneNode.path);
        auto report = [&](Severity severity, int track, std::string message)
        {
            issues.push_back({severity, sceneNode.path, track, std::move(message)});
        };

        if (sceneNode.isBank)
        {
            long long branches = std::count_if(block.children.begin(), block.children.end(), IsBranch);

            if (block.Int("branchCount", -1) != branches)
            {
                report(Severity::Warning,
                    -1,
                    "branchCount is " + std::to_string(block.Int("branchCount", -1)) + " but the bank has " +
                        std::to_string(branches) + " branches");
            }

            continue;
        }

        std::vector<size_t> tracks = TrackIndices(block);

        if (block.Int("trackCount", -1) != static_cast<long long>(tracks.size()))
        {
            report(Severity::Warning,
                -1,
                "trackCount is " + std::to_string(block.Int("trackCount", -1)) + " but the node has " +
                    std::to_string(tracks.size()) + " tracks");
        }

        for (size_t index : tracks)
        {
            const fsc::Node& track = block.children[index];
            int at = static_cast<int>(index);
            const TrackSpec* spec = FindTrack(track.key);

            if (!spec)
            {
                report(Severity::Info, at, "unknown track type '" + track.key + "' is kept as it is");
                continue;
            }

            cue |= track.key == "NISSequenceCue";
            play |= track.key == "NISSequencePlay";

            for (const FieldSpec& field : spec->fields)
            {
                const fsc::Node* value = track.Find(field.key);

                if (!value)
                {
                    report(Severity::Warning, at, std::string(spec->label) + ": missing " + field.key);
                    continue;
                }

                if (field.kind != FieldKind::String)
                {
                    continue;
                }

                std::string text = track.String(field.key);

                switch (field.ref)
                {
                case Ref::Character:
                    if (text.empty())
                    {
                        report(Severity::Warning, at, std::string(spec->label) + ": no character set");
                    }
                    else if (!cast.HasCharacter(text))
                    {
                        report(Severity::Info,
                            at,
                            std::string(spec->label) + ": '" + text +
                                "' is not loaded here, so the mission must have spawned it");
                    }
                    break;
                case Ref::Vehicle:
                    if (!cast.HasVehicle(text))
                    {
                        report(Severity::Info,
                            at,
                            std::string(spec->label) + ": vehicle '" + text +
                                "' is not activated here, so the mission must have spawned it");
                    }
                    break;
                case Ref::BodyAnimation:
                case Ref::FaceAnimation:
                case Ref::CameraAnimation:
                    if (text.empty())
                    {
                        if (field.ref != Ref::CameraAnimation)
                        {
                            report(Severity::Warning, at, std::string(spec->label) + ": no animation set");
                        }
                    }
                    else if (assets.found && !assets.FindAnimation(text))
                    {
                        report(Severity::Warning,
                            at,
                            std::string(spec->label) + ": '" + text + "' is not in the scene's streams");
                    }
                    break;
                case Ref::Branch:
                    if (ResolveBranch(roots, sceneNode.path, text).empty())
                    {
                        report(Severity::Error, at, "Branch: '" + text + "' does not lead to a node");
                    }
                    break;
                default:
                    break;
                }
            }

            for (const fsc::Node& child : track.children)
            {
                if (!FindField(*spec, child.key))
                {
                    report(Severity::Info, at, std::string(spec->label) + ": extra field '" + child.key + "'");
                }
            }

            if (track.Has("FrameBegin") && track.Int("FrameBegin") < 0)
            {
                report(Severity::Warning, at, std::string(spec->label) + ": negative start frame");
            }

            if (track.key == "NISWeaponFire" && track.Int("FrameEnd") < track.Int("FrameBegin"))
            {
                report(Severity::Warning, at, "Weapon fire: FrameEnd is before FrameBegin");
            }

            if (track.key == "NISFade")
            {
                for (const char* channel : {"Red", "Green", "Blue"})
                {
                    if (track.Int(channel) < 0 || track.Int(channel) > 255)
                    {
                        report(Severity::Warning, at, std::string("Screen fade: ") + channel + " is outside 0-255");
                    }
                }
            }
        }
    }

    if (!cue)
    {
        issues.push_back({Severity::Warning, {}, -1, "no Sequence cue: the scene never sets up its camera"});
    }

    if (!play)
    {
        issues.push_back({Severity::Warning, {}, -1, "no Sequence play: the scene never starts"});
    }

    if (!assets.error.empty())
    {
        issues.push_back({Severity::Info, {}, -1, assets.error});
    }

    return issues;
}

}
