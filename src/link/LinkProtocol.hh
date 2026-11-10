#pragma once

#include <cstdint>

#define LINKPROTOCOL_PIPE_NAME L"\\\\.\\pipe\\ScarfaceToolKitLink"

namespace tk::link
{

enum ELinkProtocolByte : uint8_t
{
    kNodeVisible = 1,
    kNodeShown = 2,
    kNodeEdited = 4,
    kNodeHasBounds = 8,
};

enum ELinkProtocolValue : uint32_t
{
    kVersion = 1,
    kMaxScript = 4096,
};

enum class Message : uint32_t
{
    Hello = 1,
    Snapshot = 2,
    Names = 3,
    Pick = 4,
    Player = 5,
    Select = 100,
    SetField = 101,
    Revert = 102,
    PickMode = 103,
    Highlight = 104,
    RunScript = 105,
};

enum class NodeType : uint8_t
{
    Project,
    Screen,
    Page,
    Layer,
    Group,
    Sprite,
    Text,
    Pure3dObject,
    Polygon,
    RoundedRectangle,
    Unknown
};

enum class Field : uint8_t
{
    Visible,
    Alpha,
    X,
    Y,
    Colour
};

#pragma pack(push, 1)

struct Header
{
    uint32_t size;
    uint32_t type;
};

struct HelloRecord
{
    uint32_t version;
    uint32_t pid;
};

struct SnapshotHeader
{
    uint32_t frame;
    uint32_t count;
};

struct NodeRecord
{
    uint32_t id;
    uint32_t parent;
    uint32_t key;
    uint8_t type;
    uint8_t project;
    uint8_t flags;
    uint8_t alpha;
    uint32_t colour;
    float x, y;
    float offsetX, offsetY;
    int16_t width, height;
    uint8_t justification;
    uint8_t pad[3];
    int32_t bounds[4];
};

struct PickRecord
{
    float x, y;
};

struct IdRecord
{
    uint32_t id;
};

struct FieldRecord
{
    uint32_t id;
    uint8_t field;
    uint8_t pad[3];
    uint32_t value;
};

struct FlagRecord
{
    uint8_t on;
};

struct PlayerRecord
{
    float x, y, z;
    float dirX, dirY, dirZ;
    char nisModel[32];
};

#pragma pack(pop)

static_assert(sizeof(NodeRecord) == 60, "NodeRecord layout");

}
