#pragma once

#include "nis/FscTree.hh"

#include <string>
#include <string_view>
#include <vector>

namespace tk::nis
{

enum class FieldKind
{
    String,
    Int,
    Float,
    Bool,
    Vector,
    Conditions
};

enum class Ref
{
    None,
    Character,
    Vehicle,
    Object,
    BodyAnimation,
    FaceAnimation,
    CameraAnimation,
    VehicleAnimation,
    Joint,
    Weapon,
    Effect,
    VehicleType,
    SoundMix,
    Branch,
    CharacterTemplate,
};

enum class Category
{
    Camera,
    Character,
    Weapon,
    Vehicle,
    Prop,
    Screen,
    Sound,
    Script,
    Flow
};

struct FieldSpec
{
    const char* key;
    FieldKind kind;
    Ref ref = Ref::None;
    const char* defaultValue = "";
    const char* help = "";
    bool milliseconds = false;
};

struct TrackSpec
{
    const char* type;
    const char* label;
    Category category;
    const char* help;
    std::vector<FieldSpec> fields;
};

const std::vector<TrackSpec>& Tracks();
const TrackSpec* FindTrack(std::string_view type);
const FieldSpec* FindField(const TrackSpec& spec, std::string_view key);
const char* CategoryName(Category category);

fsc::Node MakeTrack(const TrackSpec& spec);
fsc::Node& InsertField(fsc::Node& track, const TrackSpec& spec, const FieldSpec& field);
fsc::Node MakeConditionGroup();

const std::vector<std::string>& KnownValues(Ref ref);

std::vector<fsc::Node> MakeSceneTemplate(const std::string& name);

}
