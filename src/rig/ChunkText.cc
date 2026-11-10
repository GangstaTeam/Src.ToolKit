#include "rig/ChunkText.hh"

#include <cstring>

namespace tk
{

std::string ReadName(ByteReader& reader)
{
    Bytes chars = reader.Take(reader.U8());
    std::string name(chars.begin(), chars.end());
    name.resize(std::strlen(name.c_str()));
    return name;
}

std::string LeadingName(const Chunk& chunk)
{
    ByteReader reader(chunk.Payload());
    return ReadName(reader);
}

}
