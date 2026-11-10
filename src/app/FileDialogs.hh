#pragma once

#include <windows.h>

#include <filesystem>
#include <optional>
#include <vector>

namespace tk
{

std::optional<std::filesystem::path> ShowOpenDialog(HWND owner);
std::optional<std::filesystem::path> ShowOpenModelDialog(HWND owner);
std::optional<std::filesystem::path> ShowOpenTextureDialog(HWND owner);
std::optional<std::filesystem::path> ShowSaveDialog(HWND owner, const std::filesystem::path& suggested);
std::optional<std::filesystem::path> ShowOpenArchiveDialog(HWND owner);
std::optional<std::filesystem::path> ShowSaveArchiveDialog(HWND owner, const std::filesystem::path& suggested);
std::optional<std::filesystem::path> ShowSaveSceneDialog(HWND owner, const std::filesystem::path& suggested);
std::vector<std::filesystem::path> ShowOpenFilesDialog(HWND owner);
std::optional<std::filesystem::path> ShowFolderDialog(HWND owner, const wchar_t* title);

}
