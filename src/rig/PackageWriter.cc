#include "rig/PackageWriter.hh"

#include "core/ByteReader.hh"
#include "p3d/ChunkIds.hh"
#include "render/ImageEncoder.hh"
#include "rig/ChunkText.hh"
#include "rig/SkinChunks.hh"

#include <set>

namespace tk
{

namespace
{

std::string SkinSkeletonName(const Chunk& skin)
{
    ByteReader reader(skin.Payload());
    ReadName(reader);
    reader.U32();
    return ReadName(reader);
}

std::string PrimGroupShader(const Chunk& group)
{
    ByteReader reader(group.Payload());
    reader.U32();
    return ReadName(reader);
}

std::set<std::string> ShaderTextures(const Chunk& root, const std::set<std::string>& shaders)
{
    std::set<std::string> textures;

    for (size_t i = 0; i < root.ChildCount(); ++i)
    {
        const Chunk& shader = *root.Child(i);

        if (shader.Id() != chunk::Shader || !shaders.count(LeadingName(shader)))
        {
            continue;
        }

        for (size_t p = 0; p < shader.ChildCount(); ++p)
        {
            if (shader.Child(p)->Id() != chunk::ShaderTextureParam)
            {
                continue;
            }

            ByteReader reader(shader.Child(p)->Payload());

            if (reader.U32() == kTexParamName)
            {
                textures.insert(ReadName(reader));
            }
        }
    }

    return textures;
}

}

PackageTarget FindPackageTarget(Chunk& root)
{
    PackageTarget target;
    std::set<std::string> shaders;

    for (size_t i = 0; i < root.ChildCount(); ++i)
    {
        Chunk& chunk = *root.Child(i);

        if (chunk.Id() != chunk::Skin)
        {
            continue;
        }

        target.skins.push_back(&chunk);

        if (target.skinName.empty())
        {
            target.skinName = LeadingName(chunk);
            target.skeletonName = SkinSkeletonName(chunk);
        }

        for (size_t g = 0; g < chunk.ChildCount(); ++g)
        {
            if (chunk.Child(g)->Id() == chunk::PrimGroup)
            {
                shaders.insert(PrimGroupShader(*chunk.Child(g)));
            }
        }
    }

    std::set<std::string> textureNames = ShaderTextures(root, shaders);

    for (size_t i = 0; i < root.ChildCount(); ++i)
    {
        Chunk& chunk = *root.Child(i);

        if (chunk.Id() == chunk::Skeleton && !target.skeleton && LeadingName(chunk) == target.skeletonName)
        {
            target.skeleton = &chunk;
        }

        if (chunk.Id() == chunk::Texture && textureNames.count(LeadingName(chunk)))
        {
            target.textures.push_back(&chunk);
        }
    }

    return target;
}

bool ApplyRigToPackage(Chunk& root,
    const RiggedCharacter& rig,
    const Image& texture,
    uint32_t textureSize,
    std::string& report)
{
    PackageTarget target = FindPackageTarget(root);

    if (!target.Valid())
    {
        report = "The open file has no character skin with a skeleton. Open a package such as "
                 "packages/characters/pc/MCP_*.p3d.";
        return false;
    }

    std::optional<Bytes> png = texture.bgra.empty() ? std::nullopt : EncodePngRgb(texture, textureSize);
    RiggedCharacter bound = Rebind(rig, ReadGameSkeleton(*target.skeleton));
    RewriteSkeleton(*target.skeleton, bound.skeleton);

    for (Chunk* skin : target.skins)
    {
        RebuildPolySkin(*skin, bound);
    }

    for (Chunk* textureChunk : target.textures)
    {
        if (png)
        {
            RewriteTexture(*textureChunk, *png, textureSize);
        }
    }

    report = "Replaced skin " + target.skinName + " (" + std::to_string(target.skins.size()) + " LOD), skeleton " +
        target.skeletonName + " and " + std::to_string(png ? target.textures.size() : 0) +
        " texture(s): " + std::to_string(rig.positions.size()) + " vertices, " +
        std::to_string(rig.indices.size() / 3) + " triangles.";
    return true;
}

}
