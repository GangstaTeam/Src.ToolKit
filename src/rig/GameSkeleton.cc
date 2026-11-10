#include "rig/GameSkeleton.hh"

#include "core/ByteReader.hh"
#include "p3d/ChunkIds.hh"
#include "rig/ChunkText.hh"
#include "rig/DefaultSkeleton.hh"

namespace tk
{

namespace
{

bool IsRoot(const GameSkeleton& skeleton, size_t index)
{
    int parent = skeleton[index].parent;
    return index == 0 || parent < 0 || static_cast<size_t>(parent) >= index;
}

int BoneChild(const GameSkeleton& skeleton, size_t index)
{
    static const std::pair<const char*, const char*> kBones[] = {
        {"Clavicle", "Shoulder"},
        {"Shoulder", "Elbow"},
        {"Elbow", "Forarm"},
        {"Forarm", "Wrist"},
        {"Hip", "Knee"},
        {"Knee", "Ankle"},
    };
    const std::string& name = skeleton[index].name;

    if (name.size() < 3 || name[name.size() - 2] != '_')
    {
        return -1;
    }

    std::string family = name.substr(0, name.size() - 2), side = name.substr(name.size() - 2);

    for (const auto& [from, to] : kBones)
    {
        if (family == from)
        {
            return FindJoint(skeleton, to + side);
        }
    }

    return -1;
}

}

GameSkeleton DefaultGameSkeleton()
{
    GameSkeleton skeleton;

    for (const DefaultJoint& source : DefaultSkeletonJoints())
    {
        GameJoint joint{source.name, source.parent, {}};

        for (int i = 0; i < 16; ++i)
        {
            joint.local.m[i / 4][i % 4] = source.local[i];
        }

        skeleton.push_back(joint);
    }

    return skeleton;
}

GameSkeleton ReadGameSkeleton(const Chunk& skeleton)
{
    GameSkeleton joints;

    for (size_t i = 0; i < skeleton.ChildCount(); ++i)
    {
        const Chunk* chunk = skeleton.Child(i);

        if (chunk->Id() != chunk::SkeletonJoint)
        {
            continue;
        }

        ByteReader reader(chunk->Payload());
        GameJoint joint;
        joint.name = ReadName(reader);
        joint.parent = static_cast<int>(reader.U32());

        for (auto& row : joint.local.m)
        {
            for (float& value : row)
            {
                value = reader.F32();
            }
        }

        joints.push_back(joint);
    }

    return joints;
}

std::vector<Mat4> WorldMatrices(const GameSkeleton& skeleton)
{
    std::vector<Mat4> worlds;

    for (size_t i = 0; i < skeleton.size(); ++i)
    {
        worlds.push_back(IsRoot(skeleton, i) ? skeleton[i].local
                                             : skeleton[i].local * worlds[static_cast<size_t>(skeleton[i].parent)]);
    }

    return worlds;
}

int FindJoint(const GameSkeleton& skeleton, const std::string& name)
{
    for (size_t i = 0; i < skeleton.size(); ++i)
    {
        if (skeleton[i].name == name)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

GameSkeleton FitSkeleton(const GameSkeleton& base, const JointTargets& targets, float height)
{
    std::vector<Mat4> baseWorld = WorldMatrices(base);
    float scale = height / 1.75f;
    std::vector<Vec3> positions;

    for (size_t i = 0; i < base.size(); ++i)
    {
        auto target = targets.find(base[i].name);

        if (target != targets.end())
        {
            positions.push_back(target->second);
        }
        else if (!IsRoot(base, i) && targets.count(base[static_cast<size_t>(base[i].parent)].name))
        {
            size_t parent = static_cast<size_t>(base[i].parent);
            positions.push_back(
                positions[parent] + (baseWorld[i].Translation() - baseWorld[parent].Translation()) * scale);
        }
        else
        {
            positions.push_back(baseWorld[i].Translation() * scale);
        }
    }

    std::vector<Mat4> turn(base.size());
    std::vector<Mat4> rotations;

    for (size_t i = 0; i < base.size(); ++i)
    {
        bool root = IsRoot(base, i);

        if (!root)
        {
            turn[i] = turn[static_cast<size_t>(base[i].parent)];
        }

        int child = BoneChild(base, i);

        if (child >= 0)
        {
            Vec3 before = baseWorld[static_cast<size_t>(child)].Translation() - baseWorld[i].Translation();
            Vec3 after = positions[static_cast<size_t>(child)] - positions[i];

            if (Length(before) > 1e-6f && Length(after) > 1e-6f)
            {
                turn[i] = RotationBetween(Normalize(before), Normalize(after));
            }
        }

        Mat4 world = RotationOnly(baseWorld[i]) * turn[i];
        rotations.push_back(world);
    }

    GameSkeleton fitted = base;

    for (size_t i = 0; i < base.size(); ++i)
    {
        if (IsRoot(base, i))
        {
            fitted[i].local = rotations[i];
            fitted[i].local.SetTranslation(positions[i]);
        }
        else
        {
            size_t parent = static_cast<size_t>(base[i].parent);
            fitted[i].local = rotations[i] * InverseRigid(rotations[parent]);
            fitted[i].local.SetTranslation(TransformByTranspose(positions[i] - positions[parent], rotations[parent]));
        }
    }

    return fitted;
}

}
