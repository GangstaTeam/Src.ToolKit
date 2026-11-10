#include "rig/PackageRename.hh"

#include <cctype>
#include <cstring>

namespace tk
{

namespace
{

size_t PaddedTerminated(size_t length)
{
    size_t padded = length + 1;

    while ((padded + 1) % 4 != 0)
    {
        ++padded;
    }

    return padded;
}

size_t PaddedPlain(size_t length)
{
    return (length + 3) / 4 * 4;
}

bool MatchesAt(const Bytes& payload, size_t at, const std::string& name, size_t& stored)
{
    stored = payload[at];

    if (stored < name.size() || at + 1 + stored > payload.size())
    {
        return false;
    }

    if (stored != PaddedTerminated(name.size()) && stored != PaddedPlain(name.size()))
    {
        return false;
    }

    if (std::memcmp(payload.data() + at + 1, name.data(), name.size()) != 0)
    {
        return false;
    }

    for (size_t i = name.size(); i < stored; ++i)
    {
        if (payload[at + 1 + i] != 0)
        {
            return false;
        }
    }

    return true;
}

Bytes Encode(const std::string& name, bool terminated)
{
    size_t stored = terminated ? PaddedTerminated(name.size()) : PaddedPlain(name.size());
    Bytes out(1 + stored, 0);
    out[0] = static_cast<uint8_t>(stored);
    std::memcpy(out.data() + 1, name.data(), name.size());
    return out;
}

size_t RenameInPayload(Chunk& chunk, const std::string& from, const std::string& to)
{
    const Bytes& payload = chunk.Payload();
    Bytes out;
    size_t count = 0;

    for (size_t at = 0; at < payload.size();)
    {
        size_t stored = 0;

        if (MatchesAt(payload, at, from, stored))
        {
            Bytes encoded = Encode(to, stored == PaddedTerminated(from.size()));
            out.insert(out.end(), encoded.begin(), encoded.end());
            at += 1 + stored;
            ++count;
        }
        else
        {
            out.push_back(payload[at++]);
        }
    }

    if (count)
    {
        chunk.SetPayload(std::move(out));
    }

    return count;
}

}

size_t RenamePackage(Chunk& root, const std::string& from, const std::string& to)
{
    size_t count = RenameInPayload(root, from, to);

    for (size_t i = 0; i < root.ChildCount(); ++i)
    {
        count += RenamePackage(*root.Child(i), from, to);
    }

    return count;
}

bool IsValidPackageName(const std::string& name)
{
    if (name.empty() || name.size() > 64)
    {
        return false;
    }

    for (char c : name)
    {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
        {
            return false;
        }
    }

    return true;
}

}
