#include "render/SkeletonRenderer.hh"

namespace tk
{

namespace
{

struct LineVertex
{
    float x, y, z;
    D3DCOLOR colour;
};

const D3DCOLOR kAxisColours[3] = {D3DCOLOR_XRGB(235, 60, 60), D3DCOLOR_XRGB(60, 220, 60), D3DCOLOR_XRGB(70, 110, 255)};

void AddLine(std::vector<LineVertex>& lines, Vec3 a, Vec3 b, D3DCOLOR colour)
{
    lines.push_back({a.x, a.y, a.z, colour});
    lines.push_back({b.x, b.y, b.z, colour});
}

}

void DrawSkeletonLines(IDirect3DDevice9* device, const std::vector<JointPose>& joints, float axisLength)
{
    std::vector<LineVertex> lines;

    for (const JointPose& joint : joints)
    {
        if (joint.parent >= 0)
        {
            AddLine(lines,
                joints[static_cast<size_t>(joint.parent)].position,
                joint.position,
                D3DCOLOR_XRGB(212, 175, 55));
        }

        for (int a = 0; a < 3; ++a)
        {
            AddLine(lines, joint.position, joint.position + joint.axes[a] * axisLength, kAxisColours[a]);
        }
    }

    if (lines.empty())
    {
        return;
    }

    device->SetTexture(0, nullptr);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    device->SetFVF((D3DFVF_XYZ | D3DFVF_DIFFUSE));
    device->DrawPrimitiveUP(D3DPT_LINELIST, static_cast<UINT>(lines.size() / 2), lines.data(), sizeof(LineVertex));
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
}

}
