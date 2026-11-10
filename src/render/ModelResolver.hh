#pragma once

#include "p3d/Chunk.hh"

#include <vector>

namespace tk
{

struct ModelSelection
{
    std::vector<const Chunk*> meshes;
    Chunk* skeleton = nullptr;

    bool operator==(const ModelSelection& other) const { return meshes == other.meshes && skeleton == other.skeleton; }
};

ModelSelection ResolveModel(Chunk& root, const Chunk* selection);
ModelSelection ResolveWholeFile(Chunk& root);

}
