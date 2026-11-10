#pragma once

#include "core/Bytes.hh"
#include "doc/ChunkPath.hh"

#include <deque>

namespace tk
{

struct Snapshot
{
    Bytes file;
    ChunkPath selection;
};

class History
{
public:
    void Push(Snapshot snapshot);
    bool CanUndo() const { return !m_undo.empty(); }
    bool CanRedo() const { return !m_redo.empty(); }
    Snapshot Undo(Snapshot current);
    Snapshot Redo(Snapshot current);
    void Clear();

private:
    enum ELimit : size_t
    {
        kLimit = 64,
    };

    std::deque<Snapshot> m_undo;
    std::deque<Snapshot> m_redo;
};

}
