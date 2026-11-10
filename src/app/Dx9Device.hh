#pragma once

#include <d3d9.h>

#include <functional>

namespace tk
{

class Dx9Device
{
public:
    bool Create(HWND hwnd);
    void Destroy();
    void Resize(int width, int height);
    bool BeginFrame();
    void EndFrame();
    IDirect3DDevice9* Get() const { return m_device; }

    std::function<void()> onDeviceLost;
    std::function<void()> onDeviceReset;

private:
    void Reset();

    IDirect3D9* m_d3d = nullptr;
    IDirect3DDevice9* m_device = nullptr;
    D3DPRESENT_PARAMETERS m_params = {};
    bool m_lost = false;
};

}
