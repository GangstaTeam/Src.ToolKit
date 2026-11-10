#pragma once

#include "core/Bytes.hh"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace tk
{

class Chunk
{
public:
    enum EHeader : size_t
    {
        kHeaderSize = 12,
    };

    explicit Chunk(uint32_t id = 0, Bytes payload = {});

    uint32_t Id() const { return m_id; }
    void SetId(uint32_t id) { m_id = id; }

    const Bytes& Payload() const { return m_payload; }
    void SetPayload(Bytes payload) { m_payload = std::move(payload); }

    const Bytes& Trailing() const { return m_trailing; }
    void SetTrailing(Bytes trailing) { m_trailing = std::move(trailing); }

    Chunk* Parent() const { return m_parent; }
    size_t ChildCount() const { return m_children.size(); }
    Chunk* Child(size_t index) const { return m_children[index].get(); }
    size_t IndexInParent() const;

    Chunk* InsertChild(std::unique_ptr<Chunk> child, size_t index);
    Chunk* AppendChild(std::unique_ptr<Chunk> child);
    std::unique_ptr<Chunk> RemoveChild(size_t index);

    std::unique_ptr<Chunk> Clone() const;
    size_t DataSize() const;
    size_t TotalSize() const;
    bool IsAncestorOf(const Chunk* other) const;

private:
    uint32_t m_id;
    Bytes m_payload;
    Bytes m_trailing;
    Chunk* m_parent = nullptr;
    std::vector<std::unique_ptr<Chunk>> m_children;
};

}
