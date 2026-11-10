#include "app/FileDialogs.hh"

#include <commdlg.h>
#include <shobjidl.h>

#include <cwchar>

namespace tk
{

namespace
{

const wchar_t kFilter[] =
    L"Game files (*.p3d;*.rcf;*.fsc)\0*.p3d;*.rcf;*.fsc\0Pure3D files (*.p3d)\0*.p3d\0Cement archives (*.rcf)\0*.rcf\0"
    L"NIS scenes (*.fsc)\0*.fsc\0All files (*.*)\0*.*\0";
const wchar_t kSceneFilter[] = L"NIS scenes (*.fsc)\0*.fsc\0All files (*.*)\0*.*\0";

const wchar_t kTextureFilter[] =
    L"Images or models (*.png;*.jpg;*.jpeg;*.bmp;*.glb;*.gltf)\0*.png;*.jpg;*.jpeg;*.bmp;*.glb;*.gltf\0All files "
    L"(*.*)\0*.*\0";
const wchar_t kArchiveFilter[] = L"Cement archives (*.rcf)\0*.rcf\0All files (*.*)\0*.*\0";
const wchar_t kAnyFilter[] = L"All files (*.*)\0*.*\0";
const wchar_t kModelFilter[] = L"3D models (*.glb;*.gltf;*.fbx)\0*.glb;*.gltf;*.fbx\0All files (*.*)\0*.*\0";

OPENFILENAMEW MakeDialog(HWND owner, wchar_t* buffer, const wchar_t* filter = kFilter)
{
    OPENFILENAMEW dialog = {sizeof(dialog)};
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = buffer;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrDefExt = L"p3d";
    return dialog;
}

}

std::optional<std::filesystem::path> ShowOpenDialog(HWND owner)
{
    wchar_t buffer[MAX_PATH] = {};
    OPENFILENAMEW dialog = MakeDialog(owner, buffer);
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&dialog))
    {
        return std::nullopt;
    }

    return std::filesystem::path(buffer);
}

std::optional<std::filesystem::path> ShowOpenFiltered(HWND owner, const wchar_t* filter)
{
    wchar_t buffer[MAX_PATH] = {};
    OPENFILENAMEW dialog = MakeDialog(owner, buffer, filter);
    dialog.lpstrDefExt = nullptr;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&dialog))
    {
        return std::nullopt;
    }

    return std::filesystem::path(buffer);
}

std::optional<std::filesystem::path> ShowOpenModelDialog(HWND owner)
{
    return ShowOpenFiltered(owner, kModelFilter);
}

std::optional<std::filesystem::path> ShowOpenTextureDialog(HWND owner)
{
    return ShowOpenFiltered(owner, kTextureFilter);
}

std::optional<std::filesystem::path> ShowSaveDialog(HWND owner, const std::filesystem::path& suggested)
{
    wchar_t buffer[MAX_PATH] = {};
    std::wcsncpy(buffer, suggested.wstring().c_str(), MAX_PATH - 1);
    OPENFILENAMEW dialog = MakeDialog(owner, buffer);
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetSaveFileNameW(&dialog))
    {
        return std::nullopt;
    }

    return std::filesystem::path(buffer);
}

std::optional<std::filesystem::path> ShowOpenArchiveDialog(HWND owner)
{
    return ShowOpenFiltered(owner, kArchiveFilter);
}

std::optional<std::filesystem::path> ShowSaveArchiveDialog(HWND owner, const std::filesystem::path& suggested)
{
    wchar_t buffer[MAX_PATH] = {};
    std::wcsncpy(buffer, suggested.wstring().c_str(), MAX_PATH - 1);
    OPENFILENAMEW dialog = MakeDialog(owner, buffer, kArchiveFilter);
    dialog.lpstrDefExt = L"rcf";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetSaveFileNameW(&dialog))
    {
        return std::nullopt;
    }

    return std::filesystem::path(buffer);
}

std::optional<std::filesystem::path> ShowSaveSceneDialog(HWND owner, const std::filesystem::path& suggested)
{
    wchar_t buffer[MAX_PATH] = {};
    std::wcsncpy(buffer, suggested.wstring().c_str(), MAX_PATH - 1);
    OPENFILENAMEW dialog = MakeDialog(owner, buffer, kSceneFilter);
    dialog.lpstrDefExt = L"fsc";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetSaveFileNameW(&dialog))
    {
        return std::nullopt;
    }

    return std::filesystem::path(buffer);
}

std::vector<std::filesystem::path> ShowOpenFilesDialog(HWND owner)
{
    std::vector<wchar_t> buffer(64 * 1024);
    OPENFILENAMEW dialog = MakeDialog(owner, buffer.data(), kAnyFilter);
    dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.lpstrDefExt = nullptr;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    if (!GetOpenFileNameW(&dialog))
    {
        return {};
    }

    std::vector<std::filesystem::path> paths;
    std::filesystem::path first(buffer.data());
    const wchar_t* name = buffer.data() + wcslen(buffer.data()) + 1;

    if (!*name)
    {
        return {first};
    }

    for (; *name; name += wcslen(name) + 1)
    {
        paths.push_back(first / name);
    }

    return paths;
}

std::optional<std::filesystem::path> ShowFolderDialog(HWND owner, const wchar_t* title)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IFileOpenDialog* dialog = nullptr;

    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))))
    {
        return std::nullopt;
    }

    std::optional<std::filesystem::path> result;
    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR);
    dialog->SetTitle(title);
    IShellItem* item = nullptr;

    if (SUCCEEDED(dialog->Show(owner)) && SUCCEEDED(dialog->GetResult(&item)))
    {
        PWSTR path = nullptr;

        if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
        {
            result = std::filesystem::path(path);
            CoTaskMemFree(path);
        }

        item->Release();
    }

    dialog->Release();
    return result;
}

}
