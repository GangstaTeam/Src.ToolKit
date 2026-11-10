// overrides radical.sc by id

chunk PrimGroup : chunk_id(0x10020) subchunk(0x10011) subchunk(0x10005) subchunk(0x10006) subchunk(0x10010) subchunk(0x10007) subchunk(0x10008) subchunk(0x1001C) subchunk(0x10009) subchunk(0x1000A) subchunk(0x1000B) subchunk(0x1000C) subchunk(0x1000D) subchunk(0x10003) subchunk(0x10004)
{
    ULONG Version;
    string Shader;
    ULONG PrimitiveType;
    ULONG VertexType;
    ULONG NumVertices;
    ULONG NumIndices;
    ULONG NumMatrices;
    ULONG Unknown0;
    ULONG Optimized;
    ULONG ShaderMode;
    ULONG Unknown1;
}

chunk BoundingBox : chunk_id(0x10003)
{
    tlBox Box;
}

chunk BoundingSphere : chunk_id(0x10004)
{
    tlSphere Sphere;
}

chunk Skeleton : chunk_id(0x23000) subchunk(0x23001)
{
    string Name;
    ULONG Version;
    ULONG NumJoints;
    ULONG NumPartitions;
    ULONG NumLimbs;
}

chunk SkeletonJoint : chunk_id(0x23001)
{
    string Name;
    ULONG Parent;
    tlMatrix RestPose;
}

chunk VertexCompressionHint : chunk_id(0x10021)
{
    ULONG Hint0;
    ULONG Hint1;
    ULONG Hint2;
    ULONG Hint3;
    ULONG Hint4;
    ULONG Hint5;
    ULONG Hint6;
    ULONG Hint7;
}

chunk SortKey : chunk_id(0x122000)
{
    ULONG Key;
    float Value;
}

chunk RenderStatus : chunk_id(0x10017)
{
    ULONG CastShadow;
}

chunk MeshStats : chunk_id(0x1001D)
{
    raw Data;
}

chunk CharacterLoader : chunk_id(0x8800000)
{
    raw Data;
}

chunk CharacterTemplate : chunk_id(0x8800180)
{
    string Name;
    string Type;
}

chunk CharacterTemplateProperty : chunk_id(0x8800182)
{
    raw Data;
}

chunk SoundBank : chunk_id(0x8800108)
{
    raw Data;
}

chunk ScriptData : chunk_id(0x1000001)
{
    raw Data;
}

chunk ExportInfo : chunk_id(0x7000)
{
    raw Data;
}

chunk Locator : chunk_id(0x14000)
{
    string Name;
    raw Data;
}

chunk ColourChannel : chunk_id(0x121109)
{
    raw Data;
}

chunk CVLoadObject : chunk_id(0x8800150)
{
    string Name;
    UWORD SpawnType;
    string SoundPoi;
    ULONG StreamFlags;
    string Package;
}

// scrooby 0x18020-0x18025, loader 0x677950
// versions: sprite 2, text 18, pure3dobject 3
// 640x448, origin bottom-left

chunk tlScroobyPageChunk : chunk_id(0x18002) subchunk(0x18020) subchunk(0x18003)
{
    string Name;
    ULONG Version;
    ULONG ResolutionX;
    ULONG ResolutionY;
}

chunk ScroobyLayer : chunk_id(0x18020) subchunk(0x18021) subchunk(0x18022) subchunk(0x18023) subchunk(0x18024) subchunk(0x18009) subchunk(0x18025)
{
    string Name;
    ULONG Version;
    ULONG Visible;
    ULONG Editable;
    ULONG Alpha;
}

chunk ScroobyGroup : chunk_id(0x18021) subchunk(0x18021) subchunk(0x18022) subchunk(0x18023) subchunk(0x18024) subchunk(0x18009) subchunk(0x18025)
{
    string Name;
    ULONG Version;
    ULONG Alpha;
}

// flags: 1 stretch, 2 flip x, 4 flip y
// justification: 0 left, 1 right, 2 top, 3 bottom, 4 centre
chunk ScroobySprite : chunk_id(0x18022)
{
    string Name;
    ULONG Version;
    LONG PositionX;
    LONG PositionY;
    LONG DimensionX;
    LONG DimensionY;
    UBYTE Flags;
    ULONG JustificationX;
    ULONG JustificationY;
    COLOUR Color;
    ULONG Translucency;
    float RotationValue;
    ULONG NumberOfImages;
    array(string, NumberOfImages) ImageNames;
}

// textflags 1 = wrap, shadow unused
chunk ScroobyText : chunk_id(0x18023) subchunk(0x1800C) subchunk(0x1800B)
{
    string Name;
    ULONG Version;
    LONG PositionX;
    LONG PositionY;
    LONG DimensionX;
    LONG DimensionY;
    UBYTE ScaleToBox;
    ULONG JustificationX;
    ULONG JustificationY;
    COLOUR Color;
    ULONG Translucency;
    float RotationValue;
    string TextStyleName;
    UBYTE TextFlags;
    COLOUR ShadowColour;
    LONG ShadowOffsetX;
    LONG ShadowOffsetY;
    ULONG CurrentText;
}

chunk ScroobyPure3dObject : chunk_id(0x18024)
{
    string Name;
    ULONG Version;
    LONG PositionX;
    LONG PositionY;
    LONG DimensionX;
    LONG DimensionY;
    UBYTE ScaleToFit;
    ULONG JustificationX;
    ULONG JustificationY;
    COLOUR Color;
    ULONG Translucency;
    float RotationValue;
    string Drawable;
    string Camera;
    string Animation;
    string Lights;
}

// 9-slice of roundrectanglecorner.tga, squarecorners 1/2/4/8
chunk ScroobyRoundedRectangle : chunk_id(0x18025)
{
    string Name;
    LONG PositionX;
    LONG PositionY;
    LONG DimensionX;
    LONG DimensionY;
    ULONG SquareCorners;
    COLOUR Color;
    ULONG Unknown;
    ULONG CornerRadius;
}
