#include "doc/ChunkPath.hh"

#include <algorithm>

namespace tk
{

ChunkPath PathOf(const Chunk* chunk)
{
    ChunkPath path;

    for (const Chunk* node = chunk; node && node->Parent(); node = node->Parent())
    {
        path.push_back(node->IndexInParent());
    }

    std::reverse(path.begin(), path.end());
    return path;
}

Chunk* Resolve(Chunk* root, const ChunkPath& path)
{
    Chunk* node = root;

    for (size_t index : path)
    {
        if (!node || index >= node->ChildCount())
        {
            return nullptr;
        }

        node = node->Child(index);
    }

    return node;
}

}
