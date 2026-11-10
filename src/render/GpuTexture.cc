#include "render/GpuTexture.hh"

#include <cstring>

namespace tk
{

bool GpuTexture::Upload(IDirect3DDevice9* device, const Image& image)
{
    Release();

    if (FAILED(device->CreateTexture(image.width,
            image.height,
            1,
            0,
            D3DFMT_A8R8G8B8,
            D3DPOOL_MANAGED,
            &m_texture,
            nullptr)))
    {
        return false;
    }

    D3DLOCKED_RECT rect;

    if (FAILED(m_texture->LockRect(0, &rect, nullptr, 0)))
    {
        Release();
        return false;
    }

    for (uint32_t y = 0; y < image.height; ++y)
    {
        std::memcpy(static_cast<uint8_t*>(rect.pBits) + static_cast<size_t>(y) * rect.Pitch,
            image.bgra.data() + static_cast<size_t>(y) * image.width * 4,
            image.width * 4);
    }

    m_texture->UnlockRect(0);
    m_width = image.width;
    m_height = image.height;
    return true;
}

void GpuTexture::Release()
{
    if (m_texture)
    {
        m_texture->Release();
    }

    m_texture = nullptr;
    m_width = m_height = 0;
}

}
