#include "render/SkeletonPose.hh"

#include "core/ByteReader.hh"
#include "p3d/ChunkIds.hh"

#include <cstring>

namespace tk
{

namespace
{

struct Matrix
{
    float m[4][4];
};

Matrix Multiply(const Matrix& a, const Matrix& b)
{
    Matrix out = {};

    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            for (int k = 0; k < 4; ++k)
            {
                out.m[r][c] += a.m[r][k] * b.m[k][c];
            }
        }
    }

    return out;
}

std::string ReadName(ByteReader& reader)
{
    Bytes chars = reader.Take(reader.U8());
    std::string name(chars.begin(), chars.end());
    name.resize(std::strlen(name.c_str()));
    return name;
}

}

std::vector<JointPose> ExtractSkeleton(Chunk& skeleton)
{
    std::vector<JointPose> joints;
    std::vector<Matrix> worlds;

    for (size_t i = 0; i < skeleton.ChildCount(); ++i)
    {
        Chunk* chunk = skeleton.Child(i);

        if (chunk->Id() != chunk::SkeletonJoint)
        {
            continue;
        }

        ByteReader reader(chunk->Payload());
        JointPose joint;
        joint.name = ReadName(reader);
        joint.parent = static_cast<int>(reader.U32());
        joint.chunk = chunk;
        Matrix local;

        for (auto& row : local.m)
        {
            for (float& value : row)
            {
                value = reader.F32();
            }
        }

        bool hasParent = !joints.empty() && joint.parent >= 0 && joint.parent < static_cast<int>(worlds.size()) &&
            joint.parent != static_cast<int>(joints.size());
        Matrix world = hasParent ? Multiply(local, worlds[static_cast<size_t>(joint.parent)]) : local;

        if (!hasParent)
        {
            joint.parent = -1;
        }

        joint.position = {world.m[3][0], world.m[3][1], world.m[3][2]};

        for (int a = 0; a < 3; ++a)
        {
            joint.axes[a] = Normalize({world.m[a][0], world.m[a][1], world.m[a][2]});
        }

        worlds.push_back(world);
        joints.push_back(joint);
    }

    return joints;
}

}
