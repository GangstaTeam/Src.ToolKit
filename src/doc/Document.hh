#pragma once

#include "doc/History.hh"
#include "p3d/Chunk.hh"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace tk
{

class Document
{
public:
    Document();

    void New();
    bool Open(const std::filesystem::path& path, std::string& error);
    bool Save(std::string& error);
    bool SaveAs(const std::filesystem::path& path, std::string& error);

    Chunk* Root() const { return m_root.get(); }
    const std::filesystem::path& Path() const { return m_path; }
    bool HasPath() const { return !m_path.empty(); }
    bool Dirty() const { return m_dirty; }
    uint64_t Revision() const { return m_revision; }

    Chunk* Selection() const { return m_selection; }
    void Select(Chunk* chunk) { m_selection = chunk; }

    void SetPayload(Chunk* chunk, Bytes payload);
    void SetId(Chunk* chunk, uint32_t id);
    Chunk* Insert(Chunk* parent, size_t index, std::unique_ptr<Chunk> chunk);
    void Remove(Chunk* chunk);
    void Move(Chunk* chunk, Chunk* newParent, size_t index);
    void Edit(const std::function<void(Chunk& root)>& edit);

    bool CanUndo() const { return m_history.CanUndo(); }
    bool CanRedo() const { return m_history.CanRedo(); }
    void Undo();
    void Redo();

private:
    Snapshot Capture() const;
    void Restore(const Snapshot& snapshot);
    void BeginEdit();

    std::unique_ptr<Chunk> m_root;
    std::filesystem::path m_path;
    Chunk* m_selection = nullptr;
    History m_history;
    bool m_dirty = false;
    uint64_t m_revision = 0;
};

}
