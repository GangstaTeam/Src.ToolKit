#include "render/MeshRenderer.hh"

#include <cstring>

namespace tk
{

namespace
{

template <typename T> bool Fill(T* buffer, const void* data, size_t size)
{
    void* target = nullptr;

    if (FAILED(buffer->Lock(0, 0, &target, 0)))
    {
        return false;
    }

    std::memcpy(target, data, size);
    buffer->Unlock();
    return true;
}

D3DLIGHT9 HeadLight(const D3DMATRIX& view)
{
    D3DLIGHT9 light = {};
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse = {0.85f, 0.85f, 0.85f, 1.0f};
    Vec3 forward{view._13, view._23, view._33};
    light.Direction = {forward.x, forward.y - 0.3f, forward.z};
    return light;
}

}

void MeshRenderer::Release()
{
    for (GpuPart& part : m_parts)
    {
        if (part.vertices)
        {
            part.vertices->Release();
        }

        if (part.indices)
        {
            part.indices->Release();
        }
    }

    m_parts.clear();
}

bool MeshRenderer::UploadPart(IDirect3DDevice9* device, const MeshPart& part, GpuPart& gpu)
{
    UINT vertexBytes = static_cast<UINT>(part.vertices.size() * sizeof(MeshVertex));
    UINT indexBytes = static_cast<UINT>(part.indices.size() * sizeof(uint32_t));

    if (FAILED(device->CreateVertexBuffer(vertexBytes,
            D3DUSAGE_WRITEONLY,
            (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1),
            D3DPOOL_MANAGED,
            &gpu.vertices,
            nullptr)) ||
        FAILED(device->CreateIndexBuffer(indexBytes,
            D3DUSAGE_WRITEONLY,
            D3DFMT_INDEX32,
            D3DPOOL_MANAGED,
            &gpu.indices,
            nullptr)))
    {
        return false;
    }

    gpu.vertexCount = static_cast<UINT>(part.vertices.size());
    gpu.primitiveCount = static_cast<UINT>(part.indices.size() / 3);
    gpu.shader = part.shader;
    return Fill(gpu.vertices, part.vertices.data(), vertexBytes) && Fill(gpu.indices, part.indices.data(), indexBytes);
}

bool MeshRenderer::UpdateVertices(size_t part, const std::vector<MeshVertex>& vertices)
{
    if (part >= m_parts.size() || vertices.size() != m_parts[part].vertexCount)
    {
        return false;
    }

    return Fill(m_parts[part].vertices, vertices.data(), vertices.size() * sizeof(MeshVertex));
}

void MeshRenderer::Upload(IDirect3DDevice9* device, const MeshScene& scene)
{
    Release();

    for (const MeshPart& part : scene.parts)
    {
        GpuPart gpu;

        if (UploadPart(device, part, gpu) && gpu.primitiveCount > 0)
        {
            m_parts.push_back(gpu);
        }
        else
        {
            if (gpu.vertices)
            {
                gpu.vertices->Release();
            }

            if (gpu.indices)
            {
                gpu.indices->Release();
            }
        }
    }
}

void MeshRenderer::SetupStates(IDirect3DDevice9* device,
    const D3DMATRIX& view,
    const D3DMATRIX& projection,
    const RenderOptions& options) const
{
    D3DMATRIX world = Identity();
    device->SetTransform(D3DTS_WORLD, &world);
    device->SetTransform(D3DTS_VIEW, &view);
    device->SetTransform(D3DTS_PROJECTION, &projection);
    device->SetVertexShader(nullptr);
    device->SetPixelShader(nullptr);
    device->SetFVF((D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1));
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    device->SetRenderState(D3DRS_ALPHAREF, 0x40);
    device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    device->SetRenderState(D3DRS_FILLMODE, options.wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
    device->SetRenderState(D3DRS_LIGHTING, options.lighting ? TRUE : FALSE);
    device->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(90, 90, 90));
    device->SetRenderState(D3DRS_COLORVERTEX, TRUE);
    device->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    device->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
    D3DLIGHT9 light = HeadLight(view);
    device->SetLight(0, &light);
    device->LightEnable(0, TRUE);
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
}

void MeshRenderer::Draw(IDirect3DDevice9* device,
    const D3DMATRIX& view,
    const D3DMATRIX& projection,
    const TextureLibrary& textures,
    const RenderOptions& options) const
{
    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(30, 22, 22), 1.0f, 0);
    SetupStates(device, view, projection, options);

    for (const GpuPart& part : m_parts)
    {
        IDirect3DTexture9* texture = options.textured ? textures.ForShader(part.shader) : nullptr;
        device->SetTexture(0, texture);
        device->SetTextureStageState(0, D3DTSS_COLOROP, texture ? D3DTOP_MODULATE : D3DTOP_SELECTARG2);
        device->SetTextureStageState(0, D3DTSS_ALPHAOP, texture ? D3DTOP_SELECTARG1 : D3DTOP_SELECTARG2);
        device->SetStreamSource(0, part.vertices, 0, sizeof(MeshVertex));
        device->SetIndices(part.indices);
        device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, part.vertexCount, 0, part.primitiveCount);
    }

    device->SetTexture(0, nullptr);
    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
}

}
