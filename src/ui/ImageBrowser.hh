#pragma once

#include <cstddef>
#include <cstdint>

#include "fe/FeImageIndex.hh"
#include "fe/FeResources.hh"

#include <d3d9.h>

#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace tk
{

enum EImageBrowserColour : uint32_t
{
    kCellBack = 0xFF202022,
    kCellHover = 0xFF3C3C46,
    kCellCurrent = 0xFF40C4FF,
};

enum EImageBrowser : int
{
    kLoadsPerFrame = 4,
};

class ImageBrowser
{
public:
    explicit ImageBrowser(IDirect3DDevice9* device) : m_device(device) {}

    static const char* Window() { return "Images"; }

    void Open(const std::string& current);
    void SetCurrent(const std::string& current) { m_current = current; }
    bool IsOpen() const { return m_open; }
    void Clear();
    bool Draw(const fe::ImageIndex& index, std::string& picked);

private:
    void Filter(const fe::ImageIndex& index);
    void DrawCell(const fe::ImageIndex& index,
        const std::string& name,
        float cell,
        int& budget,
        std::string& picked,
        bool& chosen);

    IDirect3DDevice9* m_device;
    fe::Resources m_resources;
    std::set<std::filesystem::path> m_loaded;
    bool m_open = false;
    bool m_focusFilter = false;
    char m_filter[64] = "";
    std::string m_appliedFilter;
    size_t m_indexedCount = SIZE_MAX;
    std::vector<std::string> m_filtered;
    std::string m_current;
    float m_cell = 112.0f;
};

}
