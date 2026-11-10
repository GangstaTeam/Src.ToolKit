#include "render/RenderTarget.hh"

namespace tk
{

bool RenderTarget::Ensure(IDirect3DDevice9* device, int width, int height)
{
    if (m_texture && width == m_width && height == m_height)
    {
        return true;
    }

    Release();

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    UINT w = static_cast<UINT>(width), h = static_cast<UINT>(height);

    if (FAILED(device->CreateTexture(w,
            h,
            1,
            D3DUSAGE_RENDERTARGET,
            D3DFMT_A8R8G8B8,
            D3DPOOL_DEFAULT,
            &m_texture,
            nullptr)) ||
        FAILED(device->CreateDepthStencilSurface(w, h, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &m_depth, nullptr)))
    {
        Release();
        return false;
    }

    m_texture->GetSurfaceLevel(0, &m_surface);
    m_width = width;
    m_height = height;
    return true;
}

void RenderTarget::Release()
{
    if (m_surface)
    {
        m_surface->Release();
    }

    if (m_depth)
    {
        m_depth->Release();
    }

    if (m_texture)
    {
        m_texture->Release();
    }

    m_surface = nullptr;
    m_depth = nullptr;
    m_texture = nullptr;
    m_width = m_height = 0;
}

bool RenderTarget::Begin(IDirect3DDevice9* device)
{
    if (!m_texture)
    {
        return false;
    }

    device->GetRenderTarget(0, &m_savedColour);
    device->GetDepthStencilSurface(&m_savedDepth);
    device->SetRenderTarget(0, m_surface);
    device->SetDepthStencilSurface(m_depth);
    return SUCCEEDED(device->BeginScene());
}

void RenderTarget::End(IDirect3DDevice9* device)
{
    device->EndScene();
    device->SetRenderTarget(0, m_savedColour);
    device->SetDepthStencilSurface(m_savedDepth);

    if (m_savedColour)
    {
        m_savedColour->Release();
    }

    if (m_savedDepth)
    {
        m_savedDepth->Release();
    }

    m_savedColour = nullptr;
    m_savedDepth = nullptr;
}

}
