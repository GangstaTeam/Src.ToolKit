#include "p3d/Chunk.hh"

namespace tk
{

Chunk::Chunk(uint32_t id, Bytes payload) : m_id(id), m_payload(std::move(payload))
{
}

size_t Chunk::IndexInParent() const
{
    if (!m_parent)
    {
        return 0;
    }

    for (size_t i = 0; i < m_parent->m_children.size(); ++i)
    {
        if (m_parent->m_children[i].get() == this)
        {
            return i;
        }
    }

    return 0;
}

Chunk* Chunk::InsertChild(std::unique_ptr<Chunk> child, size_t index)
{
    if (index > m_children.size())
    {
        index = m_children.size();
    }

    child->m_parent = this;
    Chunk* raw = child.get();
    m_children.insert(m_children.begin() + static_cast<std::ptrdiff_t>(index), std::move(child));
    return raw;
}

Chunk* Chunk::AppendChild(std::unique_ptr<Chunk> child)
{
    return InsertChild(std::move(child), m_children.size());
}

std::unique_ptr<Chunk> Chunk::RemoveChild(size_t index)
{
    std::unique_ptr<Chunk> child = std::move(m_children[index]);
    m_children.erase(m_children.begin() + static_cast<std::ptrdiff_t>(index));
    child->m_parent = nullptr;
    return child;
}

std::unique_ptr<Chunk> Chunk::Clone() const
{
    auto copy = std::make_unique<Chunk>(m_id, m_payload);
    copy->m_trailing = m_trailing;

    for (const auto& child : m_children)
    {
        copy->AppendChild(child->Clone());
    }

    return copy;
}

size_t Chunk::DataSize() const
{
    return kHeaderSize + m_payload.size();
}

size_t Chunk::TotalSize() const
{
    size_t total = DataSize() + m_trailing.size();

    for (const auto& child : m_children)
    {
        total += child->TotalSize();
    }

    return total;
}

bool Chunk::IsAncestorOf(const Chunk* other) const
{
    for (const Chunk* node = other; node; node = node->m_parent)
    {
        if (node == this)
        {
            return true;
        }
    }

    return false;
}

}
