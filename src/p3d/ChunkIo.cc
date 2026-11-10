#include "p3d/ChunkIo.hh"

#include "core/ByteReader.hh"
#include "core/ByteWriter.hh"

namespace tk
{

namespace
{

struct Header
{
    uint32_t id = 0;
    uint32_t dataLength = 0;
    uint32_t chunkLength = 0;
};

bool ReadHeader(const Bytes& bytes, size_t offset, size_t end, Header& header)
{
    if (end - offset < Chunk::kHeaderSize)
    {
        return false;
    }

    ByteReader reader(bytes.data() + offset, Chunk::kHeaderSize);
    header.id = reader.U32();
    header.dataLength = reader.U32();
    header.chunkLength = reader.U32();
    return header.dataLength >= Chunk::kHeaderSize && header.chunkLength >= header.dataLength &&
        header.chunkLength <= end - offset;
}

std::unique_ptr<Chunk> ParseChunk(const Bytes& bytes, size_t offset, const Header& header)
{
    auto first = bytes.begin() + static_cast<std::ptrdiff_t>(offset);
    auto chunk = std::make_unique<Chunk>(header.id, Bytes(first + Chunk::kHeaderSize, first + header.dataLength));
    size_t position = offset + header.dataLength;
    size_t end = offset + header.chunkLength;
    Header child;

    while (position < end && ReadHeader(bytes, position, end, child))
    {
        chunk->AppendChild(ParseChunk(bytes, position, child));
        position += child.chunkLength;
    }

    if (position < end)
    {
        auto from = bytes.begin() + static_cast<std::ptrdiff_t>(position);
        chunk->SetTrailing(Bytes(from, bytes.begin() + static_cast<std::ptrdiff_t>(end)));
    }

    return chunk;
}

void WriteChunk(const Chunk& chunk, ByteWriter& writer)
{
    writer.U32(chunk.Id());
    writer.U32(static_cast<uint32_t>(chunk.DataSize()));
    writer.U32(static_cast<uint32_t>(chunk.TotalSize()));
    writer.Append(chunk.Payload());

    for (size_t i = 0; i < chunk.ChildCount(); ++i)
    {
        WriteChunk(*chunk.Child(i), writer);
    }

    writer.Append(chunk.Trailing());
}

}

ParseResult ParseP3d(const Bytes& bytes)
{
    Header header;

    if (bytes.size() < Chunk::kHeaderSize)
    {
        return {nullptr, "file too small"};
    }

    ByteReader reader(bytes);
    uint32_t magic = reader.U32();

    if (magic == kP3dCompressedId)
    {
        return {nullptr, "compressed P3D files are not supported yet"};
    }

    if (magic != kP3dFileId)
    {
        return {nullptr, "not a P3D file"};
    }

    if (!ReadHeader(bytes, 0, bytes.size(), header))
    {
        return {nullptr, "corrupt file header"};
    }

    return {ParseChunk(bytes, 0, header), {}};
}

Bytes SerializeP3d(const Chunk& root)
{
    ByteWriter writer;
    WriteChunk(root, writer);
    return writer.Take();
}

std::unique_ptr<Chunk> MakeEmptyFile()
{
    return std::make_unique<Chunk>(kP3dFileId);
}

}
