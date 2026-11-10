#pragma once

#include "p3d/Chunk.hh"
#include "render/ImageDecoder.hh"
#include "rig/RigBuilder.hh"

#include <cstdint>
#include <string>
#include <vector>

namespace tk
{

enum EPackageWriterValue : uint32_t
{
    kTexParamName = 0x00584554u,
};

struct PackageTarget
{
    std::vector<Chunk*> skins;
    std::vector<Chunk*> textures;
    Chunk* skeleton = nullptr;
    std::string skinName;
    std::string skeletonName;

    bool Valid() const { return !skins.empty() && skeleton; }
};

PackageTarget FindPackageTarget(Chunk& root);
bool ApplyRigToPackage(Chunk& root,
    const RiggedCharacter& rig,
    const Image& texture,
    uint32_t textureSize,
    std::string& report);

}
