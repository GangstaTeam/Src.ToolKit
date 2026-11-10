#include "app/Dx9Device.hh"

namespace tk
{

bool Dx9Device::Create(HWND hwnd)
{
    m_d3d = Direct3DCreate9(D3D_SDK_VERSION);

    if (!m_d3d)
    {
        return false;
    }

    m_params.Windowed = TRUE;
    m_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    m_params.BackBufferFormat = D3DFMT_UNKNOWN;
    m_params.EnableAutoDepthStencil = TRUE;
    m_params.AutoDepthStencilFormat = D3DFMT_D24S8;
    m_params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    return SUCCEEDED(m_d3d->CreateDevice(D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        hwnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &m_params,
        &m_device));
}

void Dx9Device::Destroy()
{
    if (m_device)
    {
        m_device->Release();
    }

    if (m_d3d)
    {
        m_d3d->Release();
    }

    m_device = nullptr;
    m_d3d = nullptr;
}

void Dx9Device::Resize(int width, int height)
{
    if (!m_device)
    {
        return;
    }

    m_params.BackBufferWidth = static_cast<UINT>(width);
    m_params.BackBufferHeight = static_cast<UINT>(height);
    Reset();
}

void Dx9Device::Reset()
{
    if (onDeviceLost)
    {
        onDeviceLost();
    }

    if (m_device->Reset(&m_params) == D3DERR_INVALIDCALL)
    {
        return;
    }

    if (onDeviceReset)
    {
        onDeviceReset();
    }
}

bool Dx9Device::BeginFrame()
{
    if (m_lost)
    {
        HRESULT state = m_device->TestCooperativeLevel();

        if (state == D3DERR_DEVICELOST)
        {
            return false;
        }

        if (state == D3DERR_DEVICENOTRESET)
        {
            Reset();
        }

        m_lost = false;
    }

    m_device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(30, 30, 34, 255), 1.0f, 0);
    return SUCCEEDED(m_device->BeginScene());
}

void Dx9Device::EndFrame()
{
    m_device->EndScene();

    if (m_device->Present(nullptr, nullptr, nullptr, nullptr) == D3DERR_DEVICELOST)
    {
        m_lost = true;
    }
}

}
