#include "core/FileIo.hh"
#include "p3d/ChunkIo.hh"
#include "rig/FbxConvert.hh"
#include "rig/ImportSession.hh"
#include "rig/PackageWriter.hh"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

using namespace tk;

namespace
{

struct Options
{
    std::filesystem::path model, package, output, texture;
    bool markers = false;
    bool keepPose = false;
    size_t maxVertices = 0;
    float height = 1.8f;
    int turns[3] = {0, 0, 0};
};

bool ParseOptions(int argc, char** argv, Options& options)
{
    if (argc < 4)
    {
        return false;
    }

    options.model = std::filesystem::u8path(argv[1]);
    options.package = std::filesystem::u8path(argv[2]);
    options.output = std::filesystem::u8path(argv[3]);

    for (int i = 4; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--markers")
        {
            options.markers = true;
        }
        else if (arg == "--keep-pose")
        {
            options.keepPose = true;
        }
        else if (arg == "--max-vertices" && i + 1 < argc)
        {
            options.maxVertices = static_cast<size_t>(std::atoll(argv[++i]));
        }
        else if (arg == "--height" && i + 1 < argc)
        {
            options.height = static_cast<float>(std::atof(argv[++i]));
        }
        else if (arg == "--texture" && i + 1 < argc)
        {
            options.texture = std::filesystem::u8path(argv[++i]);
        }
        else if (arg == "--rotate" && i + 3 < argc)
        {
            for (int& turn : options.turns)
            {
                turn = std::atoi(argv[++i]);
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}

std::filesystem::path ConvertedModel(const std::filesystem::path& model)
{
    if (model.extension() != ".fbx" && model.extension() != ".FBX")
    {
        return model;
    }

    std::filesystem::path glb = std::filesystem::temp_directory_path() / L"rigcheck_import.glb";
    std::string error;

    if (!ConvertFbx(model, glb, error))
    {
        std::printf("fbx: %s\n", error.c_str());
    }

    return glb;
}

void PrintMarkers(const ImportSession& session)
{
    const MarkerSet& markers = session.Request().markers;

    for (size_t i = 0; i < kMarkerCount; ++i)
    {
        std::printf("  %-8s %6.3f %6.3f\n", MarkerLabel(static_cast<Marker>(i)), markers[i].x, markers[i].y);
    }
}

void TruncateVertices(RiggedCharacter& rig, size_t limit)
{
    if (rig.positions.size() <= limit)
    {
        return;
    }

    std::vector<uint32_t> indices;

    for (size_t t = 0; t + 2 < rig.indices.size(); t += 3)
    {
        if (rig.indices[t] < limit && rig.indices[t + 1] < limit && rig.indices[t + 2] < limit)
        {
            indices.insert(indices.end(), rig.indices.begin() + t, rig.indices.begin() + t + 3);
        }
    }

    rig.indices = std::move(indices);
    rig.positions.resize(limit);
    rig.normals.resize(limit);
    rig.uvs.resize(std::min(rig.uvs.size(), limit * 2));
    rig.binding.joints.resize(limit);
    rig.binding.weights.resize(limit);
}

void PrintJoints(const RiggedCharacter& rig)
{
    std::vector<Mat4> worlds = WorldMatrices(rig.skeleton);

    for (size_t i = 0; i < rig.skeleton.size(); ++i)
    {
        Vec3 p = worlds[i].Translation();
        std::printf("  %-18s %7.3f %7.3f %7.3f\n", rig.skeleton[i].name.c_str(), p.x, p.y, p.z);
    }
}

}

int main(int argc, char** argv)
{
    Options options;

    if (!ParseOptions(argc, argv, options))
    {
        std::printf("usage: rigcheck <model.glb|fbx> <package.p3d> <out.p3d> [--markers] [--keep-pose] [--max-vertices "
                    "n] [--height 1.8] [--rotate x y z] [--texture file]\n");
        return 1;
    }

    ImportSession session;
    std::string error;

    if (!session.Load(ConvertedModel(options.model), error))
    {
        std::printf("load: %s\n", error.c_str());
        return 1;
    }

    if (!options.texture.empty() && !session.LoadTexture(options.texture, error))
    {
        std::printf("texture: %s\n", error.c_str());
    }

    for (int axis = 0; axis < 3; ++axis)
    {
        if (options.turns[axis])
        {
            session.Rotate(axis, options.turns[axis]);
        }
    }

    session.SetHeight(options.height);

    if (options.markers)
    {
        session.Request().source = RigSource::Markers;
    }

    if (options.keepPose)
    {
        session.Request().straightenArms = false;
    }

    std::printf("model: %zu vertices, %zu triangles, %zu bones, texture %ux%u\n",
        session.Model().positions.size(),
        session.Model().indices.size() / 3,
        session.Model().bones.size(),
        session.Texture().width,
        session.Texture().height);
    PrintMarkers(session);
    auto bytes = ReadFile(options.package);
    ParseResult parsed = bytes ? ParseP3d(*bytes) : ParseResult{};

    if (!parsed.root)
    {
        std::printf("package: cannot read %s\n", options.package.u8string().c_str());
        return 1;
    }

    PackageTarget target = FindPackageTarget(*parsed.root);
    GameSkeleton base = target.skeleton ? ReadGameSkeleton(*target.skeleton) : DefaultGameSkeleton();

    if (!session.Build(base, error))
    {
        std::printf("rig: %s\n", error.c_str());
        return 1;
    }

    if (options.maxVertices)
    {
        TruncateVertices(*session.MutableRig(), options.maxVertices);
    }

    PrintJoints(*session.Rig());
    std::string report;

    if (!ApplyRigToPackage(*parsed.root, *session.Rig(), session.Texture(), 512, report))
    {
        std::printf("apply: %s\n", report.c_str());
        return 1;
    }

    WriteFile(options.output, SerializeP3d(*parsed.root));
    std::printf("%s\nwrote %s\n", report.c_str(), options.output.u8string().c_str());
    return 0;
}
