#include "doc/Document.hh"

#include "core/FileIo.hh"
#include "p3d/ChunkIo.hh"

namespace tk
{

Document::Document()
{
    New();
}

void Document::New()
{
    m_root = MakeEmptyFile();
    m_path.clear();
    m_selection = nullptr;
    m_history.Clear();
    m_dirty = false;
    ++m_revision;
}

bool Document::Open(const std::filesystem::path& path, std::string& error)
{
    auto bytes = ReadFile(path);

    if (!bytes)
    {
        error = "cannot read " + path.u8string();
        return false;
    }

    ParseResult parsed = ParseP3d(*bytes);

    if (!parsed.root)
    {
        error = path.filename().u8string() + ": " + parsed.error;
        return false;
    }

    m_root = std::move(parsed.root);
    m_path = path;
    m_selection = nullptr;
    m_history.Clear();
    m_dirty = false;
    ++m_revision;
    return true;
}

bool Document::Save(std::string& error)
{
    return SaveAs(m_path, error);
}

bool Document::SaveAs(const std::filesystem::path& path, std::string& error)
{
    if (!WriteFile(path, SerializeP3d(*m_root)))
    {
        error = "cannot write " + path.u8string();
        return false;
    }

    m_path = path;
    m_dirty = false;
    return true;
}

Snapshot Document::Capture() const
{
    return {SerializeP3d(*m_root), PathOf(m_selection)};
}

void Document::Restore(const Snapshot& snapshot)
{
    m_root = ParseP3d(snapshot.file).root;
    m_selection = Resolve(m_root.get(), snapshot.selection);
    m_dirty = true;
    ++m_revision;
}

void Document::BeginEdit()
{
    m_history.Push(Capture());
    m_dirty = true;
    ++m_revision;
}

void Document::SetPayload(Chunk* chunk, Bytes payload)
{
    if (chunk->Payload() == payload)
    {
        return;
    }

    BeginEdit();
    chunk->SetPayload(std::move(payload));
}

void Document::SetId(Chunk* chunk, uint32_t id)
{
    if (chunk->Id() == id)
    {
        return;
    }

    BeginEdit();
    chunk->SetId(id);
}

Chunk* Document::Insert(Chunk* parent, size_t index, std::unique_ptr<Chunk> chunk)
{
    BeginEdit();
    return parent->InsertChild(std::move(chunk), index);
}

void Document::Remove(Chunk* chunk)
{
    Chunk* parent = chunk->Parent();

    if (!parent)
    {
        return;
    }

    BeginEdit();

    if (m_selection && chunk->IsAncestorOf(m_selection))
    {
        m_selection = nullptr;
    }

    parent->RemoveChild(chunk->IndexInParent());
}

void Document::Move(Chunk* chunk, Chunk* newParent, size_t index)
{
    Chunk* parent = chunk->Parent();

    if (!parent || chunk->IsAncestorOf(newParent))
    {
        return;
    }

    BeginEdit();
    size_t from = chunk->IndexInParent();

    if (parent == newParent && from < index)
    {
        --index;
    }

    newParent->InsertChild(parent->RemoveChild(from), index);
}

void Document::Edit(const std::function<void(Chunk& root)>& edit)
{
    BeginEdit();
    m_selection = nullptr;
    edit(*m_root);
}

void Document::Undo()
{
    if (!m_history.CanUndo())
    {
        return;
    }

    Restore(m_history.Undo(Capture()));
}

void Document::Redo()
{
    if (!m_history.CanRedo())
    {
        return;
    }

    Restore(m_history.Redo(Capture()));
}

}
