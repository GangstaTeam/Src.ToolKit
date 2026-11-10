#include "render/ModelResolver.hh"

#include "core/ByteReader.hh"
#include "p3d/ChunkIds.hh"

#include <cctype>
#include <cstring>
#include <set>
#include <string>

namespace tk
{

namespace
{

std::string ReadName(ByteReader& reader)
{
    Bytes chars = reader.Take(reader.U8());
    std::string name(chars.begin(), chars.end());
    name.resize(std::strlen(name.c_str()));
    return name;
}

std::string LeadingName(const Chunk& chunk)
{
    ByteReader reader(chunk.Payload());
    return ReadName(reader);
}

std::string SkinSkeletonName(const Chunk& skin)
{
    ByteReader reader(skin.Payload());
    ReadName(reader);
    reader.U32();
    return ReadName(reader);
}

bool IsModel(const Chunk& chunk)
{
    return chunk.Id() == chunk::Mesh || chunk.Id() == chunk::Skin;
}

Chunk* FindTopLevel(Chunk& root, uint32_t id, const std::string& name)
{
    for (size_t i = 0; i < root.ChildCount(); ++i)
    {
        Chunk* child = root.Child(i);

        if (child->Id() == id && LeadingName(*child) == name)
        {
            return child;
        }
    }

    return nullptr;
}

Chunk* SkeletonForSkin(Chunk& root, const Chunk& skin)
{
    return skin.Id() == chunk::Skin ? FindTopLevel(root, chunk::Skeleton, SkinSkeletonName(skin)) : nullptr;
}

void CollectWords(const Chunk& chunk, std::set<std::string>& words)
{
    const Bytes& payload = chunk.Payload();
    std::string word;

    for (uint8_t byte : payload)
    {
        if (std::isprint(byte))
        {
            word.push_back(static_cast<char>(byte));
            continue;
        }

        if (word.size() >= 3)
        {
            words.insert(word);
        }

        word.clear();
    }

    if (word.size() >= 3)
    {
        words.insert(word);
    }

    for (size_t i = 0; i < chunk.ChildCount(); ++i)
    {
        CollectWords(*chunk.Child(i), words);
    }
}

ModelSelection FromModels(Chunk& root, std::vector<const Chunk*> meshes)
{
    ModelSelection selection;
    selection.meshes = std::move(meshes);

    for (const Chunk* mesh : selection.meshes)
    {
        if (Chunk* skeleton = SkeletonForSkin(root, *mesh))
        {
            selection.skeleton = skeleton;
            break;
        }
    }

    return selection;
}

ModelSelection ByReferencedNames(Chunk& root, const Chunk& referrer, bool firstOnly)
{
    std::set<std::string> words;
    CollectWords(referrer, words);
    std::vector<const Chunk*> meshes;

    for (size_t i = 0; i < root.ChildCount() && !(firstOnly && !meshes.empty()); ++i)
    {
        const Chunk* child = root.Child(i);

        if (IsModel(*child) && words.count(LeadingName(*child)))
        {
            meshes.push_back(child);
        }
    }

    return FromModels(root, meshes);
}

ModelSelection FromSkeleton(Chunk& root, Chunk& skeleton)
{
    std::string name = LeadingName(skeleton);
    ModelSelection selection;
    selection.skeleton = &skeleton;

    for (size_t i = 0; i < root.ChildCount(); ++i)
    {
        const Chunk* child = root.Child(i);

        if (child->Id() == chunk::Skin && SkinSkeletonName(*child) == name)
        {
            selection.meshes.push_back(child);
            break;
        }
    }

    return selection;
}

}

ModelSelection ResolveModel(Chunk& root, const Chunk* selection)
{
    for (const Chunk* chunk = selection; chunk && chunk != &root; chunk = chunk->Parent())
    {
        if (IsModel(*chunk))
        {
            return FromModels(root, {chunk});
        }

        if (chunk->Id() == chunk::Skeleton)
        {
            return FromSkeleton(root, *chunk->Parent()->Child(chunk->IndexInParent()));
        }

        if (chunk->Id() == chunk::CharacterTemplate)
        {
            return ByReferencedNames(root, *chunk, true);
        }

        if (chunk->Id() == chunk::CompositeDrawable)
        {
            return ByReferencedNames(root, *chunk, false);
        }
    }

    return {};
}

ModelSelection ResolveWholeFile(Chunk& root)
{
    ModelSelection selection = FromModels(root, {&root});

    for (size_t i = 0; i < root.ChildCount() && !selection.skeleton; ++i)
    {
        selection.skeleton = SkeletonForSkin(root, *root.Child(i));
    }

    return selection;
}

}
