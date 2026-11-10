#include "app/RecentFiles.hh"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cstring>

namespace tk
{

namespace
{

void* OpenEntry(ImGuiContext*, ImGuiSettingsHandler* handler, const char* name)
{
    return std::strcmp(name, "Recent") == 0 ? handler->UserData : nullptr;
}

void ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    size_t prefix = std::strlen(RECENTFILES_PREFIX);

    if (std::strncmp(line, RECENTFILES_PREFIX, prefix) != 0)
    {
        return;
    }

    static_cast<RecentFiles*>(entry)->Append(std::filesystem::u8path(line + prefix));
}

void WriteAll(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buffer)
{
    auto* recent = static_cast<RecentFiles*>(handler->UserData);
    buffer->appendf("[%s][Recent]\n", handler->TypeName);

    for (const auto& path : recent->Items())
    {
        buffer->appendf("%s%s\n", RECENTFILES_PREFIX, path.u8string().c_str());
    }

    buffer->append("\n");
}

}

void RecentFiles::RegisterSettingsHandler()
{
    ImGuiSettingsHandler handler;
    handler.TypeName = "ToolKit";
    handler.TypeHash = ImHashStr("ToolKit");
    handler.ReadOpenFn = &OpenEntry;
    handler.ReadLineFn = &ReadLine;
    handler.WriteAllFn = &WriteAll;
    handler.UserData = this;
    ImGui::AddSettingsHandler(&handler);
}

void RecentFiles::Add(const std::filesystem::path& path)
{
    Remove(path);
    m_items.insert(m_items.begin(), path);

    if (m_items.size() > kLimit)
    {
        m_items.resize(kLimit);
    }

    ImGui::MarkIniSettingsDirty();
}

void RecentFiles::Append(const std::filesystem::path& path)
{
    if (m_items.size() < kLimit && std::find(m_items.begin(), m_items.end(), path) == m_items.end())
    {
        m_items.push_back(path);
    }
}

void RecentFiles::Remove(const std::filesystem::path& path)
{
    m_items.erase(std::remove(m_items.begin(), m_items.end(), path), m_items.end());
    ImGui::MarkIniSettingsDirty();
}

void RecentFiles::Clear()
{
    m_items.clear();
    ImGui::MarkIniSettingsDirty();
}

}
