#include "rig/BoneMapping.hh"

#include <cctype>

namespace tk
{

namespace
{

struct Rule
{
    const char* keyword;
    const char* joint;
};

const Rule kCentreRules[] = {
    {"hips", "Pelvis"},
    {"pelvis", "Pelvis"},
    {"chest", "Spine_2"},
    {"neck", "Neck"},
    {"headtop", ""},
    {"headend", ""},
    {"head", "Head"},
};

const Rule kSideRules[] = {
    {"thumb", ""},
    {"index", ""},
    {"middle", ""},
    {"ring", ""},
    {"pinky", ""},
    {"finger", ""},
    {"toe", ""},
    {"shoulder", "Clavicle"},
    {"clavicle", "Clavicle"},
    {"collar", "Clavicle"},
    {"forearm", "Elbow"},
    {"lowerarm", "Elbow"},
    {"elbow", "Elbow"},
    {"upperarm", "Shoulder"},
    {"arm", "Shoulder"},
    {"hand", "Wrist"},
    {"wrist", "Wrist"},
    {"upleg", "Hip"},
    {"thigh", "Hip"},
    {"upperleg", "Hip"},
    {"hip", "Hip"},
    {"lowerleg", "Knee"},
    {"calf", "Knee"},
    {"shin", "Knee"},
    {"knee", "Knee"},
    {"leg", "Knee"},
    {"foot", "Ankle"},
    {"ankle", "Ankle"},
};

const char* const kNoise[] = {"mixamorig", "bip001", "bip01", "def", "org"};

struct ParsedName
{
    std::string core;
    char side = 0;
};

ParsedName Parse(const std::string& name)
{
    std::string lower;

    for (char c : name.substr(name.rfind(':') == std::string::npos ? 0 : name.rfind(':') + 1))
    {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    ParsedName parsed;
    std::string token;
    auto flush = [&]
    {
        if (token == "l" || token == "left")
        {
            parsed.side = 'L';
        }
        else if (token == "r" || token == "right")
        {
            parsed.side = 'R';
        }
        else
        {
            parsed.core += token;
        }

        token.clear();
    };

    for (char c : lower)
    {
        if (std::isalnum(static_cast<unsigned char>(c)))
        {
            token += c;
        }
        else
        {
            flush();
        }
    }

    flush();

    for (const char* side : {"left", "right"})
    {
        size_t at = parsed.core.find(side);

        if (at == std::string::npos)
        {
            continue;
        }

        parsed.side = side[0] == 'l' ? 'L' : 'R';
        parsed.core.erase(at, std::char_traits<char>::length(side));
    }

    for (const char* noise : kNoise)
    {
        size_t at = parsed.core.find(noise);

        if (at != std::string::npos)
        {
            parsed.core.erase(at, std::char_traits<char>::length(noise));
        }
    }

    return parsed;
}

std::string SpineJoint(const std::string& core)
{
    if (core.rfind("spine", 0) != 0)
    {
        return "-";
    }

    std::string rest = core.substr(5);
    return rest.empty() || rest == "0" || rest == "00" || rest == "000" ? "Spine_1" : "Spine_2";
}

std::string GuessJoint(const std::string& name)
{
    ParsedName parsed = Parse(name);

    if (!parsed.side)
    {
        std::string spine = SpineJoint(parsed.core);

        if (spine != "-")
        {
            return spine;
        }

        for (const Rule& rule : kCentreRules)
        {
            if (parsed.core.find(rule.keyword) != std::string::npos)
            {
                return rule.joint;
            }
        }

        return "";
    }

    for (const Rule& rule : kSideRules)
    {
        if (parsed.core.find(rule.keyword) == std::string::npos)
        {
            continue;
        }

        return *rule.joint ? std::string(rule.joint) + "_" + parsed.side : "";
    }

    return "";
}

}

const std::vector<std::string>& MappableJoints()
{
    static const std::vector<std::string> joints = {
        "Pelvis",
        "Spine_1",
        "Spine_2",
        "Neck",
        "Head",
        "Clavicle_L",
        "Shoulder_L",
        "Elbow_L",
        "Wrist_L",
        "Clavicle_R",
        "Shoulder_R",
        "Elbow_R",
        "Wrist_R",
        "Hip_L",
        "Knee_L",
        "Ankle_L",
        "Hip_R",
        "Knee_R",
        "Ankle_R",
    };
    return joints;
}

std::vector<std::string> GuessBoneMapping(const std::vector<SourceBone>& bones)
{
    std::vector<std::string> mapping;

    for (const SourceBone& bone : bones)
    {
        mapping.push_back(GuessJoint(bone.name));
    }

    return mapping;
}

std::vector<std::string> ResolveMapping(const std::vector<SourceBone>& bones, const std::vector<std::string>& mapping)
{
    std::vector<std::string> resolved(bones.size());

    for (size_t i = 0; i < bones.size(); ++i)
    {
        int bone = static_cast<int>(i);

        for (int guard = 0; bone >= 0 && guard < 1000; ++guard)
        {
            size_t b = static_cast<size_t>(bone);

            if (b < mapping.size() && !mapping[b].empty())
            {
                resolved[i] = mapping[b];
                break;
            }

            bone = bones[b].parent;
        }

        if (resolved[i].empty())
        {
            resolved[i] = "Pelvis";
        }
    }

    return resolved;
}

}
