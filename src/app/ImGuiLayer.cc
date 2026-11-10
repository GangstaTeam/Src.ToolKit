#include "app/ImGuiLayer.hh"

#include "app/Theme.hh"

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

#include <filesystem>
#include <string>

namespace tk
{

namespace
{

const char* IniPath()
{
    static const std::string path = []
    {
        wchar_t module[MAX_PATH];
        GetModuleFileNameW(nullptr, module, MAX_PATH);
        return (std::filesystem::path(module).parent_path() / "toolkit.ini").string();
    }();
    return path.c_str();
}

}

bool ImGuiLayer::Init(HWND hwnd, IDirect3DDevice9* device)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = IniPath();
    theme::LoadFonts(io);
    theme::Apply();
    return ImGui_ImplWin32_Init(hwnd) && ImGui_ImplDX9_Init(device);
}

void ImGuiLayer::Shutdown()
{
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::BeginFrame()
{
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::EndFrame()
{
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::OnDeviceLost()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
}

void ImGuiLayer::OnDeviceReset()
{
    ImGui_ImplDX9_CreateDeviceObjects();
}

}
