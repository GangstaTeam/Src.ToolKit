#pragma once

#include <d3d9.h>
#include <windows.h>

namespace tk
{

class ImGuiLayer
{
public:
    bool Init(HWND hwnd, IDirect3DDevice9* device);
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    void OnDeviceLost();
    void OnDeviceReset();
};

}
