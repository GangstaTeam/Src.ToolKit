#pragma once

#include <cstdint>

#include <imgui.h>

namespace tk::theme
{

enum EThemeColour : uint32_t
{
    kGold = 0xFF37AFD4,
    kRed = 0xFF1F16C8,
    kBone = 0xFFD2E4EC,
};

void Apply();
void LoadFonts(ImGuiIO& io);

}
