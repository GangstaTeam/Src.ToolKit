#include "ui/TexturePanel.hh"

#include "render/TextureLibrary.hh"

#include <imgui.h>

namespace tk
{

void TexturePanel::Refresh(const Chunk* chunk)
{
    if (chunk == m_cachedChunk && m_document.Revision() == m_cachedRevision)
    {
        return;
    }

    m_cachedChunk = chunk;
    m_cachedRevision = m_document.Revision();
    m_texture.Release();
    const Chunk* imageData = FindImageData(chunk);

    if (!imageData)
    {
        return;
    }

    if (auto image = DecodeImageData(*imageData))
    {
        m_texture.Upload(m_device, *image);
    }
}

void TexturePanel::Draw()
{
    if (!ImGui::Begin("Texture"))
    {
        ImGui::End();
        return;
    }

    Refresh(m_document.Selection());

    if (!m_texture.Get())
    {
        ImGui::TextDisabled("Select a Texture, Image or ImageData chunk.");
    }
    else
    {
        ImGui::Text("%u x %u", m_texture.Width(), m_texture.Height());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160);
        ImGui::SliderFloat("Zoom", &m_zoom, 0.1f, 4.0f, "%.2fx");
        ImGui::BeginChild("image", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::Image(m_texture.Get(), ImVec2(m_texture.Width() * m_zoom, m_texture.Height() * m_zoom));
        ImGui::EndChild();
    }

    ImGui::End();
}

}
