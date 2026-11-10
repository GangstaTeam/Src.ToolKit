#include "render/MeshExtractor.hh"

#include "core/ByteReader.hh"
#include "p3d/ChunkIds.hh"

#include <cstring>

namespace tk
{

namespace
{

struct PrimGroupHeader
{
    std::string shader;
    uint32_t primitiveType = 0;
};

PrimGroupHeader ReadHeader(const Chunk& primGroup)
{
    ByteReader reader(primGroup.Payload());
    reader.U32();
    Bytes name = reader.Take(reader.U8());
    PrimGroupHeader header;
    header.shader.assign(name.begin(), name.end());
    header.shader.resize(std::strlen(header.shader.c_str()));
    header.primitiveType = reader.U32();
    return header;
}

std::vector<float> ReadFloats(const Chunk& list, size_t perItem, size_t skip)
{
    ByteReader reader(list.Payload());
    uint32_t count = reader.U32();

    for (size_t i = 0; i < skip; ++i)
    {
        reader.U32();
    }

    std::vector<float> values(static_cast<size_t>(count) * perItem);

    for (float& value : values)
    {
        value = reader.F32();
    }

    return reader.Failed() ? std::vector<float>() : values;
}

std::vector<uint32_t> ReadU32s(const Chunk& list)
{
    ByteReader reader(list.Payload());
    std::vector<uint32_t> values(reader.U32());

    for (uint32_t& value : values)
    {
        value = reader.U32();
    }

    return reader.Failed() ? std::vector<uint32_t>() : values;
}

const Chunk* FindChild(const Chunk& parent, uint32_t id)
{
    for (size_t i = 0; i < parent.ChildCount(); ++i)
    {
        if (parent.Child(i)->Id() == id)
        {
            return parent.Child(i);
        }
    }

    return nullptr;
}

void FillVertices(const Chunk& primGroup, MeshPart& part, Bounds& bounds)
{
    const Chunk* positionList = FindChild(primGroup, chunk::Positions);

    if (!positionList)
    {
        return;
    }

    std::vector<float> positions = ReadFloats(*positionList, 3, 0);
    const Chunk* normalList = FindChild(primGroup, chunk::Normals);
    const Chunk* uvList = FindChild(primGroup, chunk::Uvs);
    const Chunk* colourList = FindChild(primGroup, chunk::Colours);
    std::vector<float> normals = normalList ? ReadFloats(*normalList, 3, 0) : std::vector<float>();
    std::vector<float> uvs = uvList ? ReadFloats(*uvList, 2, 1) : std::vector<float>();
    std::vector<uint32_t> colours = colourList ? ReadU32s(*colourList) : std::vector<uint32_t>();
    size_t count = positions.size() / 3;
    part.vertices.resize(count);

    for (size_t i = 0; i < count; ++i)
    {
        MeshVertex& v = part.vertices[i];
        v = {positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2], 0.0f, 1.0f, 0.0f, 0xFFFFFFFFu, 0.0f, 0.0f};

        if (normals.size() >= (i + 1) * 3)
        {
            std::memcpy(&v.nx, &normals[i * 3], 12);
        }

        if (uvs.size() >= (i + 1) * 2)
        {
            v.u = uvs[i * 2];
            v.v = 1.0f - uvs[i * 2 + 1];
        }

        if (i < colours.size())
        {
            v.colour = colours[i];
        }

        bounds.Add(v.x, v.y, v.z);
    }
}

void FillIndices(const Chunk& primGroup, MeshPart& part)
{
    if (const Chunk* indexList = FindChild(primGroup, chunk::Indices))
    {
        part.indices = ReadU32s(*indexList);
    }

    if (part.indices.empty() && !FindChild(primGroup, chunk::Strips))
    {
        for (uint32_t i = 0; i < part.vertices.size(); ++i)
        {
            part.indices.push_back(i);
        }
    }

    for (uint32_t& index : part.indices)
    {
        if (index >= part.vertices.size())
        {
            index = 0;
        }
    }
}

std::vector<uint32_t> StripToList(const std::vector<uint32_t>& strip)
{
    std::vector<uint32_t> list;

    for (size_t i = 2; i < strip.size(); ++i)
    {
        uint32_t a = strip[i - 2], b = strip[i - 1], c = strip[i];

        if (a == b || b == c || a == c)
        {
            continue;
        }

        bool flip = (i % 2) == 1;
        list.insert(list.end(), {a, flip ? c : b, flip ? b : c});
    }

    return list;
}

void Collect(const Chunk& chunk, MeshScene& scene)
{
    if (chunk.Id() == chunk::PrimGroup)
    {
        MeshPart part;
        PrimGroupHeader header = ReadHeader(chunk);
        part.shader = header.shader;
        FillVertices(chunk, part, scene.bounds);
        FillIndices(chunk, part);

        if (header.primitiveType == kPrimTriangleStrip)
        {
            part.indices = StripToList(part.indices);
        }

        scene.triangles += part.indices.size() / 3;

        if (!part.vertices.empty() && !part.indices.empty())
        {
            scene.parts.push_back(std::move(part));
        }

        return;
    }

    for (size_t i = 0; i < chunk.ChildCount(); ++i)
    {
        Collect(*chunk.Child(i), scene);
    }
}

}

MeshScene ExtractMeshes(const std::vector<const Chunk*>& sources)
{
    MeshScene scene;

    for (const Chunk* source : sources)
    {
        Collect(*source, scene);
    }

    return scene;
}

}
