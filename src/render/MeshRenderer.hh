#pragma once

#include "render/MeshData.hh"
#include "render/Math3d.hh"
#include "render/TextureLibrary.hh"

#include <d3d9.h>

#include <string>
#include <vector>

namespace tk
{

struct RenderOptions
{
    bool wireframe = false;
    bool lighting = true;
    bool textured = true;
};

class MeshRenderer
{
public:
    MeshRenderer() = default;
    MeshRenderer(const MeshRenderer&) = delete;
    MeshRenderer& operator=(const MeshRenderer&) = delete;
    ~MeshRenderer() { Release(); }

    void Upload(IDirect3DDevice9* device, const MeshScene& scene);
    bool UpdateVertices(size_t part, const std::vector<MeshVertex>& vertices);
    void Release();
    void Draw(IDirect3DDevice9* device,
        const D3DMATRIX& view,
        const D3DMATRIX& projection,
        const TextureLibrary& textures,
        const RenderOptions& options) const;

private:
    struct GpuPart
    {
        IDirect3DVertexBuffer9* vertices = nullptr;
        IDirect3DIndexBuffer9* indices = nullptr;
        UINT vertexCount = 0;
        UINT primitiveCount = 0;
        std::string shader;
    };

    void SetupStates(IDirect3DDevice9* device,
        const D3DMATRIX& view,
        const D3DMATRIX& projection,
        const RenderOptions& options) const;
    bool UploadPart(IDirect3DDevice9* device, const MeshPart& part, GpuPart& gpu);

    std::vector<GpuPart> m_parts;
};

}
