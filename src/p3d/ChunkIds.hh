#pragma once

#include <cstdint>

namespace tk::chunk
{

enum : uint32_t
{
    Mesh = 0x10000,
    Skin = 0x10001,
    Box = 0x10003,
    Sphere = 0x10004,
    Positions = 0x10005,
    Normals = 0x10006,
    Uvs = 0x10007,
    Colours = 0x10008,
    Strips = 0x10009,
    Indices = 0x1000A,
    MatrixList = 0x1000B,
    WeightList = 0x1000C,
    MatrixPalette = 0x1000D,
    PackedNormals = 0x10010,
    PrimGroup = 0x10020,

    Shader = 0x11000,
    ShaderTextureParam = 0x11002,

    Project = 0x18000,
    Screen = 0x18001,
    Page = 0x18002,
    Layer = 0x18003,
    Group = 0x18004,
    MultiSprite = 0x18006,
    MultiText = 0x18007,
    Pure3dObject = 0x18008,
    Polygon = 0x18009,
    StringTextBible = 0x1800B,
    StringHardCoded = 0x1800C,
    TextBible = 0x1800D,
    Language = 0x1800E,
    Layer2 = 0x18020,
    Group2 = 0x18021,
    Sprite2 = 0x18022,
    Text2 = 0x18023,
    Pure3dObject2 = 0x18024,
    RoundedRectangle = 0x18025,

    Texture = 0x19000,
    Image = 0x19001,
    ImageData = 0x19002,
    Sprite = 0x19005,

    TextureFont = 0x22000,
    FontGlyphs = 0x22001,
    FontPictureOffsets = 0x22006,
    FontKerning = 0x22007,

    Skeleton = 0x23000,
    SkeletonJoint = 0x23001,

    Animation = 0x121000,
    CompositeDrawable = 0x123000,

    CharacterTemplate = 0x8800000,
    NisLoader = 0x8800102,
    NisLoaderStatic = 0x8800103,
    NisPartition = 0x8800114,
    LoadObject = 0x8800150,
};

}
