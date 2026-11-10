#include "core/FileIo.hh"
#include "p3d/ChunkIo.hh"
#include "p3d/PayloadCodec.hh"
#include "p3d/SchemaParser.hh"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <map>
#include <string>

using namespace tk;

namespace
{

struct IdStats
{
    size_t total = 0;
    size_t decoded = 0;
    size_t mismatched = 0;
    size_t reencodeDiffers = 0;
    std::string example;
    std::string error;
};

struct Report
{
    std::map<uint32_t, IdStats> ids;
    size_t files = 0;
    size_t parseFailures = 0;
    size_t roundTripFailures = 0;
};

void CheckChunk(const Schema& schema, const Chunk& chunk, const std::string& file, Report& report)
{
    IdStats& stats = report.ids[chunk.Id()];
    ++stats.total;

    if (const ChunkDef* def = schema.FindChunk(chunk.Id()))
    {
        DecodedPayload decoded = DecodePayload(schema, *def, chunk.Payload());

        if (decoded.ok && decoded.leftover.empty())
        {
            ++stats.decoded;

            if (EncodePayload(decoded) != chunk.Payload())
            {
                ++stats.reencodeDiffers;
            }
        }
        else
        {
            ++stats.mismatched;

            if (stats.example.empty())
            {
                stats.example = file;
                stats.error = decoded.error;
            }
        }
    }
    else if (stats.example.empty())
    {
        stats.example = file;
    }

    for (size_t i = 0; i < chunk.ChildCount(); ++i)
    {
        CheckChunk(schema, *chunk.Child(i), file, report);
    }
}

void CheckFile(const Schema& schema, const std::filesystem::path& path, Report& report)
{
    auto bytes = ReadFile(path);

    if (!bytes)
    {
        return;
    }

    ++report.files;
    ParseResult parsed = ParseP3d(*bytes);

    if (!parsed.root)
    {
        ++report.parseFailures;
        return;
    }

    if (SerializeP3d(*parsed.root) != *bytes)
    {
        ++report.roundTripFailures;
        std::printf("ROUNDTRIP MISMATCH %s\n", path.string().c_str());
    }

    for (size_t i = 0; i < parsed.root->ChildCount(); ++i)
    {
        CheckChunk(schema, *parsed.root->Child(i), path.string(), report);
    }
}

void Print(const Schema& schema, const Report& report)
{
    std::printf("files %zu, unparsed %zu, roundtrip failures %zu\n",
        report.files,
        report.parseFailures,
        report.roundTripFailures);
    std::printf("%-10s %-34s %8s %8s %8s %8s  example / error\n", "id", "name", "total", "ok", "bad", "reenc");

    for (const auto& [id, stats] : report.ids)
    {
        bool known = schema.FindChunk(id) != nullptr;

        if (known && stats.mismatched == 0 && stats.reencodeDiffers == 0)
        {
            continue;
        }

        std::printf("0x%-8X %-34s %8zu %8zu %8zu %8zu  %s %s\n",
            id,
            known ? schema.ChunkName(id).c_str() : "(no schema)",
            stats.total,
            stats.decoded,
            stats.mismatched,
            stats.reencodeDiffers,
            std::filesystem::path(stats.example).filename().string().c_str(),
            stats.error.c_str());
    }
}

}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::printf("usage: p3dcheck <schema dir> <file or directory>\n");
        return 1;
    }

    Schema schema;
    std::string error;

    if (!LoadSchemaDirectory(argv[1], schema, error))
    {
        std::printf("schema error: %s\n", error.c_str());
        return 1;
    }

    std::printf("schema: %zu chunk types\n", schema.Chunks().size());
    Report report;
    std::filesystem::path target = argv[2];
    bool verbose = argc > 3 && std::string(argv[3]) == "-v";
    try
    {
        if (std::filesystem::is_directory(target))
        {
            auto options = std::filesystem::directory_options::skip_permission_denied;

            for (const auto& entry : std::filesystem::recursive_directory_iterator(target, options))
            {
                if (entry.path().extension() != ".p3d")
                {
                    continue;
                }

                if (verbose)
                {
                    std::printf("%s\n", entry.path().string().c_str());
                }

                CheckFile(schema, entry.path(), report);
            }
        }
        else
        {
            CheckFile(schema, target, report);
        }
    }

    catch (const std::exception& e)
    {
        std::printf("error: %s\n", e.what());
    }

    Print(schema, report);
    return 0;
}
