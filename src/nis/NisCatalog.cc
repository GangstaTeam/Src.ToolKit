#include "nis/NisCatalog.hh"

namespace tk::nis
{

namespace
{

using K = FieldKind;

FieldSpec Frame(const char* key = "FrameBegin", const char* help = "Frame (30 per second) the track fires on.")
{
    return {key, K::Int, Ref::None, "0", help};
}

FieldSpec Text(const char* key, Ref ref, const char* help)
{
    return {key, K::String, ref, "\"\"", help};
}

FieldSpec Vec(const char* key, const char* help)
{
    return {key, K::Vector, Ref::None, "0 0 0", help};
}

std::vector<TrackSpec> BuildTracks()
{
    const char* character = "Character the track drives: MainCharacter or a name loaded by a Resource loader.";
    return {
        {"NISSequenceCue",
            "Sequence cue",
            Category::Camera,
            "Prepares the scene in the init node: camera animation, optional sound and the origin every position is "
            "relative to.",
            {Text("SoundName", Ref::None, "Sound played with the sequence; empty for none."),
                Text("CameraAnimationName",
                    Ref::CameraAnimation,
                    "Camera animation (CAM_...) from the scene's streams."),
                Frame(),
                Vec("Position", "Scene origin."),
                {"Facing", K::Float, Ref::None, "0", "Scene rotation in degrees."}}},
        {"NISSequencePlay",
            "Sequence play",
            Category::Camera,
            "Starts the cued sequence. Goes first in the start node.",
            {}},
        {"NISCameraAnimation",
            "Camera animation",
            Category::Camera,
            "Switches to another camera animation.",
            {Text("CameraAnimationName", Ref::CameraAnimation, "Camera animation (CAM_...).")}},
        {"NISHoldCameraPosition",
            "Hold camera",
            Category::Camera,
            "Holds the camera at a fixed point looking along a direction.",
            {Vec("Position", "Camera position."),
                Vec("Direction", "Look direction."),
                {"WorldSpace", K::Bool, Ref::None, "0", "Position is in world space instead of scene space."}}},
        {"NISResourceLoader",
            "Resource loader",
            Category::Character,
            "Spawns a character for the scene from a character template. Belongs in the init node.",
            {Text("Name", Ref::None, "Name the other tracks use for this character."),
                Text("TemplateName", Ref::CharacterTemplate, "Character template, e.g. CharTemp_Sandman.")}},
        {"NISCharacterPosition",
            "Character position",
            Category::Character,
            "Places a character before it animates.",
            {Text("CharacterName", Ref::Character, character),
                Vec("Position", "World position."),
                {"Facing", K::Float, Ref::None, "0", "Heading in degrees."},
                {"Clut", K::Int, Ref::None, "-1", "Colour variation; -1 keeps the current one."}}},
        {"NISCharacterAnimation",
            "Body animation",
            Category::Character,
            "Plays a body animation (PTRN) on a character.",
            {Text("CharacterName", Ref::Character, character),
                Text("AnimationName", Ref::BodyAnimation, "Body animation from the scene's streams.")}},
        {"NISExpAnimation",
            "Face animation",
            Category::Character,
            "Plays a facial expression animation (EXP_...).",
            {Text("CharacterName", Ref::Character, character),
                Text("AnimationName", Ref::FaceAnimation, "Facial animation (EXP_...)."),
                Text("MixerModelName", Ref::None, "Face mixer of the NIS model, e.g. tonynisHawaiian_Face.")}},
        {"NISSwapModel",
            "Swap model",
            Category::Character,
            "Replaces a character's drawable, e.g. with its high detail NIS model.",
            {Text("Character", Ref::Character, character),
                Text("newModel", Ref::None, "Drawable name, e.g. tonynisHawaiianDrawable.")}},
        {"NISCharacterVisibility",
            "Character visibility",
            Category::Character,
            "Shows or hides a character.",
            {Text("CharacterName", Ref::Character, character), {"IsVisible", K::Bool, Ref::None, "1", ""}, Frame()}},
        {"NISCharacterAttach",
            "Character attach",
            Category::Character,
            "Attaches an object to a character joint, or detaches it.",
            {Text("CharacterName", Ref::Character, character),
                Text("ObjectName", Ref::Object, "Object to attach."),
                Text("CharacterJointName", Ref::Joint, "Joint to attach to."),
                Vec("OffsetPosition", "Offset from the joint."),
                Vec("OffsetDirection", "Rotation offset."),
                {"Detach", K::Bool, Ref::None, "0", "Detach instead of attach."},
                Frame()}},
        {"NISCharacterDamage",
            "Character damage",
            Category::Character,
            "Damages a character.",
            {Text("CharacterName", Ref::Character, character),
                Text("JointName", Ref::Joint, "Joint hit; empty for none."),
                {"Damage", K::Float, Ref::None, "1", "Damage amount."},
                Frame()}},
        {"NISCharacterEffect",
            "Character effect",
            Category::Character,
            "Plays a hit or gore effect on a character.",
            {Text("CharacterName", Ref::Character, character),
                Text("EffectType", Ref::Effect, "Effect to play."),
                Text("JointName", Ref::Joint, "Joint the effect starts at."),
                Text("SourceCharacterName", Ref::Character, "Character the hit comes from."),
                Frame()}},
        {"NISDeleteCharacter",
            "Delete character",
            Category::Character,
            "Removes a character spawned for the scene. Used in the end node.",
            {Text("CharacterName", Ref::Character, character)}},
        {"NISWeaponAttach",
            "Weapon attach",
            Category::Weapon,
            "Puts a weapon in a character's hand; empty template removes it.",
            {Text("CharacterName", Ref::Character, character),
                Text("WeaponTemplateName", Ref::Weapon, "Weapon template."),
                Frame()}},
        {"NISWeaponFire",
            "Weapon fire",
            Category::Weapon,
            "Fires the character's weapon between two frames.",
            {Text("CharacterName", Ref::Character, character),
                {"IsSecondaryWeapon", K::Bool, Ref::None, "0", ""},
                Frame(),
                Frame("FrameEnd", "Frame firing stops.")}},
        {"NISActivateVehicle",
            "Activate vehicle",
            Category::Vehicle,
            "Spawns a vehicle for the scene.",
            {Text("VehicleName", Ref::None, "Name the other tracks use for this vehicle."),
                Text("VehicleType", Ref::VehicleType, "Car, Boat, Helicopter or Plane."),
                {"VehicleClut", K::Int, Ref::None, "0", "Colour variation."}}},
        {"NISVehicleAnimation",
            "Vehicle animation",
            Category::Vehicle,
            "Plays an animation on a vehicle.",
            {Text("VehicleName", Ref::Vehicle, "Vehicle spawned by Activate vehicle."),
                Text("AnimationName", Ref::VehicleAnimation, "Vehicle animation.")}},
        {"NISVehicleProperty",
            "Vehicle property",
            Category::Vehicle,
            "Changes a vehicle's thrust.",
            {Text("VehicleName", Ref::Vehicle, "Vehicle spawned by Activate vehicle."),
                Frame(),
                {"ThrustFactor", K::Float, Ref::None, "1", ""}}},
        {"NISObjectPosition",
            "Object position",
            Category::Prop,
            "Moves a script object.",
            {Frame(),
                Text("ObjectName", Ref::Object, "Object to move."),
                Vec("Position", "World position."),
                {"Facing", K::Float, Ref::None, "0", "Heading in degrees."}}},
        {"NISRenderableTrack",
            "Renderable",
            Category::Prop,
            "Shows a composite drawable as a named renderable.",
            {Text("RenderableName", Ref::None, "Name the other tracks use."),
                Text("CompositeDrawableName", Ref::None, "Composite drawable to show.")}},
        {"NISRenderableAnimationTrack",
            "Renderable animation",
            Category::Prop,
            "Animates a renderable.",
            {Text("RenderableName", Ref::Object, "Renderable to animate."),
                Text("AnimationName", Ref::None, "Animation.")}},
        {"NISStatePropSpawn",
            "State prop spawn",
            Category::Prop,
            "Spawns a state prop.",
            {Text("StatePropName", Ref::None, "Name the other tracks use."),
                Text("StatePropTemplateName", Ref::None, "State prop template."),
                Vec("Position", "World position."),
                Vec("Direction", "Facing direction."),
                Frame()}},
        {"NISStatePropState",
            "State prop state",
            Category::Prop,
            "Changes a state prop's state.",
            {Text("StatePropName", Ref::Object, "State prop."),
                Text("StateName", Ref::None, "State, e.g. idle, open, final."),
                Frame()}},
        {"NISDeleteScriptObject",
            "Delete object",
            Category::Prop,
            "Removes a script object.",
            {Text("Name", Ref::Object, "Object to remove.")}},
        {"NISFade",
            "Screen fade",
            Category::Screen,
            "Fades the screen to a colour and back.",
            {Frame(),
                {"FadeInFrames", K::Int, Ref::None, "0", "Frames to fade to the colour."},
                {"DurationFrames", K::Int, Ref::None, "0", "Frames held at the colour."},
                {"FadeOutFrames", K::Int, Ref::None, "0", "Frames to fade back to the picture."},
                {"Red", K::Int, Ref::None, "0", ""},
                {"Green", K::Int, Ref::None, "0", ""},
                {"Blue", K::Int, Ref::None, "0", ""}}},
        {"SoundFade",
            "Sound mix fade",
            Category::Sound,
            "Fades a sound mix (usually ducking the world for the cutscene) in or out.",
            {{"TimeBegin", K::Int, Ref::None, "0", "Start time in milliseconds.", true},
                {"fadeTime", K::Int, Ref::None, "500", "Fade length in milliseconds.", true},
                {"isFadeIn", K::Bool, Ref::None, "1", ""},
                Text("mixName", Ref::SoundMix, "Mix to fade, e.g. duck_cinematics.")}},
        {"NISScriptFunctionCallTrack",
            "Script call",
            Category::Script,
            "Runs a script statement. Subtitles use SubtitleManager_LocalisedDialogueEvent( ^nis\\\\\\\\Line^, 0, ms "
            ");",
            {Frame(), Text("ScriptFunctionCall", Ref::None, "Script statement. Write ^ for a quote.")}},
        {"sequence",
            "Branch",
            Category::Flow,
            "Moves to another node of the scene when the node finishes, e.g. ../end.",
            {Text("branch", Ref::Branch, "Target node, relative to this one."),
                {"priority", K::Int, Ref::None, "1", ""},
                {"conditionGroup", K::Conditions}}},
        {"NISDriveWorldStreamer",
            "Drive world streamer",
            Category::Flow,
            "Lets the scene camera drive world streaming.",
            {{"Enable", K::Bool, Ref::None, "1", ""}}},
    };
}

}

const std::vector<TrackSpec>& Tracks()
{
    static const std::vector<TrackSpec> tracks = BuildTracks();
    return tracks;
}

const TrackSpec* FindTrack(std::string_view type)
{
    for (const TrackSpec& spec : Tracks())
    {
        if (type == spec.type)
        {
            return &spec;
        }
    }

    return nullptr;
}

const FieldSpec* FindField(const TrackSpec& spec, std::string_view key)
{
    for (const FieldSpec& field : spec.fields)
    {
        if (key == field.key)
        {
            return &field;
        }
    }

    return nullptr;
}

const char* CategoryName(Category category)
{
    switch (category)
    {
    case Category::Camera:
        return "Camera";
    case Category::Character:
        return "Character";
    case Category::Weapon:
        return "Weapon";
    case Category::Vehicle:
        return "Vehicle";
    case Category::Prop:
        return "Prop";
    case Category::Screen:
        return "Screen";
    case Category::Sound:
        return "Sound";
    case Category::Script:
        return "Script";
    case Category::Flow:
        return "Flow";
    }

    return "";
}

namespace
{

fsc::Node MakeField(const FieldSpec& field)
{
    if (field.kind == FieldKind::Conditions)
    {
        return MakeConditionGroup();
    }

    if (field.kind != FieldKind::Vector)
    {
        return fsc::Node::Value(field.key, field.defaultValue);
    }

    fsc::Node vector = fsc::Node::Block(field.key);

    for (const char* axis : {"x", "y", "z"})
    {
        vector.children.push_back(fsc::Node::Value(axis, "0"));
    }

    return vector;
}

fsc::Node MakeFightNode(const std::string& name, std::vector<fsc::Node> tracks)
{
    fsc::Node node = fsc::Node::Block("fightNode");
    node.SetString("name", name);
    node.SetInt("trackCount", static_cast<long long>(tracks.size()));
    node.SetInt("branchCount", 0);
    node.children.push_back(MakeConditionGroup());

    for (fsc::Node& track : tracks)
    {
        node.children.push_back(std::move(track));
    }

    return node;
}

fsc::Node MakeBank(const std::string& name, std::vector<fsc::Node> branches)
{
    fsc::Node bank = fsc::Node::Block("fightBank");
    bank.SetString("name", name);
    bank.SetInt("branchCount", static_cast<long long>(branches.size()));
    bank.children.push_back(MakeConditionGroup());

    for (fsc::Node& branch : branches)
    {
        bank.children.push_back(std::move(branch));
    }

    return bank;
}

fsc::Node Track(const char* type, std::initializer_list<std::pair<const char*, std::string>> values)
{
    fsc::Node track = MakeTrack(*FindTrack(type));

    for (const auto& [key, raw] : values)
    {
        track.SetRaw(key, raw);
    }

    return track;
}

}

fsc::Node MakeTrack(const TrackSpec& spec)
{
    fsc::Node track = fsc::Node::Block(spec.type);

    for (const FieldSpec& field : spec.fields)
    {
        track.children.push_back(MakeField(field));
    }

    return track;
}

fsc::Node& InsertField(fsc::Node& track, const TrackSpec& spec, const FieldSpec& field)
{
    size_t at = 0;

    for (const FieldSpec& other : spec.fields)
    {
        if (&other == &field)
        {
            break;
        }

        for (size_t i = 0; i < track.children.size(); ++i)
        {
            if (track.children[i].key == other.key)
            {
                at = i + 1;
            }
        }
    }

    auto inserted = track.children.insert(track.children.begin() + static_cast<std::ptrdiff_t>(at), MakeField(field));
    return *inserted;
}

fsc::Node MakeConditionGroup()
{
    fsc::Node group = fsc::Node::Block("conditionGroup");
    group.SetInt("conditionCount", 0);
    return group;
}

const std::vector<std::string>& KnownValues(Ref ref)
{
    static const std::vector<std::string> none;
    static const std::vector<std::string> joints = {"Motion_Root",
        "Balance_Root",
        "Character_Root",
        "Pelvis",
        "Hip_L",
        "Knee_L",
        "Ankle_L",
        "Ball_L",
        "Hip_R",
        "Knee_R",
        "Ankle_R",
        "Ball_R",
        "Spine_1",
        "Spine_2",
        "Neck",
        "Head",
        "Jaw",
        "Clavicle_L",
        "Shoulder_L",
        "Elbow_L",
        "Forarm_L",
        "Wrist_L",
        "Clavicle_R",
        "Shoulder_R",
        "Elbow_R",
        "Forarm_R",
        "Wrist_R",
        "right_hand_attach",
        "left_hand_attach"};
    static const std::vector<std::string> weapons = {"",
        "WeaponTemplateAK47",
        "WeaponTemplateUzi",
        "WeaponTemplateColt45",
        "WeaponTemplateM16",
        "WeaponTemplateSniper",
        "WeaponTemplateM16Mortar",
        "WeaponTemplateMetalPipe"};
    static const std::vector<std::string> effects =
        {"ShotEffectHeavy", "ShotEffectLight", "Headshot", "GoreBloodExplosion", "BloodPool", "GrenadeExplosion"};
    static const std::vector<std::string> vehicles = {"Car", "Boat", "Helicopter", "Plane"};
    static const std::vector<std::string> mixes = {"duck_cinematics", "duck_owned", "duck_intro_cam"};

    switch (ref)
    {
    case Ref::Joint:
        return joints;
    case Ref::Weapon:
        return weapons;
    case Ref::Effect:
        return effects;
    case Ref::VehicleType:
        return vehicles;
    case Ref::SoundMix:
        return mixes;
    default:
        return none;
    }
}

std::vector<fsc::Node> MakeSceneTemplate(const std::string& name)
{
    std::string clear = fsc::Quote("SubtitleManager_ClearDialogueEvents();");
    std::vector<fsc::Node> init;
    init.push_back(Track("NISSequenceCue", {{"CameraAnimationName", fsc::Quote("CAM_" + name + "_Cam")}}));

    std::vector<fsc::Node> start;
    start.push_back(Track("NISSequencePlay", {}));
    start.push_back(Track("NISFade", {{"FadeOutFrames", "15"}}));
    start.push_back(Track("SoundFade", {{"mixName", fsc::Quote("duck_cinematics")}}));
    start.push_back(Track("sequence", {{"branch", fsc::Quote("../end")}}));

    std::vector<fsc::Node> end;
    end.push_back(Track("SoundFade", {{"isFadeIn", "0"}, {"mixName", fsc::Quote("duck_cinematics")}}));
    end.push_back(Track("NISScriptFunctionCallTrack", {{"ScriptFunctionCall", clear}}));

    std::vector<fsc::Node> nodes;
    nodes.push_back(MakeFightNode("init", std::move(init)));
    nodes.push_back(MakeFightNode("start", std::move(start)));
    nodes.push_back(MakeFightNode("end", std::move(end)));
    std::vector<fsc::Node> inner;
    inner.push_back(MakeBank(name, std::move(nodes)));
    std::vector<fsc::Node> roots;
    roots.push_back(MakeBank(name, std::move(inner)));
    return roots;
}

}
