#include "rig/FbxConvert.hh"

#include <windows.h>

#include <fstream>

namespace tk
{

namespace
{

const char kScript[] = R"(import sys
import bpy
args = sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=args[0])
for obj in bpy.data.objects:
    if obj.type == "ARMATURE":
        obj.animation_data_clear()
        obj.data.pose_position = "REST"
bpy.ops.export_scene.gltf(filepath=args[1], export_format="GLB", export_skins=True, export_animations=False)
)";

std::filesystem::path EnvPath(const wchar_t* name)
{
    wchar_t buffer[MAX_PATH] = {};
    DWORD length = GetEnvironmentVariableW(name, buffer, MAX_PATH);
    return length > 0 && length < MAX_PATH ? std::filesystem::path(buffer) : std::filesystem::path();
}

std::wstring Quote(const std::filesystem::path& path)
{
    return L"\"" + path.wstring() + L"\"";
}

bool RunHidden(std::wstring commandLine, DWORD& exitCode)
{
    STARTUPINFOW startup = {sizeof(startup)};
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process = {};
    if (!CreateProcessW(nullptr,
            commandLine.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startup,
            &process))
    {
        return false;
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

}

std::optional<std::filesystem::path> FindConverter()
{
    std::error_code ec;
    for (const wchar_t* root : {L"ProgramFiles", L"ProgramW6432"})
    {
        std::filesystem::path foundation = EnvPath(root) / L"Blender Foundation";
        if (!std::filesystem::is_directory(foundation, ec))
        {
            continue;
        }
        std::filesystem::path best;
        for (const auto& entry : std::filesystem::directory_iterator(foundation, ec))
        {
            std::filesystem::path exe = entry.path() / L"blender.exe";
            if (std::filesystem::exists(exe, ec) && (best.empty() || exe > best))
            {
                best = exe;
            }
        }
        if (!best.empty())
        {
            return best;
        }
    }
    return std::nullopt;
}

bool ConvertFbx(const std::filesystem::path& source, const std::filesystem::path& target, std::string& error)
{
    auto converter = FindConverter();
    if (!converter)
    {
        error = "FBX import is not available on this PC. Export the model as .glb instead.";
        return false;
    }
    std::filesystem::path script = std::filesystem::temp_directory_path() / L"toolkit_fbx_to_glb.py";
    std::ofstream(script, std::ios::binary) << kScript;
    std::error_code ec;
    std::filesystem::remove(target, ec);
    std::wstring command =
        Quote(*converter) + L" --background --python " + Quote(script) + L" -- " + Quote(source) + L" " + Quote(target);
    DWORD exitCode = 1;
    if (!RunHidden(command, exitCode))
    {
        error = "could not start the FBX import";
        return false;
    }
    if (!std::filesystem::exists(target, ec))
    {
        error = "could not import " + source.filename().u8string() + " (exit code " + std::to_string(exitCode) + ")";
        return false;
    }
    return true;
}

}
