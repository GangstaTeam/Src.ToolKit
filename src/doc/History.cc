#include "doc/History.hh"

namespace tk
{

void History::Push(Snapshot snapshot)
{
    m_undo.push_back(std::move(snapshot));

    if (m_undo.size() > kLimit)
    {
        m_undo.pop_front();
    }

    m_redo.clear();
}

Snapshot History::Undo(Snapshot current)
{
    Snapshot previous = std::move(m_undo.back());
    m_undo.pop_back();
    m_redo.push_back(std::move(current));
    return previous;
}

Snapshot History::Redo(Snapshot current)
{
    Snapshot next = std::move(m_redo.back());
    m_redo.pop_back();
    m_undo.push_back(std::move(current));
    return next;
}

void History::Clear()
{
    m_undo.clear();
    m_redo.clear();
}

}
