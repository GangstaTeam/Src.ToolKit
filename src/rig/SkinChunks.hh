#pragma once

#include "p3d/Chunk.hh"
#include "rig/RigBuilder.hh"

#include <cstdint>

namespace tk
{

enum ESkinChunksValue : uint32_t
{
    kTextureVersion = 14000,
};

void RebuildPolySkin(Chunk& skin, const RiggedCharacter& rig);
void RewriteSkeleton(Chunk& skeleton, const GameSkeleton& fitted);
void RewriteTexture(Chunk& texture, const Bytes& png, uint32_t size);

}
