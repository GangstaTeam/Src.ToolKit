chunk tlAnimCollDSGChunk : chunk_id(0x3F00008) subchunk(0x4512) subchunk(0x121200) subchunk(0x48A0) subchunk(0x20001) subchunk(0x17002)
{
    string Name;
    ULONG Version;
    ULONG HasAlpha;
}

chunk tlAnimDSGChunk : chunk_id(0x3F0000C) subchunk(0x4512) subchunk(0x121200) subchunk(0x48A0) subchunk(0x17002)
{
    string Name;
    ULONG Version;
    ULONG HasAlpha;
}

chunk tlAnimDSGWrapperChunk : chunk_id(0x3F0000F) subchunk(0x20002) subchunk(0x4512) subchunk(0x121200) subchunk(0x48A0) subchunk(0x20001) subchunk(0x17002) subchunk(0x10000) subchunk(0x20001) subchunk(0x3000600) subchunk(0x4500)
{
    string Name;
    ULONG Version;
    ULONG HasAlpha;
}

chunk tlAnimObjDSGWrapperChunk : chunk_id(0x3F00010) subchunk(0x8020000) subchunk(0x20002) subchunk(0x4512) subchunk(0x121200) subchunk(0x48A0) subchunk(0x20001) subchunk(0x17002) subchunk(0x10000) subchunk(0x20001) subchunk(0x3000600) subchunk(0x4500) subchunk(0x20000) subchunk(0x20001)
{
    string Name;
    UBYTE Version;
    UBYTE HasAlpha;
}

chunk tlAnimatedObjectFactoryChunk : chunk_id(0x20000) subchunk(0x20002)
{
    ULONG Version;
    string Name;
    string BaseObjectName;
    ULONG NumAnimations;
}

chunk tlAnimatedObjectChunk : chunk_id(0x20001)
{
    ULONG Version;
    string Name;
    string FactoryName;
    ULONG StartingAnimation;
}

chunk tlAnimatedObjectAnimationChunk : chunk_id(0x20002) subchunk(0x121200)
{
    ULONG Version;
    string Name;
    float FrameRate;
    ULONG NumControllers;
}

chunk tlAnimationChunk : chunk_id(0x20002) subchunk(0x121004) subchunk(0x121002)
{
    ULONG Version;
    string Name;
    FOURCC AnimationType;
    float NumFrames;
    float FrameRate;
    ULONG Cyclic;
}

chunk tlAnimationSizeChunk : chunk_id(0x121004)
{
    ULONG Version;
    ULONG PC;
    ULONG PS2;
    ULONG XBOX;
    ULONG GC;
}

chunk tlAnimationGroupChunk : chunk_id(0x18004) subchunk(0x12110E) subchunk(0x121100) subchunk(0x121101) subchunk(0x121102) subchunk(0x121103) subchunk(0x121104) subchunk(0x121105) subchunk(0x121111) subchunk(0x121106) subchunk(0x121107) subchunk(0x121108) subchunk(0x121109) subchunk(0x12110A)
{
    ULONG Version;
    string Name;
    ULONG GroupId;
    ULONG NumChannels;
}

chunk tlAnimationGroupListChunk : chunk_id(0x121002) subchunk(0x18004)
{
    ULONG Version;
    ULONG NumGroups;
}

struct tlAttributeRow
{
    string Sound;
    string Particle;
    string Animation;
    float Friction;
    float Mass;
    float Elasticity;
}

chunk tlAttributeTableChunk : chunk_id(0x3000602)
{
    ULONG NumRows;
    array(tlAttributeRow, NumRows) Table;
}

chunk tlBBoxChunk : chunk_id(0x10003)
{
    tlBox Box;
}

chunk tlBSphereChunk : chunk_id(0x10004)
{
    tlSphere Sphere;
}

chunk tlBillboardQuadV14Chunk : chunk_id(0x17000)
{
    string Name;
    ULONG Version;
    string Shader;
    COLOUR Colour;
    float Width;
    float Height;
    ULONG ZTest;
    ULONG ZWrite;
    ULONG Fog;
    ULONG BillboardMode;
}

chunk tlBillboardDisplayInfoChunk : chunk_id(0x17003)
{
    ULONG Version;
    tlQuat Rotation;
    FOURCC CutOffMode;
    tlUV UVOffsetRange;
    float SourceRange;
    float EdgeRange;
}

chunk tlBillboardPerspectiveInfoChunk : chunk_id(0x17004)
{
    ULONG Version;
    ULONG Perspective;
}

chunk tlBillboardQuadChunk : chunk_id(0x17001) subchunk(0x17003) subchunk(0x17004)
{
    ULONG Version;
    string Name;
    FOURCC BillboardMode;
    tlPoint Translation;
    COLOUR Colour;
    tlUV Uv0;
    tlUV Uv1;
    tlUV Uv2;
    tlUV Uv3;
    float Width;
    float Height;
    float Distance;
    tlUV UVOffset;
}

chunk tlBillboardQuadGroupChunk : chunk_id(0x17002) subchunk(0x17001)
{
    ULONG Version;
    string Name;
    string Shader;
    ULONG ZTest;
    ULONG ZWrite;
    ULONG Fog;
    ULONG NumQuads;
}

chunk tlBreakableObjectChunk : chunk_id(0x3001000) subchunk(0x20000) subchunk(0x20001) subchunk(0x121200) subchunk(0x10000) subchunk(0x20002) subchunk(0x48A0) subchunk(0x4500) subchunk(0x15800) subchunk(0x15801) subchunk(0x4512)
{
    ULONG BreakableType;
    ULONG MaxInstances;
}

chunk tlCameraAnimChunk16 : chunk_id(0x4900) subchunk(0x4901)
{
    string AnimName;
    ULONG Version;
    string CameraName;
    ULONG NumFrames;
    float FrameRate;
    ULONG Cyclic;
}

chunk tlCameraAnimChannelChunk16 : chunk_id(0x4901) subchunk(0x4902) subchunk(0x4903) subchunk(0x4904) subchunk(0x4905) subchunk(0x4906) subchunk(0x4907)
{
}

chunk tlCameraAnimPosChannelChunk16 : chunk_id(0x4902) subchunk(0x4801)
{
}

chunk tlCameraAnimLookChannelChunk16 : chunk_id(0x4903) subchunk(0x4801)
{
}

chunk tlCameraAnimUpChannelChunk16 : chunk_id(0x4904) subchunk(0x4801)
{
}

chunk tlCameraAnimFOVChannelChunk16 : chunk_id(0x4905) subchunk(0x4800)
{
}

chunk tlCameraAnimNearClipChannelChunk16 : chunk_id(0x4906) subchunk(0x4800)
{
}

chunk tlCameraAnimFarClipChannelChunk16 : chunk_id(0x4907) subchunk(0x4800)
{
}

chunk tlCameraChunk16 : chunk_id(0x2200)
{
    string Name;
    ULONG Version;
    float FOV;
    float AspectRatio;
    float NearClip;
    float FarClip;
    float PositionX;
    float PositionY;
    float PositionZ;
    float LookX;
    float LookY;
    float LookZ;
    float UpX;
    float UpY;
    float UpZ;
}

chunk tlChannelInterpolationModeChunk : chunk_id(0x121110)
{
    ULONG Version;
    ULONG Interpolate;
}

chunk tlIntChannelChunk : chunk_id(0x12110E) subchunk(0x121110)
{
    ULONG Version;
    FOURCC Param;
    ULONG NumFrames;
    array(UWORD, NumFrames) Frames;
    array(ULONG, NumFrames) Values;
}

chunk tlFloat1ChannelChunk : chunk_id(0x121100) subchunk(0x121110)
{
    ULONG Version;
    FOURCC Param;
    ULONG NumFrames;
    array(UWORD, NumFrames) Frames;
    array(float, NumFrames) Values;
}

chunk tlFloat2ChannelChunk : chunk_id(0x121101) subchunk(0x121110)
{
    ULONG Version;
    FOURCC Param;
    ULONG NumFrames;
    array(UWORD, NumFrames) Frames;
    array(tlPoint2D, NumFrames) Values;
}

chunk tlVector1DOFChannelChunk : chunk_id(0x121102) subchunk(0x121110)
{
    ULONG Version;
    FOURCC Param;
    UWORD Mapping;
    tlPoint Constants;
    ULONG NumFrames;
    array(UWORD, NumFrames) Frames;
    array(float, NumFrames) Values;
}

chunk tlVector2DOFChannelChunk : chunk_id(0x121103) subchunk(0x121110)
{
    ULONG Version;
    FOURCC Param;
    UWORD Mapping;
    tlPoint Constants;
    ULONG NumFrames;
    array(UWORD, NumFrames) Frames;
    array(tlPoint2D, NumFrames) Values;
}

chunk tlVector3DOFChannelChunk : chunk_id(0x121104) subchunk(0x121110)
{
    ULONG Version;
    FOURCC Param;
    ULONG NumFrames;
    array(UWORD, NumFrames) Frames;
    array(tlPoint, NumFrames) Values;
}

chunk tlQuaternionFormatChunk : chunk_id(0x12110F)
{
    ULONG Version;
    FOURCC Format;
}

chunk tlQuaternionChannelChunk : chunk_id(0x121105) subchunk(0x12110F) subchunk(0x121110)
{
    ULONG Version;
    FOURCC Param;
    ULONG NumFrames;
    array(UWORD, NumFrames) Frames;
    array(tlQuat, NumFrames) Values;
}

chunk tlCompressedQuaternionChannelChunk : chunk_id(0x121111) subchunk(0x121110)
{
    ULONG Version;
    FOURCC Param;
    ULONG NumFrames;
    array(UWORD, NumFrames) Frames;
    array(tlCompressedQuat, NumFrames) Values;
}

chunk tlStringChannelChunk : chunk_id(0x121106)
{
    ULONG Version;
    FOURCC Param;
    ULONG NumFrames;
    array(UWORD, NumFrames) Frames;
    array(string, NumFrames) Values;
}

chunk tlEntityChannelChunk : chunk_id(0x121107)
{
    ULONG Version;
    FOURCC Param;
    ULONG NumFrames;
    array(UWORD, NumFrames) Frames;
    array(string, NumFrames) Values;
}

chunk tlBoolChannelChunk : chunk_id(0x121108)
{
    ULONG Version;
    FOURCC Param;
    WORD StartState;
    ULONG NumFrames;
    array(UWORD, NumFrames) Values;
}

chunk tlColourChannelChunk : chunk_id(0x121109) subchunk(0x121110)
{
    ULONG Version;
    FOURCC Param;
    ULONG NumFrames;
    array(UWORD, NumFrames) Frames;
    array(COLOUR, NumFrames) Values;
}

chunk tlEventDataImageChunk : chunk_id(0x12110D)
{
    string DataFormat;
    ULONG DataLen;
    array(UBYTE, DataLen) Data;
}

chunk tlEventChunk : chunk_id(0x12110B) subchunk(0x12110D)
{
    string Name;
    ULONG Param;
    UWORD Time;
}

chunk tlEventChannelChunk : chunk_id(0x12110A) subchunk(0x12110B)
{
    ULONG Version;
    FOURCC Param;
    ULONG NumFrames;
}

chunk tlCollisionObjectChunk : chunk_id(0x20001) subchunk(0x7010021) subchunk(0x7010020) subchunk(0x7010001) subchunk(0x7010023)
{
    string Name;
    ULONG Version;
    string StringData;
    ULONG NumSubObject;
    ULONG NumOwner;
}

chunk tlCollisionObjectAttributeChunk : chunk_id(0x7010023)
{
    UWORD StaticAttribute;
    ULONG DefaultArea;
    UWORD CanRoll;
    UWORD CanSlide;
    UWORD CanSpin;
    UWORD CanBounce;
    ULONG ExtraAttribute1;
    ULONG ExtraAttribute2;
    ULONG ExtraAttribute3;
}

chunk tlCollisionVolumeOwnerChunk : chunk_id(0x7010021) subchunk(0x7010022)
{
    ULONG NumNames;
}

chunk tlCollisionVolumeOwnerNameChunk : chunk_id(0x7010022)
{
    string Name;
}

chunk tlSelfCollisionChunk : chunk_id(0x7010020)
{
    ULONG JointIndex1;
    ULONG JointIndex2;
    UWORD SelfOnly1;
    UWORD SelfOnly2;
}

chunk tlCollisionVolumeChunk : chunk_id(0x7010001) subchunk(0x7010006) subchunk(0x10004) subchunk(0x7010003) subchunk(0x7010004) subchunk(0x7010005) subchunk(0x7010001)
{
    ULONG ObjectReferenceIndex;
    ULONG OwnerIndex;
    ULONG NumSubVolume;
}

chunk tlBBoxVolumeChunk : chunk_id(0x7010006)
{
    ULONG Nothing;
}

chunk tlSphereVolumeChunk : chunk_id(0x10004) subchunk(0x121301)
{
    float SphereRadius;
}

chunk tlCylinderVolumeChunk : chunk_id(0x7010003) subchunk(0x121301)
{
    float CylinderRadius;
    float Length;
    UWORD FlatEnd;
}

chunk tlOBBoxVolumeChunk : chunk_id(0x7010004) subchunk(0x121301)
{
    float Length1;
    float Length2;
    float Length3;
}

chunk tlWallVolumeChunk : chunk_id(0x7010005) subchunk(0x121301)
{
}

chunk tlCollisionVectorChunk : chunk_id(0x121301)
{
    float X;
    float Y;
    float Z;
}

chunk tlCompositeDrawableChunk16 : chunk_id(0x4512) subchunk(0x4513) subchunk(0x4514) subchunk(0x4517)
{
    string Name;
    string SkeletonName;
}

chunk tlCompositeDrawableSkinListChunk16 : chunk_id(0x4513) subchunk(0x4515)
{
    ULONG NumElements;
}

chunk tlCompositeDrawablePropListChunk16 : chunk_id(0x4514) subchunk(0x4516)
{
    ULONG NumElements;
}

chunk tlCompositeDrawableSkinChunk16 : chunk_id(0x4515) subchunk(0x4519)
{
    string Name;
    ULONG IsTranslucent;
}

chunk tlCompositeDrawablePropChunk16 : chunk_id(0x4516) subchunk(0x4519)
{
    string Name;
    ULONG IsTranslucent;
    ULONG SkeletonJointID;
}

chunk tlCompositeDrawableEffectListChunk16 : chunk_id(0x4517) subchunk(0x4518)
{
    ULONG NumElements;
}

chunk tlCompositeDrawableEffectChunk16 : chunk_id(0x4518) subchunk(0x4519)
{
    string Name;
    ULONG IsTranslucent;
    ULONG SkeletonJointID;
}

chunk tlCompositeDrawableSortOrderChunk16 : chunk_id(0x4519)
{
    float SortOrder;
}

chunk tlDynaPhysDSGChunk : chunk_id(0x3F00002) subchunk(0x10000) subchunk(0x20001) subchunk(0x3000600) subchunk(0x20001) subchunk(0x3000008)
{
    string Name;
    ULONG Version;
    ULONG HasAlpha;
}

chunk tlEntityChannelChunk16 : chunk_id(0x42A0)
{
    ULONG NumFrames;
    array(UWORD, NumFrames) Times;
    array(string, NumFrames) Names;
}

chunk tlEntityDSGChunk : chunk_id(0x3F00000) subchunk(0x10000)
{
    string Name;
    ULONG Version;
    ULONG HasAlpha;
}

chunk tlExportInfoNamedIntChunk16 : chunk_id(0x7032)
{
    string Name;
    ULONG Value;
}

chunk tlExportInfoNamedStringChunk16 : chunk_id(0x7031)
{
    string Name;
    string Value;
}

chunk tlExportInfoChunk16 : chunk_id(0x7030) subchunk(0x7032) subchunk(0x7031)
{
    string Name;
}

chunk tlExpressionGroupChunk16 : chunk_id(0x4A01) subchunk(0x4A82)
{
    string Name;
    ULONG Version;
    string TargetName;
    ULONG NumExpression;
    ULONG NumPreset;
    array(ULONG, NumExpression) Stages;
}

chunk tlExpressionAnimChannelChunk16 : chunk_id(0x4A11)
{
    string Name;
    ULONG Version;
    string ExpressionName;
    ULONG NumKeys;
    array(UWORD, NumKeys) TimeIndex;
    array(float, NumKeys) StateKeys;
    array(float, NumKeys) WeightKeys;
}

chunk tlExpressionAnimChunk16 : chunk_id(0x4A10) subchunk(0x4A11)
{
    string Name;
    ULONG Version;
    ULONG NumFrames;
    float FrameRate;
    ULONG Cyclic;
    ULONG NumChannel;
}

chunk tlExpressionMixerChunk16 : chunk_id(0x4A20)
{
    string Name;
    ULONG Version;
    ULONG Type;
    string TargetName;
    string ExpressionGroupName;
}

chunk tlExpressionChunk : chunk_id(0x21000)
{
    ULONG Version;
    string Name;
    ULONG NumKeys;
    array(float, NumKeys) Keys;
    array(ULONG, NumKeys) Indices;
}

chunk tlExpressionGroupChunk : chunk_id(0x21001) subchunk(0x21000)
{
    ULONG Version;
    string Name;
    string TargetName;
    ULONG NumExpressions;
    array(ULONG, NumExpressions) Stages;
}

chunk tlExpressionMixerChunk : chunk_id(0x21002)
{
    ULONG Version;
    string Name;
    ULONG Type;
    string TargetName;
    string ExpressionGroupName;
}

chunk tlExtraMatrixChunk : chunk_id(0x300000C)
{
    tlMatrix Matrix;
}

chunk tlFenceDSGChunk : chunk_id(0x3F00007) subchunk(0x7010005)
{
}

chunk tlFenceLineChunk : chunk_id(0x3000001) subchunk(0x7010005)
{
    ULONG NumWalls;
}

struct FlexibleJointConnectionData
{
    ULONG Index1;
    ULONG Index2;
    float Length;
    UWORD OnBranch;
}

struct FlexibleJointHingeData
{
    ULONG Index1;
    ULONG Index2;
    ULONG Index3;
}

chunk tlFlexibleJointChunk : chunk_id(0x7011020) subchunk(0x7012021) subchunk(0x7012023) subchunk(0x7012022)
{
    string Name;
    ULONG Version;
}

chunk tlFlexibleLambdaJointParamChunk : chunk_id(0x7012023)
{
    string Name;
    float LambdaF;
    float LambdaD;
    float KappaF;
    float KappaD;
    float Wind1DKf;
}

chunk tlFlexibleJointParametersChunk : chunk_id(0x7012021)
{
    string Name;
    float Stretch1Dkf;
    float Stretch1Dkd;
    float Bend1Dkf;
    float Bend1Dkd;
    float Wind1DKf;
}

chunk tlFlexibleJointDefinitionChunk : chunk_id(0x7012022)
{
    string Name;
    string ParametersName;
    string RestingPosParametersName;
    string ConnectionParametersName;
    UWORD Gravity;
    UWORD SimMethod;
    UWORD UseRestingPos;
    UWORD RestMethod;
    UWORD UpdateMethod;
    UWORD UseVirtualJoint;
    UWORD Solver;
    float ExternalDensityFactor;
    ULONG NumJointIndex;
    array(ULONG, NumJointIndex) FlexibleJointIndex;
    ULONG NumFixJointIndex;
    array(ULONG, NumFixJointIndex) FlexibleFixJointIndex;
    ULONG NumEndOfBranchIndex;
    array(ULONG, NumEndOfBranchIndex) EndOfBranchIndex;
    ULONG NumConnection;
    array(FlexibleJointConnectionData, NumConnection) FlexibleJointConnection;
    ULONG NumHinge;
    array(FlexibleJointHingeData, NumHinge) FlexibleJointHinge;
}

struct FlexibleParticlePosData
{
    float x;
    float y;
    float z;
}

struct FlexibleObjectTriMapData
{
    UWORD v1;
    UWORD v2;
    UWORD v3;
}

struct FlexibleObjectEdgeMapData
{
    UWORD v1;
    UWORD v2;
}

chunk tlFlexibleObjectChunk : chunk_id(0x20001) subchunk(0x7012002) subchunk(0x7012003) subchunk(0x7012004) subchunk(0x7012005) subchunk(0x7012006) subchunk(0x7012001) subchunk(0x7012007)
{
    string Name;
    ULONG Version;
    UWORD Dimension;
    float ExternalDensityFactor;
    UWORD Solver;
    UWORD NbParticle;
    array(FlexibleParticlePosData, NbParticle) FlexibleParticlePos;
    UWORD NbPGM;
    array(UWORD, NbPGM) PGMNbVertex;
}

chunk tlFlexibleObjectFixParticleChunk : chunk_id(0x7012002)
{
    UWORD FixParticleCount;
    array(UWORD, FixParticleCount) FixParticle;
}

chunk tlFlexibleObjectMapToVLChunk : chunk_id(0x7012003)
{
    UWORD VCount;
    array(UWORD, VCount) MapToVL;
}

chunk tlFlexibleObjectTriMapChunk : chunk_id(0x7012004)
{
    UWORD TriMapCount;
    array(FlexibleObjectTriMapData, TriMapCount) TriMap;
}

chunk tlFlexibleObjectEdgeMapChunk : chunk_id(0x7012005)
{
    UWORD EdgeMapCount;
    array(FlexibleObjectEdgeMapData, EdgeMapCount) EdgeMap;
}

chunk tlFlexibleObjectEdgeLengthChunk : chunk_id(0x7012006)
{
    UWORD EdgeLengthCount;
    array(float, EdgeLengthCount) EdgeLength;
}

chunk tlFlexibleObjectParamChunk : chunk_id(0x7012001)
{
    float Stretch1Dkf;
    float Stretch1Dkd;
    float Bend1Dkf;
    float Bend1Dkd;
    float Wind1DKf;
    float Stretch2Dkf;
    float Stretch2Dkd;
    float Shear2Dkf;
    float Shear2Dkd;
    float Bend2Dkf;
    float Bend2Dkd;
    float Wind2DKf;
}

chunk tlFlexibleLambdaObjectParamChunk : chunk_id(0x7012007)
{
    string Name;
    float LambdaF;
    float LambdaD;
    float KappaF;
    float KappaD;
    float IotaF;
    float IotaD;
    float Wind1DKf;
    float Wind2DKf;
}

chunk tlFollowCamDataChunk : chunk_id(0x3000100)
{
    ULONG ID;
    float Rotation;
    float Elevation;
    float Magnitude;
    tlPoint TargetOffset;
}

struct tlTextureGlyph
{
    ULONG TextureNum;
    tlUV BottomLeft;
    tlUV TopRight;
    float LeftBearing;
    float RightBearing;
    float Width;
    float Advance;
    ULONG Code;
}

struct tlImageGlyph
{
    UWORD XOrigin;
    UWORD LeftBearing;
    UWORD RightBearing;
    UWORD Width;
    UWORD Advance;
    ULONG Code;
}

chunk tlTextureFontChunk : chunk_id(0x22000) subchunk(0x19000) subchunk(0x22001)
{
    ULONG Version;
    string Name;
    string Shader;
    float FontSize;
    float FontWidth;
    float FontHeight;
    float FontBaseLine;
    ULONG NumTextures;
}

chunk tlTextureGlyphListChunk : chunk_id(0x22001)
{
    ULONG NumGlyphs;
    array(tlTextureGlyph, NumGlyphs) Glyphs;
}

chunk tlImageFontChunk : chunk_id(0x22002) subchunk(0x19001) subchunk(0x22003)
{
    ULONG Version;
    string Name;
    ULONG FontSize;
    ULONG FontWidth;
    ULONG FontHeight;
    ULONG FontBaseLine;
}

chunk tlImageGlyphListChunk : chunk_id(0x22003)
{
    ULONG NumGlyphs;
    array(tlImageGlyph, NumGlyphs) Glyphs;
}

chunk tlFrameControllerChunk : chunk_id(0x121200)
{
    ULONG Version;
    string Name;
    FOURCC Type;
    float FrameOffset;
    string HierarchyName;
    string AnimationName;
}

chunk tlFrameControllerChunk16 : chunk_id(0x4520)
{
    string Name;
    ULONG Version;
    ULONG Type;
    string HierarchyName;
    string AnimationName;
}

chunk tlGameAttrChunk : chunk_id(0x12000) subchunk(0x11003) subchunk(0x11004) subchunk(0x11005) subchunk(0x11006) subchunk(0x11007)
{
    string Name;
    ULONG Version;
    ULONG NumParams;
}

chunk tlGameAttrIntParamChunk : chunk_id(0x11003)
{
    string ParamName;
    ULONG Value;
}

chunk tlGameAttrFloatParamChunk : chunk_id(0x11004)
{
    string ParamName;
    float Value;
}

chunk tlGameAttrColourParamChunk : chunk_id(0x11005)
{
    string ParamName;
    COLOUR Value;
}

chunk tlGameAttrVectorParamChunk : chunk_id(0x11006)
{
    string ParamName;
    tlPoint Value;
}

chunk tlGameAttrMatrixParamChunk : chunk_id(0x11007)
{
    string ParamName;
    tlMatrix Value;
}

chunk tlHistoryChunk16 : chunk_id(0x7000)
{
    UWORD NumLines;
    array(string, NumLines) History;
}

chunk tlImageDataChunk : chunk_id(0x19002)
{
    ULONG ImageDataSize;
    array(UBYTE, ImageDataSize) ImageData;
}

chunk tlImageFileNameChunk : chunk_id(0x19003)
{
    string FileName;
}

chunk tlImageChunk : chunk_id(0x19001) subchunk(0x19002) subchunk(0x19003)
{
    string Name;
    ULONG Version;
    ULONG Width;
    ULONG Height;
    ULONG Bpp;
    ULONG Palettized;
    ULONG HasAlpha;
    ULONG Format;
}

chunk tlVolumeImageChunk : chunk_id(0x19004) subchunk(0x19001)
{
    string Name;
    ULONG Version;
    ULONG Width;
    ULONG Height;
    ULONG Depth;
    ULONG Bpp;
    ULONG Palettized;
    ULONG HasAlpha;
    ULONG Format;
}

chunk tlInstAnimDynaPhysDSGChunk : chunk_id(0x3F0000E) subchunk(0x3F0000F) subchunk(0x3F00010) subchunk(0x3000600) subchunk(0x3000008)
{
    string Name;
    ULONG Version;
    ULONG HasAlpha;
}

chunk tlInstParticleSystemChunk : chunk_id(0x3001001) subchunk(0x15800) subchunk(0x121200) subchunk(0x11000) subchunk(0x20002) subchunk(0x19000)
{
    ULONG ParticleType;
    ULONG MaxInstances;
}

chunk tlInstaEntityDSGChunk : chunk_id(0x3F00009) subchunk(0x10000) subchunk(0x3000008)
{
    string Name;
    ULONG Version;
    ULONG HasAlpha;
}

chunk tlInstaStaticPhysDSGChunk : chunk_id(0x3F0000A) subchunk(0x10000) subchunk(0x3000600) subchunk(0x20001) subchunk(0x3000008)
{
    string Name;
    ULONG Version;
    ULONG HasAlpha;
}

chunk tlInstancesChunk : chunk_id(0x3000008) subchunk(0x120100)
{
    string Name;
}

chunk tlIntersectDSGChunk : chunk_id(0x3F00003) subchunk(0x10003) subchunk(0x10004) subchunk(0x300000E)
{
    ULONG NumIndices;
    array(ULONG, NumIndices) Indices;
    ULONG NumPositions;
    array(tlPoint, NumPositions) Positions;
    ULONG NumNormals;
    array(tlPoint, NumNormals) Normals;
}

chunk tlIntersectionChunk : chunk_id(0x3000004)
{
    string Name;
    tlPoint Centre;
    float Radius;
    ULONG Type;
}

chunk tlLensFlareDSGChunk : chunk_id(0x3F0000D) subchunk(0x4512) subchunk(0x17002) subchunk(0x10000)
{
    string Name;
    ULONG Version;
    ULONG NumBillBoardQuads;
}

chunk tlLightAnimChunk16 : chunk_id(0x4980) subchunk(0x4981)
{
    string AnimName;
    ULONG Version;
    ULONG NumFrames;
    float FrameRate;
    ULONG Cyclic;
}

chunk tlLightAnimChannelChunk16 : chunk_id(0x4981) subchunk(0x4982) subchunk(0x4983) subchunk(0x4985)
{
}

chunk tlLightAnimColourChannelChunk16 : chunk_id(0x4982) subchunk(0x4216)
{
}

chunk tlLightAnimParamChannelChunk16 : chunk_id(0x4983) subchunk(0x4801)
{
}

chunk tlLightAnimEnableChannelChunk16 : chunk_id(0x4985) subchunk(0x4800)
{
}

chunk tlLightChunk : chunk_id(0x13000) subchunk(0x13001) subchunk(0x13003) subchunk(0x13002) subchunk(0x13004) subchunk(0x13006) subchunk(0x13008)
{
    string Name;
    ULONG Version;
    ULONG Type;
    COLOUR Colour;
    float Constant;
    float Linear;
    float Squared;
    ULONG Enabled;
}

chunk tlLightDirectionChunk : chunk_id(0x13001)
{
    tlPoint Direction;
}

chunk tlLightPositionChunk : chunk_id(0x13002)
{
    tlPoint Position;
}

chunk tlLightConeParamChunk : chunk_id(0x13003)
{
    float Phi;
    float Theta;
    float Falloff;
    float Range;
}

chunk tlLightShadowChunk : chunk_id(0x13004)
{
    ULONG Shadow;
}

chunk tlLightDecayRangeRotationYChunk : chunk_id(0x13007)
{
    float RotationY;
}

chunk tlLightDecayRangeChunk : chunk_id(0x13006) subchunk(0x13007)
{
    ULONG DecayRangeType;
    tlPoint Inner;
    tlPoint Outer;
}

chunk tlLightIlluminationTypeChunk : chunk_id(0x13008) subchunk(0x13008)
{
    ULONG IlluminationType;
}

chunk tlLightGroupChunk16 : chunk_id(0x2380)
{
    string Name;
    ULONG NumLights;
    array(string, NumLights) Lights;
}

chunk tlLocatorChunk : chunk_id(0x14000)
{
    string Name;
    ULONG Version;
    tlPoint Position;
}

chunk tlMemorySectionChunk : chunk_id(0xFFFF0000)
{
    string Name;
    ULONG BeginSection;
    ULONG EndSection;
}

chunk tlMeshChunk : chunk_id(0x10000) subchunk(0x10020) subchunk(0x10003) subchunk(0x10004) subchunk(0x10017) subchunk(0x10018)
{
    string Name;
    ULONG Version;
    ULONG NumPrimGroups;
}

chunk tlRenderStatusChunk : chunk_id(0x10017)
{
    ULONG CastShadow;
}

struct tlMultiControllerTrackData
{
    string Name;
    float StartTime;
    float EndTime;
    float Scale;
}

chunk tlMultiControllerChunk16 : chunk_id(0x48A0) subchunk(0x48A1) subchunk(0x48A2)
{
    string Name;
    ULONG Version;
    float Length;
    float Framerate;
    ULONG NumTracks;
}

chunk tlMultiControllerTracksChunk16 : chunk_id(0x48A1)
{
    ULONG NumTracks;
    array(tlMultiControllerTrackData, NumTracks) Tracks;
}

chunk tlMultiControllerTrackChunk16 : chunk_id(0x48A2)
{
    string Name;
    float StartTime;
    float EndTime;
    float Scale;
}

chunk tlObjectAttributeChunk : chunk_id(0x3000600)
{
    ULONG ClassType;
    ULONG PhyPropID;
    string Sound;
}

chunk tlLensFlareChunk : chunk_id(0x16007)
{
    ULONG Version;
    string Name;
    COLOUR Colour;
    tlUV Uv0;
    tlUV Uv1;
    tlUV Uv2;
    tlUV Uv3;
    tlUV UvOffset;
    float Distance;
    float Width;
    float Height;
}

chunk tlLensFlareGroupChunk : chunk_id(0x16006) subchunk(0x16007)
{
    ULONG Version;
    string Name;
    string ShaderName;
    ULONG ZTest;
    ULONG ZWrite;
    ULONG Fog;
    float SourceRadius;
    float EdgeRadius;
    ULONG NumFlares;
}

chunk tlOpticVectorV14Chunk : chunk_id(0x16F00)
{
    float X;
    float Y;
    float Z;
}

chunk tlCoronaV14Chunk : chunk_id(0x16000) subchunk(0x16F00)
{
    string Name;
    ULONG Version;
    string Shader;
    COLOUR Colour;
    ULONG TextureFrameRate;
    ULONG NumTextureFrames;
    float Width;
    float Height;
    float SourceRadius;
    float FallOffTime;
    float DistanceCutOff;
    ULONG PerspectiveScale;
    float FrameRate;
    ULONG BillboardMode;
}

chunk tlLensFlareParentV14Chunk : chunk_id(0x16001) subchunk(0x16002) subchunk(0x16F00)
{
    string Name;
    ULONG Version;
    ULONG NumFlares;
    float SourceRadius;
    float FallOffTime;
    float DistanceCutOff;
    float FrameRate;
}

chunk tlLensFlareV14Chunk : chunk_id(0x16002)
{
    string Shader;
    COLOUR Colour;
    float Distance;
    float Width;
    float Height;
}

chunk tlParticleInstancingInfoChunk : chunk_id(0x1580B)
{
    ULONG Version;
    ULONG MaxInstances;
}

chunk tlParticleSystemFactoryChunk : chunk_id(0x15800) subchunk(0x1580B) subchunk(0x15805) subchunk(0x15806) subchunk(0x15807)
{
    ULONG Version;
    string Name;
    float FrameRate;
    ULONG NumAnimFrames;
    ULONG NumOLFrames;
    UWORD CycleAnim;
    UWORD EnableSorting;
    ULONG NumEmitters;
}

chunk tlParticleSystemChunk : chunk_id(0x15801)
{
    ULONG Version;
    string Name;
    string FactoryName;
}

chunk tlParticleAnimationChunk : chunk_id(0x15808) subchunk(0x20002)
{
    ULONG Version;
}

chunk tlEmitterAnimationChunk : chunk_id(0x15809) subchunk(0x20002)
{
    ULONG Version;
}

chunk tlGeneratorAnimationChunk : chunk_id(0x1580A) subchunk(0x20002)
{
    ULONG Version;
}

chunk tlBaseEmitterFactoryChunk : chunk_id(0x15805) subchunk(0x15808) subchunk(0x15809) subchunk(0x1580A)
{
    ULONG Version;
    string Name;
    FOURCC ParticleType;
    FOURCC GeneratorType;
    ULONG ZTest;
    ULONG ZWrite;
    ULONG Fog;
    ULONG MaxParticles;
    ULONG InfiniteLife;
    float RotationalCohesion;
    float TranslationalCohesion;
}

chunk tlSpriteEmitterFactoryChunk : chunk_id(0x15806) subchunk(0x15805)
{
    ULONG Version;
    string Name;
    string ShaderName;
    FOURCC AngleMode;
    float Angle;
    FOURCC TextureAnimMode;
    ULONG NumTextureFrames;
    ULONG TextureFrameRate;
}

chunk tlDrawableEmitterFactoryChunk : chunk_id(0x15807) subchunk(0x15805)
{
    ULONG Version;
    string Name;
    string DrawableName;
    FOURCC AngleMode;
    float Angle;
}

chunk tlPedpathChunk : chunk_id(0x300000B)
{
    ULONG NumPoints;
    array(tlPoint, NumPoints) Points;
}

chunk tlPhotonMapChunk : chunk_id(0x13005)
{
    string Name;
    ULONG Version;
    ULONG NumLights;
    array(string, NumLights) Lights;
    array(float, NumLights) LightScales;
    ULONG NumPhotons;
    array(tlPhoton, NumPhotons) Photons;
}

chunk tlPhysWrapperChunk : chunk_id(0x3000601) subchunk(0x3000600) subchunk(0x20001)
{
    string Name;
}

chunk tlPhysicsObjectChunk : chunk_id(0x20001) subchunk(0x121301) subchunk(0x7011001) subchunk(0x7011020)
{
    string Name;
    ULONG Version;
    string StringData;
    ULONG NumJoints;
    float Volume;
    float RestingSensitivity;
}

chunk tlPhysicsJointChunk : chunk_id(0x7011020) subchunk(0x121301) subchunk(0x7011001)
{
    ULONG Index;
    float Volume;
    float Stiffness;
    float MaxAngle;
    float MinAngle;
    ULONG DOF;
}

chunk tlPhysicsVectorChunk : chunk_id(0x121301)
{
    float X;
    float Y;
    float Z;
}

chunk tlPhysicsInertiaMatrixChunk : chunk_id(0x7011001)
{
    float XX;
    float XY;
    float XZ;
    float YY;
    float YZ;
    float ZZ;
}

struct tlTransKeyData
{
    float X;
    float Y;
    float Z;
}

struct tlRotKeyData
{
    UWORD X;
    UWORD Y;
    UWORD Z;
}

struct tlQuatKeyData
{
    float W;
    float X;
    float Y;
    float Z;
}

chunk tlPoseAnimChunk16 : chunk_id(0x4700) subchunk(0x4201) subchunk(0x4703)
{
    string Name;
    ULONG Version;
    string SkeletonName;
    ULONG NumFrames;
    float FrameRate;
    ULONG Cyclic;
}

chunk tlPoseJointListChunk16 : chunk_id(0x4201) subchunk(0x4702)
{
    ULONG ListType;
    ULONG NumJoints;
}

chunk tlPoseAnimMirroredChunk16 : chunk_id(0x4703)
{
    string Name;
}

chunk tlAnimChannelChunk16 : chunk_id(0x4702) subchunk(0x4800) subchunk(0x4801) subchunk(0x4802) subchunk(0x4803) subchunk(0x4806) subchunk(0x4804) subchunk(0x4805) subchunk(0x4807)
{
    string Name;
    ULONG Version;
    ULONG PoseIndex;
}

chunk tlChannel1DOFChunk16 : chunk_id(0x4800)
{
    ULONG NumKeys;
    ULONG Mapping;
    ULONG NumConstants;
    array(float, NumConstants) Constants;
    array(UWORD, NumKeys) TimeIndex;
    array(float, NumKeys) Frames;
}

chunk tlChannel3DOFChunk16 : chunk_id(0x4801)
{
    ULONG NumKeys;
    array(UWORD, NumKeys) TimeIndex;
    array(tlTransKeyData, NumKeys) Frames;
}

chunk tlChannel1DOFAngleChunk16 : chunk_id(0x4802)
{
    ULONG NumKeys;
    ULONG Mapping;
    ULONG NumConstants;
    array(float, NumConstants) Constants;
    array(UWORD, NumKeys) TimeIndex;
    array(UWORD, NumKeys) Frames;
}

chunk tlChannel3DOFAngleChunk16 : chunk_id(0x4803)
{
    ULONG NumKeys;
    array(UWORD, NumKeys) TimeIndex;
    array(ULONG, NumKeys) Frames;
}

chunk tlChannelQuaternionChunk16 : chunk_id(0x4806)
{
    ULONG NumKeys;
    array(UWORD, NumKeys) TimeIndex;
    array(tlQuatKeyData, NumKeys) Frames;
}

chunk tlChannelStaticVectorChunk16 : chunk_id(0x4804)
{
    float X;
    float Y;
    float Z;
}

chunk tlChannelStaticAngleChunk16 : chunk_id(0x4805)
{
    float X;
    float Y;
    float Z;
}

chunk tlChannelStaticQuaternionChunk16 : chunk_id(0x4807)
{
    float W;
    float X;
    float Y;
    float Z;
}

chunk tlKeyListColourChunk16 : chunk_id(0x4216)
{
    ULONG NumKeys;
    array(UWORD, NumKeys) Frames;
    array(ULONG, NumKeys) Keys;
}

struct tlVtxOffset
{
    ULONG Index;
    float OffsetX;
    float OffsetY;
    float OffsetZ;
}

chunk tlPrimGroupChunk : chunk_id(0x10020) subchunk(0x10011) subchunk(0x10005) subchunk(0x10006) subchunk(0x10010) subchunk(0x10007) subchunk(0x10008) subchunk(0x1001C) subchunk(0x10009) subchunk(0x1000A) subchunk(0x1000B) subchunk(0x1000C) subchunk(0x1000D) subchunk(0x1000F) subchunk(0x10012) subchunk(0x10013) subchunk(0x10014) subchunk(0x10015) subchunk(0x10016) subchunk(0x1000E)
{
    ULONG Version;
    string Shader;
    ULONG PrimitiveType;
    ULONG VertexType;
    ULONG NumVertices;
    ULONG NumIndices;
    ULONG NumMatrices;
}

chunk tlVertexShaderChunk : chunk_id(0x10011)
{
    string VertexShaderName;
}

chunk tlPositionListChunk : chunk_id(0x10005)
{
    ULONG NumPositions;
    array(tlPoint, NumPositions) Positions;
}

chunk tlNormalListChunk : chunk_id(0x10006)
{
    ULONG NumNormals;
    array(tlPoint, NumNormals) Normals;
}

chunk tlTangentListChunk : chunk_id(0x10015)
{
    ULONG NumTangents;
    array(tlPoint, NumTangents) Tangents;
}

chunk tlBinormalListChunk : chunk_id(0x10016)
{
    ULONG NumBinormals;
    array(tlPoint, NumBinormals) Binormals;
}

chunk tlPackedNormalListChunk : chunk_id(0x10010)
{
    ULONG NumNormals;
    array(UBYTE, NumNormals) Normals;
}

chunk tlUVListChunk : chunk_id(0x10007)
{
    ULONG NumUVs;
    ULONG Channel;
    array(tlUV, NumUVs) UVs;
}

chunk tlColourListChunk : chunk_id(0x10008)
{
    ULONG NumColours;
    array(tlColour, NumColours) Colours;
}

chunk tlMultiColourListChunk : chunk_id(0x1001C)
{
    ULONG NumColours;
    ULONG Channel;
    array(tlColour, NumColours) Colours;
}

chunk tlStripListChunk : chunk_id(0x10009)
{
    ULONG NumStrips;
    array(ULONG, NumStrips) Strips;
}

chunk tlIndexListChunk : chunk_id(0x1000A)
{
    ULONG NumIndices;
    array(ULONG, NumIndices) Indices;
}

chunk tlExpressionOffsetsChunk : chunk_id(0x10018) subchunk(0x1000E)
{
    ULONG NumPrimGroups;
    ULONG NumOffsetLists;
    array(ULONG, NumPrimGroups) PrimGroupIndices;
}

chunk tlOffsetListChunk : chunk_id(0x1000E)
{
    ULONG NumOffsets;
    ULONG KeyIndex;
    array(tlVtxOffset, NumOffsets) Offsets;
    ULONG PrimGroupIndex;
}

chunk tlMatrixListChunk : chunk_id(0x1000B)
{
    ULONG NumMatrices;
    array(COLOUR, NumMatrices) Matrices;
}

chunk tlWeightListChunk : chunk_id(0x1000C)
{
    ULONG NumWeights;
    array(tlPoint, NumWeights) Weights;
}

chunk tlMatrixPaletteChunk : chunk_id(0x1000D)
{
    ULONG NumMatrices;
    array(ULONG, NumMatrices) Matrices;
}

chunk tlInstanceInfoChunk : chunk_id(0x1000F)
{
    ULONG NumInstances;
    ULONG VertexCount;
    ULONG IndexCount;
}

chunk tlPrimGroupMemoryImageVertexDescriptionChunk : chunk_id(0x10014)
{
    ULONG Version;
    ULONG Param;
    ULONG MemoryImageVertexDescriptionSize;
    array(UBYTE, MemoryImageVertexDescriptionSize) MemoryImageVertexDescription;
}

chunk tlPrimGroupMemoryImageVertexChunk : chunk_id(0x10012)
{
    ULONG Version;
    ULONG Param;
    ULONG MemoryImageVertexSize;
    array(UBYTE, MemoryImageVertexSize) MemoryImageVertex;
}

chunk tlPrimGroupMemoryImageIndexChunk : chunk_id(0x10013)
{
    ULONG Version;
    ULONG Param;
    ULONG MemoryImageIndexSize;
    array(UBYTE, MemoryImageIndexSize) MemoryImageIndex;
}

chunk tlRoadChunk : chunk_id(0x3000003) subchunk(0x3000002)
{
    string Name;
    ULONG Type;
    string StartIntersection;
    string EndIntersection;
    ULONG Density;
    ULONG Speed;
}

chunk tlRoadSegmentChunk : chunk_id(0x3000002)
{
    string Name;
    string RoadSegmentData;
    tlMatrix HierarchyMatrix;
    tlMatrix ScaleMatrix;
}

chunk tlRoadSegmentDataChunk : chunk_id(0x3000009)
{
    string Name;
    ULONG Type;
    ULONG NumLanes;
    ULONG HasShoulder;
    tlPoint Direction;
    tlPoint Top;
    tlPoint Bottom;
}

chunk tlScenegraphChunk : chunk_id(0x120100) subchunk(0x120101)
{
    string Name;
    ULONG Version;
}

chunk tlScenegraphRootChunk : chunk_id(0x120101) subchunk(0x120102) subchunk(0x120103) subchunk(0x120107) subchunk(0x120105) subchunk(0x120108) subchunk(0x120109) subchunk(0x120104)
{
}

chunk tlScenegraphBranchChunk : chunk_id(0x120102) subchunk(0x120102) subchunk(0x120103) subchunk(0x120107) subchunk(0x120105) subchunk(0x120108) subchunk(0x120109) subchunk(0x120104)
{
    string Name;
    ULONG NChildren;
}

chunk tlScenegraphTransformChunk : chunk_id(0x120103) subchunk(0x120103) subchunk(0x120107) subchunk(0x120105) subchunk(0x120108) subchunk(0x120109) subchunk(0x120104)
{
    string Name;
    ULONG NChildren;
    tlMatrix Transform;
}

chunk tlScenegraphVisibilityChunk : chunk_id(0x120104) subchunk(0x120102) subchunk(0x120103) subchunk(0x120107) subchunk(0x120105) subchunk(0x120108) subchunk(0x120109) subchunk(0x120104)
{
    string Name;
    ULONG NChildren;
    ULONG IsVisible;
}

chunk tlScenegraphAttachmentChunk : chunk_id(0x120105) subchunk(0x120106)
{
    string Name;
    string DrawablePoseName;
    ULONG NPoints;
}

chunk tlScenegraphAttachmentPointChunk : chunk_id(0x120106) subchunk(0x120102) subchunk(0x120103) subchunk(0x120107) subchunk(0x120105) subchunk(0x120108) subchunk(0x120109) subchunk(0x120104)
{
    ULONG Joint;
}

chunk tlScenegraphDrawableChunk : chunk_id(0x120107) subchunk(0x12010A)
{
    string Name;
    string DrawableName;
    ULONG IsTranslucent;
}

chunk tlScenegraphCameraChunk : chunk_id(0x120108)
{
    string Name;
    string CameraName;
}

chunk tlScenegraphLightGroupChunk : chunk_id(0x120109)
{
    string Name;
    string LightGroupName;
}

chunk tlScenegraphSortOrderChunk : chunk_id(0x12010A)
{
    float SortOrder;
}

chunk tlScenegraphTransformAnimChunk16 : chunk_id(0x9150) subchunk(0x4800) subchunk(0x4801) subchunk(0x4802) subchunk(0x4803) subchunk(0x4804) subchunk(0x4805)
{
    string Name;
    ULONG Version;
    string NodeName;
    ULONG NumFrames;
    float FrameRate;
    ULONG Cyclic;
}

chunk tlScroobyProjectChunk : chunk_id(0x18000) subchunk(0x18002) subchunk(0x18001)
{
    string Name;
    ULONG Version;
    ULONG ResolutionX;
    ULONG ResolutionY;
    string Platform;
    string PagePath;
    string ResourcePath;
    string ScreenPath;
}

chunk tlScroobyScreenChunk : chunk_id(0x18001)
{
    string Name;
    ULONG Version;
    ULONG NumberOfPages;
    array(string, NumberOfPages) PageNames;
}

chunk tlScroobyPageChunk : chunk_id(0x18002) subchunk(0x18003) subchunk(0x18100) subchunk(0x18101) subchunk(0x18104) subchunk(0x18105)
{
    string Name;
    ULONG Version;
    ULONG ResolutionX;
    ULONG ResolutionY;
}

chunk tlScroobyLayerChunk : chunk_id(0x18003) subchunk(0x18004) subchunk(0x18007) subchunk(0x18006) subchunk(0x18009) subchunk(0x18008)
{
    string Name;
    ULONG Version;
    ULONG Visible;
    ULONG Editable;
    ULONG Alpha;
}

chunk tlScroobyStringTextBibleChunk : chunk_id(0x1800B)
{
    string BibleName;
    string StringId;
}

chunk tlScroobyStringHardCodedChunk : chunk_id(0x1800C)
{
    string String;
}

chunk tlScroobyMultiTextChunk : chunk_id(0x18007) subchunk(0x1800B) subchunk(0x1800C)
{
    string Name;
    ULONG Version;
    ULONG PositionX;
    ULONG PositionY;
    ULONG DimensionX;
    ULONG DimensionY;
    ULONG JustificationX;
    ULONG JustificationY;
    COLOUR Color;
    ULONG Translucency;
    float RotationValue;
    string TextStyleName;
    UBYTE ShadowEnabled;
    COLOUR ShadowColour;
    ULONG ShadowOffsetX;
    ULONG ShadowOffsetY;
    ULONG CurrentText;
}

chunk tlScroobyMultiSpriteChunk : chunk_id(0x18006)
{
    string Name;
    ULONG Version;
    ULONG PositionX;
    ULONG PositionY;
    ULONG DimensionX;
    ULONG DimensionY;
    ULONG JustificationX;
    ULONG JustificationY;
    COLOUR Color;
    ULONG Translucency;
    float RotationValue;
    ULONG NumberOfImages;
    array(string, NumberOfImages) ImageNames;
}

chunk tlScroobyPolygonChunk : chunk_id(0x18009)
{
    string Name;
    ULONG Version;
    ULONG Translucency;
    ULONG NumPoints;
    array(tlPoint, NumPoints) Points;
    array(tlColour, NumPoints) Colors;
}

chunk tlScroobyGroupChunk : chunk_id(0x18004) subchunk(0x18004) subchunk(0x18006) subchunk(0x18007) subchunk(0x18009) subchunk(0x18008)
{
    string Name;
    ULONG Version;
    ULONG Alpha;
}

chunk tlScroobyPure3dObjectChunk : chunk_id(0x18008)
{
    string Name;
    ULONG Version;
    ULONG PositionX;
    ULONG PositionY;
    ULONG DimensionX;
    ULONG DimensionY;
    ULONG JustificationX;
    ULONG JustificationY;
    COLOUR Color;
    ULONG Translucency;
    float RotationValue;
    string Pure3dFilename;
}

chunk tlScroobyTextBibleChunk : chunk_id(0x1800D) subchunk(0x1800E)
{
    string Name;
    ULONG NumLanguages;
    string Languages;
}

chunk tlScroobyLanguageChunk : chunk_id(0x1800E)
{
    string Name;
    UBYTE Language;
    ULONG NumStrings;
    ULONG Modulo;
    ULONG SizeOfBuffer;
    array(ULONG, NumStrings) Hashes;
    array(ULONG, NumStrings) Offsets;
    array(UBYTE, SizeOfBuffer) Buffer;
}

chunk tlScroobyImageResourceChunk : chunk_id(0x18100)
{
    string Name;
    ULONG Version;
    string Filename;
}

chunk tlScroobyPure3dResourceChunk : chunk_id(0x18101)
{
    string Name;
    ULONG Version;
    string Filename;
    string InventoryName;
    string CameraName;
    string AnimationName;
}

chunk tlScroobyTextStyleResourceChunk : chunk_id(0x18104)
{
    string Name;
    ULONG Version;
    string Filename;
    string InventoryName;
}

chunk tlScroobyTextBibleResourceChunk : chunk_id(0x18105)
{
    string Name;
    ULONG Version;
    string Filename;
    string InventoryName;
}

chunk tlScroobyTextStyleResourceChunk16 : chunk_id(0x18102)
{
    string Name;
    ULONG Version;
    string Filename;
}

chunk tlScroobyTextBibleResourceChunk16 : chunk_id(0x18103)
{
    string Name;
    ULONG Version;
    string Filename;
}

chunk tlSetChunk : chunk_id(0x3000110) subchunk(0x19000)
{
    string Name;
    ULONG Version;
    UBYTE ChildCount;
}

chunk tlShaderChunk : chunk_id(0x11000) subchunk(0x11001) subchunk(0x11002) subchunk(0x11003) subchunk(0x11004) subchunk(0x11005) subchunk(0x11006) subchunk(0x11007)
{
    string Name;
    ULONG Version;
    string PddiShaderName;
    ULONG HasTranslucency;
    ULONG VertexNeeds;
    ULONG VertexMask;
    ULONG NumParams;
}

chunk tlShaderDefinitionChunk : chunk_id(0x11001)
{
    string Name;
    longstring Definition;
}

chunk tlShaderTextureParamChunk : chunk_id(0x11002)
{
    FOURCC Param;
    string Value;
}

chunk tlShaderIntParamChunk : chunk_id(0x11003)
{
    FOURCC Param;
    ULONG Value;
}

chunk tlShaderFloatParamChunk : chunk_id(0x11004)
{
    FOURCC Param;
    float Value;
}

chunk tlShaderColourParamChunk : chunk_id(0x11005)
{
    FOURCC Param;
    COLOUR Value;
}

chunk tlShaderVectorParamChunk : chunk_id(0x11006)
{
    FOURCC Param;
    tlPoint Value;
}

chunk tlShaderMatrixParamChunk : chunk_id(0x11007)
{
    FOURCC Param;
    tlMatrix Value;
}

struct tlTopologyStruct
{
    UWORD v0;
    UWORD v1;
    UWORD v2;
    UWORD n0;
    UWORD n1;
    UWORD n2;
}

chunk tlShadowSkinChunk : chunk_id(0x10019) subchunk(0x10005) subchunk(0x1000C) subchunk(0x1000B) subchunk(0x1001B) subchunk(0x10003) subchunk(0x10004)
{
    string Name;
    ULONG Version;
    string SkeletonName;
    ULONG NumVertices;
    ULONG NumTriangles;
}

chunk tlTopologyChunk : chunk_id(0x1001B)
{
    ULONG NumTopology;
    array(tlTopologyStruct, NumTopology) Topology;
}

chunk tlShadowMeshChunk : chunk_id(0x1001A) subchunk(0x10005) subchunk(0x1001B) subchunk(0x10003) subchunk(0x10004)
{
    string Name;
    ULONG Version;
    ULONG NumVertices;
    ULONG NumTriangles;
}

chunk tlSkeletonChunk16 : chunk_id(0x4500) subchunk(0x4501)
{
    string Name;
    ULONG Version;
    ULONG NumJoints;
}

chunk tlSkeletonJointChunk16 : chunk_id(0x4501) subchunk(0x4504) subchunk(0x4503)
{
    string Name;
    ULONG Parent;
    ULONG DOF;
    ULONG FreeAxes;
    ULONG PrimaryAxis;
    ULONG SecondaryAxis;
    ULONG TwistAxis;
    tlMatrix RestPose;
}

chunk tlSkeletonJointMirrorMapChunk16 : chunk_id(0x4503)
{
    ULONG MappedJointIndex;
    float XAxisMap;
    float YAxisMap;
    float ZAxisMap;
}

chunk tlSkeletonJointBonePreserveChunk16 : chunk_id(0x4504)
{
    ULONG PreserveBoneLengths;
}

struct tlBoneWeightingData
{
    UWORD FirstVertex;
    UWORD JointIndex;
    float Weight;
}

chunk tlSkinChunk : chunk_id(0x10001) subchunk(0x10020) subchunk(0x10003) subchunk(0x10004) subchunk(0x10018)
{
    string Name;
    ULONG Version;
    string SkeletonName;
    ULONG NumPrimGroups;
}

chunk tlSmartPropChunk : chunk_id(0x8010000) subchunk(0x8010001)
{
    ULONG Version;
    string Name;
    string ObjectFactoryName;
    string Material;
    ULONG MaterialEnum;
    ULONG NumBreakables;
    ULONG RenderingCost;
    ULONG SimulationCost;
    ULONG NumStates;
}

chunk tlSmartPropStateDataChunk : chunk_id(0x8010001) subchunk(0x8010002) subchunk(0x8010003) subchunk(0x8010004) subchunk(0x8010005) subchunk(0x8010006) subchunk(0x8010007) subchunk(0x8010008)
{
    string Name;
    ULONG AutoTransition;
    float OutFrame;
    ULONG OutState;
    ULONG CanSimulate;
    ULONG NumDrawables;
    ULONG NumFrameControllers;
    ULONG NumEvents;
    ULONG NumCallbacks;
    ULONG NumAppliedForces;
    ULONG NumExtraAttributes;
}

chunk tlSmartPropVisibilitiesDataChunk : chunk_id(0x8010002)
{
    string Name;
    ULONG Visible;
}

chunk tlSmartPropFrameControllerDataChunk : chunk_id(0x8010003)
{
    string Name;
    ULONG Cyclic;
    float MinFrame;
    float MaxFrame;
    float RelativeSpeed;
}

chunk tlSmartPropEventDataChunk : chunk_id(0x8010004)
{
    string Name;
    ULONG State;
    ULONG EventEnum;
}

chunk tlSmartPropCallbackDataChunk : chunk_id(0x8010005)
{
    string Name;
    float OnFrame;
    ULONG EventEnum;
}

chunk tlSmartPropAppliedForceDataChunk : chunk_id(0x8010006)
{
    float OnFrame;
    float forceX;
    float forceY;
    float forceZ;
    float posX;
    float posY;
    float posZ;
}

chunk tlSmartPropBreakableDataChunk : chunk_id(0x8010007)
{
    ULONG IsBreakable;
    ULONG Joint;
    float OnFrame;
    float forceX;
    float forceY;
    float forceZ;
    float posX;
    float posY;
    float posZ;
    string Material;
    ULONG MaterialEnum;
}

chunk tlSmartPropExtraAttributeDataChunk : chunk_id(0x8010008)
{
    string ExtraAttribute;
}

chunk tlSpriteChunk : chunk_id(0x1800A) subchunk(0x19001)
{
    string Name;
    ULONG NativeX;
    ULONG NativeY;
    string Shader;
    ULONG ImageWidth;
    ULONG ImageHeight;
    ULONG ImageCount;
    ULONG BlitBorder;
}

chunk tlStatePropChunk : chunk_id(0x8020000) subchunk(0x8020001)
{
    ULONG Version;
    string Name;
    string ObjectFactoryName;
    ULONG NumStates;
}

chunk tlStatePropStateDataChunk : chunk_id(0x8020001) subchunk(0x8020002) subchunk(0x8020003) subchunk(0x8020004) subchunk(0x8020005)
{
    string Name;
    ULONG AutoTransition;
    ULONG OutState;
    ULONG NumDrawables;
    ULONG NumFrameControllers;
    ULONG NumEvents;
    ULONG NumCallbacks;
    float OutFrame;
}

chunk tlStatePropVisibilitiesDataChunk : chunk_id(0x8020002)
{
    string Name;
    ULONG Visible;
}

chunk tlStatePropFrameControllerDataChunk : chunk_id(0x8020003)
{
    string Name;
    ULONG Cyclic;
    ULONG NumberOfCycles;
    ULONG HoldFrame;
    float MinFrame;
    float MaxFrame;
    float RelativeSpeed;
}

chunk tlStatePropEventDataChunk : chunk_id(0x8020004)
{
    string Name;
    ULONG State;
    ULONG EventEnum;
}

chunk tlStatePropCallbackDataChunk : chunk_id(0x8020005)
{
    string Name;
    ULONG EventEnum;
    float OnFrame;
}

chunk tlStaticPhysDSGChunk : chunk_id(0x3F00001) subchunk(0x3000600) subchunk(0x20001)
{
    string Name;
    ULONG Version;
}

chunk tlTerrainTypeChunk : chunk_id(0x300000E)
{
    ULONG Version;
    ULONG NumTypes;
    array(UBYTE, NumTypes) Types;
}

chunk tlTextureAnimChunk16 : chunk_id(0x3520) subchunk(0x3521)
{
    string AnimName;
    ULONG Version;
    string MaterialName;
    ULONG NumFrames;
    float FrameRate;
    ULONG Cyclic;
}

chunk tlTextureAnimChannelChunk16 : chunk_id(0x3521) subchunk(0x42A0)
{
}

chunk tlTextureChunk : chunk_id(0x19000) subchunk(0x19001) subchunk(0x19004)
{
    string Name;
    ULONG Version;
    ULONG Width;
    ULONG Height;
    ULONG Bpp;
    ULONG AlphaDepth;
    ULONG NumMipMaps;
    ULONG TextureType;
    ULONG Usage;
    ULONG Priority;
}

chunk tlTreeDSGChunk : chunk_id(0x3F00004) subchunk(0x3F00005)
{
    ULONG NumNodes;
    tlPoint BoundsMin;
    tlPoint BoundsMax;
}

chunk tlContiguousBinNodeChunk : chunk_id(0x3F00005) subchunk(0x3F00006)
{
    ULONG SubTreeSize;
    ULONG ParentOffset;
}

chunk tlSpatialNodeChunk : chunk_id(0x3F00006)
{
    UBYTE PlaneAxis;
    float PlanePosn;
    ULONG NumSEntities;
    ULONG NumSPhys;
    ULONG NumIntersects;
    ULONG NumDPhys;
    ULONG NumFences;
    ULONG NumRoads;
    ULONG NumPaths;
    ULONG NumAnims;
}

chunk tlVertexAnimKeyChunk : chunk_id(0x121304) subchunk(0x121109) subchunk(0x121301) subchunk(0x121302)
{
    ULONG Version;
    string Name;
}

chunk tlColourOffsetListChunk : chunk_id(0x121109) subchunk(0x121303)
{
    ULONG Version;
    ULONG NumOffsets;
    array(tlColourOffset, NumOffsets) Offsets;
}

chunk tlVectorOffsetListChunk : chunk_id(0x121301) subchunk(0x121303)
{
    ULONG Version;
    ULONG NumOffsets;
    FOURCC Param;
    array(tlPoint, NumOffsets) Offsets;
}

chunk tlVector2OffsetListChunk : chunk_id(0x121302) subchunk(0x121303)
{
    ULONG Version;
    ULONG NumOffsets;
    FOURCC Param;
    array(tlUV, NumOffsets) Offsets;
}

chunk tlOffsetIndexListChunk : chunk_id(0x121303)
{
    ULONG Version;
    ULONG NumIndex;
    array(ULONG, NumIndex) Index;
}

chunk tlVertexOffsetExpressionChunk16 : chunk_id(0x4A82)
{
    string Name;
    ULONG NumKeys;
    array(float, NumKeys) Keys;
    array(ULONG, NumKeys) Indices;
}

chunk tlVisibilityAnimChunk16 : chunk_id(0x4290) subchunk(0x4291)
{
    string AnimName;
    string SceneName;
    ULONG Version;
    ULONG NumFrames;
    float FrameRate;
    ULONG NumChannels;
}

chunk tlVisibilityAnimChannelChunk16 : chunk_id(0x4291)
{
    string Nodename;
    WORD bStartState;
    ULONG NumFrames;
    array(ULONG, NumFrames) Frames;
}

chunk tlWBLocatorChunk : chunk_id(0x14000) subchunk(0x3000006) subchunk(0x3000007)
{
    string Name;
    ULONG Type;
    ULONG NumDataElements;
    array(ULONG, NumDataElements) DataElements;
    tlPoint Position;
    ULONG NumTriggers;
}

chunk tlWBRailCamChunk : chunk_id(0x300000A)
{
    string Name;
    ULONG Behaviour;
    float MinRadius;
    float MaxRadius;
    ULONG TrackRail;
    float TrackDist;
    ULONG ReverseSense;
    float FOV;
    tlPoint TargetOffset;
    tlPoint AxisPlay;
    float PositionLag;
    float TargetLag;
}

chunk tlWBSplineChunk : chunk_id(0x3000007) subchunk(0x300000A)
{
    string Name;
    ULONG NumCVs;
    array(tlPoint, NumCVs) CVs;
}

chunk tlWBTriggerVolumeChunk : chunk_id(0x3000006)
{
    string Name;
    ULONG Type;
    tlPoint Scale;
    tlMatrix Matrix;
}

chunk tlWalkerCamDataChunk : chunk_id(0x3000101)
{
    ULONG ID;
    float MinMagnitude;
    float MaxMagnitude;
    float Elevation;
    tlPoint TargetOffset;
}

chunk tlWallChunk : chunk_id(0x7010005)
{
    tlPoint Start;
    tlPoint End;
    tlPoint Normal;
}

chunk tlWorldSphereDSGChunk : chunk_id(0x3F0000B) subchunk(0x10000) subchunk(0x48A0) subchunk(0x4520) subchunk(0x4500) subchunk(0x20002) subchunk(0x4512) subchunk(0x17002) subchunk(0x3F0000D)
{
    string Name;
    ULONG Version;
    ULONG NumMeshes;
    ULONG NumBillBoardQuads;
}

chunk PrimgroupOld : chunk_id(0x10002)
{
    raw Data;
}

chunk Meshstats : chunk_id(0x1001D)
{
    raw Data;
}

chunk Vertexcompressionhint : chunk_id(0x10021)
{
    raw Data;
}

chunk Deformvertexlist : chunk_id(0x10022)
{
    raw Data;
}

chunk Deformnormallist : chunk_id(0x10023)
{
    raw Data;
}

chunk Vertexanimuv : chunk_id(0x10F01)
{
    raw Data;
}

chunk Vertexanimdata : chunk_id(0x10F02)
{
    raw Data;
}

chunk BillboardQuad : chunk_id(0x17005)
{
    raw Data;
}

chunk BillboardQuadGroup : chunk_id(0x17006)
{
    raw Data;
}

chunk BillboardTransform : chunk_id(0x17007)
{
    raw Data;
}

chunk BillboardDisplayInfo : chunk_id(0x17008)
{
    raw Data;
}

chunk BillboardUvInfo : chunk_id(0x17009)
{
    raw Data;
}

chunk BillboardCutoffSource : chunk_id(0x1700A)
{
    raw Data;
}

chunk BillboardCutoffEdge : chunk_id(0x1700B)
{
    raw Data;
}

chunk BillboardCutoffFalloff : chunk_id(0x1700C)
{
    raw Data;
}

chunk BillboardCutoffRange : chunk_id(0x1700D)
{
    raw Data;
}

chunk Skeleton : chunk_id(0x23000)
{
    raw Data;
}

chunk SkeletonJoint : chunk_id(0x23001)
{
    raw Data;
}

chunk Animation : chunk_id(0x121000)
{
    raw Data;
}

chunk Group : chunk_id(0x121001)
{
    raw Data;
}

chunk SizeList : chunk_id(0x121006)
{
    raw Data;
}

chunk SizeEntry : chunk_id(0x121007)
{
    raw Data;
}

chunk FrameController : chunk_id(0x121201)
{
    raw Data;
}

chunk ControllerList : chunk_id(0x121204)
{
    raw Data;
}

chunk Vertexanim : chunk_id(0x121305)
{
    raw Data;
}

chunk Vertexanimframe : chunk_id(0x121306)
{
    raw Data;
}

chunk Sortkey : chunk_id(0x122000)
{
    raw Data;
}

chunk CompositeDrawable : chunk_id(0x123000)
{
    raw Data;
}

chunk CompositeDrawablePrimitive : chunk_id(0x123001)
{
    raw Data;
}

chunk CharacterLoader : chunk_id(0x8800000)
{
    raw Data;
}

chunk VehicleLoader : chunk_id(0x8800001)
{
    raw Data;
}

chunk SkyLoader : chunk_id(0x8800002)
{
    raw Data;
}

chunk WorldgeoLoader : chunk_id(0x8800003)
{
    raw Data;
}

chunk ZonepkgLoader : chunk_id(0x8800004)
{
    raw Data;
}

chunk SfstatepropLoader : chunk_id(0x8800005)
{
    raw Data;
}

chunk SflightgroupLoader : chunk_id(0x8800007)
{
    raw Data;
}

chunk ShadowLoader : chunk_id(0x8800008)
{
    raw Data;
}

chunk OccluderLoader : chunk_id(0x880000A)
{
    raw Data;
}

chunk Streampackage : chunk_id(0x8800100)
{
    raw Data;
}

chunk Streamtrigger : chunk_id(0x8800101)
{
    raw Data;
}

chunk Scriptobject : chunk_id(0x9900190)
{
    raw Data;
}

chunk Gamegroup : chunk_id(0x9900191)
{
    raw Data;
}

chunk Property : chunk_id(0x9900192)
{
    raw Data;
}

chunk Location : chunk_id(0x9900194)
{
    raw Data;
}
