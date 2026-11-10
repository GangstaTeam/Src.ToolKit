#include "nis/NisPackage.hh"

#include "core/ByteReader.hh"
#include "core/ByteWriter.hh"
#include "core/FileIo.hh"
#include "nis/NisScene.hh"
#include "p3d/ChunkIds.hh"
#include "p3d/ChunkIo.hh"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace tk::nis
{

namespace
{

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

bool SameName(const std::string& a, const std::string& b)
{
    return Lower(a) == Lower(b);
}

void WritePString(ByteWriter& writer, const std::string& text)
{
    size_t length = text.size();

    while ((length + 1) % 4)
    {
        ++length;
    }

    writer.U8(static_cast<uint8_t>(length));
    writer.Append(reinterpret_cast<const uint8_t*>(text.data()), text.size());

    for (size_t i = text.size(); i < length; ++i)
    {
        writer.U8(0);
    }
}

std::string ReadPString(ByteReader& reader)
{
    uint8_t length = reader.U8();
    Bytes bytes = reader.Take(length);
    std::string text(bytes.begin(), bytes.end());
    size_t zero = text.find('\0');
    return zero == std::string::npos ? text : text.substr(0, zero);
}

Bytes Serialize(uint32_t id, const Bytes& payload, const Bytes& child)
{
    ByteWriter writer;
    writer.U32(id);
    writer.U32(static_cast<uint32_t>(12 + payload.size()));
    writer.U32(static_cast<uint32_t>(12 + payload.size() + child.size()));
    writer.Append(payload);
    writer.Append(child);
    return writer.Take();
}

int StreamNumber(const std::filesystem::path& path)
{
    std::string stem = Lower(path.stem().u8string());
    return stem.rfind("stream", 0) == 0 ? std::atoi(stem.c_str() + 6) : -1;
}

std::vector<std::filesystem::path> StreamFiles(const std::filesystem::path& folder)
{
    std::vector<std::filesystem::path> streams;
    std::error_code ec;

    for (const auto& entry : std::filesystem::directory_iterator(folder, ec))
    {
        if (entry.is_regular_file() && Lower(entry.path().extension().u8string()) == ".p3d" &&
            StreamNumber(entry.path()) >= 0)
        {
            streams.push_back(entry.path());
        }
    }

    std::sort(streams.begin(),
        streams.end(),
        [](const auto& a, const auto& b)
        {
            return StreamNumber(a) < StreamNumber(b);
        });
    return streams;
}

std::filesystem::path FindDialogue(const std::filesystem::path& gameFolder, const std::string& scene)
{
    std::error_code ec;

    for (const auto& entry : std::filesystem::directory_iterator(DialogueFolder(gameFolder), ec))
    {
        if (SameName(entry.path().stem().u8string(), scene) && Lower(entry.path().extension().u8string()) == ".p3d")
        {
            return entry.path();
        }
    }

    return {};
}

bool CallnisLists(const std::string& text, const std::string& scene)
{
    return Lower(text).find("\"" + Lower(scene) + "\"") != std::string::npos;
}

void RenameIn(Chunk& chunk, const std::string& from, const std::string& to, int& count)
{
    Bytes payload = chunk.Payload();
    Bytes out;
    std::string lowerFrom = Lower(from);
    size_t i = 0;

    while (i < payload.size())
    {
        if (i + 4 + from.size() <= payload.size())
        {
            uint32_t length =
                payload[i] | payload[i + 1] << 8 | payload[i + 2] << 16 | static_cast<uint32_t>(payload[i + 3]) << 24;
            if (length == from.size() &&
                Lower(std::string(payload.begin() + i + 4, payload.begin() + i + 4 + length)) == lowerFrom)
            {
                uint32_t size = static_cast<uint32_t>(to.size());

                for (int k = 0; k < 4; ++k)
                {
                    out.push_back(static_cast<uint8_t>(size >> (8 * k)));
                }

                out.insert(out.end(), to.begin(), to.end());
                i += 4 + length;
                ++count;
                continue;
            }
        }

        out.push_back(payload[i++]);
    }

    chunk.SetPayload(std::move(out));

    for (size_t c = 0; c < chunk.ChildCount(); ++c)
    {
        RenameIn(*chunk.Child(c), from, to, count);
    }
}

}

std::filesystem::path NisFolder(const std::filesystem::path& gameFolder)
{
    return gameFolder / "art" / "nis";
}

std::filesystem::path DialogueFolder(const std::filesystem::path& gameFolder)
{
    return gameFolder / "art" / "sound" / "scripts" / "localised" / "english" / "nis";
}

std::filesystem::path DetectGameFolder(const std::filesystem::path& file)
{
    std::error_code ec;

    for (std::filesystem::path dir = file.parent_path(); !dir.empty(); dir = dir.parent_path())
    {
        if (std::filesystem::is_directory(NisFolder(dir), ec))
        {
            return dir;
        }

        if (dir == dir.root_path())
        {
            break;
        }
    }

    return {};
}

std::vector<std::string> ListSceneFolders(const std::filesystem::path& gameFolder)
{
    std::vector<std::string> scenes;
    std::error_code ec;

    for (const auto& entry : std::filesystem::directory_iterator(NisFolder(gameFolder), ec))
    {
        if (entry.is_directory() && std::filesystem::exists(entry.path() / "static.p3d", ec))
        {
            scenes.push_back(entry.path().filename().u8string());
        }
    }

    std::sort(scenes.begin(),
        scenes.end(),
        [](const std::string& a, const std::string& b)
        {
            return Lower(a) < Lower(b);
        });
    return scenes;
}

bool ReadPartition(const std::filesystem::path& stream, StreamPartition& partition)
{
    std::optional<Bytes> bytes = ReadFile(stream);

    if (!bytes)
    {
        return false;
    }

    ParseResult parsed = ParseP3d(*bytes);

    if (!parsed.root)
    {
        return false;
    }

    for (size_t i = 0; i < parsed.root->ChildCount(); ++i)
    {
        const Chunk* chunk = parsed.root->Child(i);

        if (chunk->Id() != chunk::NisPartition)
        {
            continue;
        }

        ByteReader reader(chunk->Payload());
        partition.index = static_cast<int>(reader.U32());
        partition.count = static_cast<int>(reader.U32());
        reader.U32();
        reader.U32();
        reader.U32();
        partition.frames = static_cast<int>(reader.U32());
        return !reader.Failed();
    }

    return false;
}

Bytes MakeLoader(const LoaderInfo& info)
{
    ByteWriter payload;
    WritePString(payload, info.scene);
    WritePString(payload, info.scene);
    WritePString(payload, "init");
    WritePString(payload, "end");
    WritePString(payload, info.dialogue.empty() ? info.scene : info.dialogue);

    for (float value : {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f})
    {
        payload.F32(value);
    }

    payload.U32(3);
    payload.U32(1);
    payload.U32(static_cast<uint32_t>(info.streams));
    WritePString(payload, info.scene + "/stream");
    payload.U32(static_cast<uint32_t>(info.frames));
    payload.U32(kFramesPerStream);

    std::string staticPath = (info.staticScene.empty() ? info.scene : info.staticScene) + "/static.p3d";
    ByteWriter files;
    WritePString(files, staticPath);
    files.U32(1);
    WritePString(files, staticPath);

    Bytes chunk = Serialize(chunk::NisLoader, payload.Take(), Serialize(chunk::NisLoaderStatic, files.Take(), {}));
    ByteWriter file;
    file.U32(kP3dFileId);
    file.U32(12);
    file.U32(static_cast<uint32_t>(12 + chunk.size()));
    file.Append(chunk);
    return file.Take();
}

bool ReadLoader(const Bytes& bytes, LoaderInfo& info)
{
    ParseResult parsed = ParseP3d(bytes);

    if (!parsed.root || parsed.root->ChildCount() != 1 || parsed.root->Child(0)->Id() != chunk::NisLoader)
    {
        return false;
    }

    const Chunk& loader = *parsed.root->Child(0);
    ByteReader reader(loader.Payload());
    info.scene = ReadPString(reader);
    ReadPString(reader);
    ReadPString(reader);
    ReadPString(reader);
    info.dialogue = ReadPString(reader);
    reader.Take(24 + 8);
    info.streams = static_cast<int>(reader.U32());
    ReadPString(reader);
    info.frames = static_cast<int>(reader.U32());

    if (reader.Failed() || loader.ChildCount() != 1)
    {
        return false;
    }

    ByteReader files(loader.Child(0)->Payload());
    std::string staticPath = ReadPString(files);
    info.staticScene = staticPath.substr(0, staticPath.find('/'));
    return !files.Failed();
}

int RenameDialogue(Bytes& bytes, const std::string& from, const std::string& to, std::string& error)
{
    ParseResult parsed = ParseP3d(bytes);

    if (!parsed.root)
    {
        error = parsed.error;
        return -1;
    }

    int count = 0;

    for (size_t i = 0; i < parsed.root->ChildCount(); ++i)
    {
        RenameIn(*parsed.root->Child(i), from, to, count);
    }

    bytes = SerializeP3d(*parsed.root);
    return count;
}

bool IsValidSceneName(const std::string& name)
{
    if (name.empty() || name.size() > 100)
    {
        return false;
    }

    for (char c : name)
    {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-')
        {
            return false;
        }
    }

    return true;
}

BuildPlan PlanBuild(const std::filesystem::path& gameFolder, const BuildOptions& options)
{
    BuildPlan plan;
    std::error_code ec;
    std::filesystem::path nis = NisFolder(gameFolder);

    if (gameFolder.empty() || !std::filesystem::is_directory(nis, ec))
    {
        plan.error = "Set the game folder (the one holding art/nis).";
        return plan;
    }

    if (!IsValidSceneName(options.scene))
    {
        plan.error = "Scene names use letters, digits, _ and - only.";
        return plan;
    }

    std::filesystem::path source = nis / std::filesystem::u8path(options.assetsFrom);

    if (options.assetsFrom.empty() || !std::filesystem::exists(source / "static.p3d", ec))
    {
        plan.error = "Pick the scene whose animations and models to use.";
        return plan;
    }

    std::vector<std::filesystem::path> streams = StreamFiles(source);

    if (streams.empty())
    {
        plan.error = options.assetsFrom + " has no stream files.";
        return plan;
    }

    plan.inPlace = SameName(options.scene, options.assetsFrom);

    if (plan.inPlace && std::filesystem::exists(gameFolder / "cement.rcf", ec))
    {
        plan.notes.push_back(
            "cement.rcf is present and the game reads it before loose files: a shipped scene rebuilt under its own "
            "name only takes effect from patch01.rcf. A new name has no such problem.");
    }

    std::filesystem::path target = nis / std::filesystem::u8path(options.scene);

    if (!plan.inPlace)
    {
        if (std::filesystem::exists(target, ec) && !options.overwrite)
        {
            plan.error = "art/nis/" + options.scene + "/ already exists. Tick Overwrite to replace it.";
            return plan;
        }

        plan.writes.push_back(target / "static.p3d");

        for (const auto& stream : streams)
        {
            plan.writes.push_back(target / stream.filename());
        }
    }

    plan.writes.push_back(nis / "fsc" / std::filesystem::u8path(options.scene + ".fsc"));
    std::filesystem::path loader = nis / std::filesystem::u8path(options.scene + ".p3d");

    if (plan.inPlace && std::filesystem::exists(loader, ec))
    {
        plan.notes.push_back("The scene's own loader is kept.");
    }
    else
    {
        plan.writes.push_back(loader);
    }

    if (options.dialogue && !plan.inPlace)
    {
        if (FindDialogue(gameFolder, options.assetsFrom).empty())
        {
            plan.notes.push_back(options.assetsFrom + " has no dialogue script; the scene plays without one.");
        }
        else
        {
            plan.writes.push_back(DialogueFolder(gameFolder) / std::filesystem::u8path(options.scene + ".p3d"));
        }
    }

    if (options.listInCallnis)
    {
        std::optional<std::string> callnis = ReadText(nis / "callnis.txt");

        if (callnis && CallnisLists(*callnis, options.scene))
        {
            plan.notes.push_back("Already listed in callnis.txt.");
        }
        else
        {
            plan.writes.push_back(nis / "callnis.txt");
        }
    }

    return plan;
}

BuildPlan
Build(const std::filesystem::path& gameFolder, const BuildOptions& options, const std::vector<fsc::Node>& roots)
{
    BuildPlan plan = PlanBuild(gameFolder, options);

    if (!plan.error.empty())
    {
        return plan;
    }

    auto fail = [&](const std::string& message)
    {
        plan.error = message;
        return plan;
    };
    std::error_code ec;
    std::filesystem::path nis = NisFolder(gameFolder);
    std::filesystem::path source = nis / std::filesystem::u8path(options.assetsFrom);
    std::filesystem::path target = nis / std::filesystem::u8path(options.scene);
    std::vector<std::filesystem::path> streams = StreamFiles(source);

    if (!plan.inPlace)
    {
        std::filesystem::create_directories(target, ec);

        for (const auto& old : StreamFiles(target))
        {
            std::filesystem::remove(old, ec);
        }

        if (!std::filesystem::copy_file(source / "static.p3d",
                target / "static.p3d",
                std::filesystem::copy_options::overwrite_existing,
                ec))
        {
            return fail("Cannot copy static.p3d: " + ec.message());
        }

        for (const auto& stream : streams)
        {
            if (!std::filesystem::copy_file(stream,
                    target / stream.filename(),
                    std::filesystem::copy_options::overwrite_existing,
                    ec))
            {
                return fail("Cannot copy " + stream.filename().u8string() + ": " + ec.message());
            }
        }
    }

    std::vector<fsc::Node> script = roots;
    RenameScene(script, options.scene);
    std::string text = fsc::Write(script);
    std::filesystem::create_directories(nis / "fsc", ec);

    if (!WriteFile(nis / "fsc" / std::filesystem::u8path(options.scene + ".fsc"), Bytes(text.begin(), text.end())))
    {
        return fail("Cannot write the .fsc.");
    }

    std::filesystem::path loader = nis / std::filesystem::u8path(options.scene + ".p3d");

    if (std::find(plan.writes.begin(), plan.writes.end(), loader) != plan.writes.end())
    {
        LoaderInfo info;
        info.scene = options.scene;
        info.streams = static_cast<int>(streams.size());
        StreamPartition partition;
        info.frames = ReadPartition(streams.front(), partition) ? partition.frames : info.streams * kFramesPerStream;

        if (!WriteFile(loader, MakeLoader(info)))
        {
            return fail("Cannot write " + loader.filename().u8string());
        }
    }

    std::filesystem::path dialogueTarget = DialogueFolder(gameFolder) / std::filesystem::u8path(options.scene + ".p3d");

    if (std::find(plan.writes.begin(), plan.writes.end(), dialogueTarget) != plan.writes.end())
    {
        std::optional<Bytes> dialogue = ReadFile(FindDialogue(gameFolder, options.assetsFrom));
        std::string error;
        int renamed = dialogue ? RenameDialogue(*dialogue, options.assetsFrom, options.scene, error) : -1;

        if (renamed < 0)
        {
            return fail("Cannot read the dialogue script of " + options.assetsFrom + ". " + error);
        }

        if (renamed == 0)
        {
            plan.notes.push_back("The dialogue script does not name " + options.assetsFrom + "; copied unchanged.");
        }

        if (!WriteFile(dialogueTarget, *dialogue))
        {
            return fail("Cannot write the dialogue script.");
        }
    }

    std::filesystem::path callnisPath = nis / "callnis.txt";

    if (std::find(plan.writes.begin(), plan.writes.end(), callnisPath) != plan.writes.end())
    {
        std::string callnis = ReadText(callnisPath).value_or("");
        std::filesystem::path backup = nis / "callnis.txt.orig";

        if (!callnis.empty() && !std::filesystem::exists(backup, ec))
        {
            std::filesystem::copy_file(callnisPath, backup, ec);
        }

        if (!callnis.empty() && callnis.back() != '\n')
        {
            callnis += "\r\n";
        }

        callnis += "\"" + options.scene + "\"\r\n";

        if (!WriteFile(callnisPath, Bytes(callnis.begin(), callnis.end())))
        {
            return fail("Cannot update callnis.txt.");
        }
    }

    return plan;
}

}
