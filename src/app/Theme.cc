#include "app/Theme.hh"

#include <filesystem>

namespace tk::theme
{

namespace
{

ImVec4 Rgb(int r, int g, int b, float a = 1.0f)
{
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

void ApplyShape(ImGuiStyle& style)
{
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
    style.WindowPadding = ImVec2(10.0f, 8.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;
    style.SeparatorTextBorderSize = 2.0f;
}

void ApplySurfaces(ImVec4* c)
{
    c[ImGuiCol_WindowBg] = Rgb(16, 12, 12);
    c[ImGuiCol_ChildBg] = Rgb(16, 12, 12);
    c[ImGuiCol_PopupBg] = Rgb(20, 14, 14, 0.98f);
    c[ImGuiCol_MenuBarBg] = Rgb(10, 8, 8);
    c[ImGuiCol_Border] = Rgb(58, 40, 22);
    c[ImGuiCol_BorderShadow] = Rgb(0, 0, 0, 0.0f);
    c[ImGuiCol_FrameBg] = Rgb(32, 22, 22);
    c[ImGuiCol_FrameBgHovered] = Rgb(48, 26, 26);
    c[ImGuiCol_FrameBgActive] = Rgb(64, 28, 28);
    c[ImGuiCol_TitleBg] = Rgb(10, 8, 8);
    c[ImGuiCol_TitleBgActive] = Rgb(26, 10, 10);
    c[ImGuiCol_TitleBgCollapsed] = Rgb(10, 8, 8);
    c[ImGuiCol_ScrollbarBg] = Rgb(12, 9, 9);
    c[ImGuiCol_ScrollbarGrab] = Rgb(70, 30, 30);
    c[ImGuiCol_ScrollbarGrabHovered] = Rgb(142, 16, 24);
    c[ImGuiCol_ScrollbarGrabActive] = Rgb(200, 22, 31);
    c[ImGuiCol_TableHeaderBg] = Rgb(26, 16, 16);
    c[ImGuiCol_TableRowBg] = Rgb(0, 0, 0, 0.0f);
    c[ImGuiCol_TableRowBgAlt] = Rgb(255, 230, 200, 0.03f);
    c[ImGuiCol_TableBorderLight] = Rgb(40, 28, 20);
    c[ImGuiCol_TableBorderStrong] = Rgb(58, 40, 22);
    c[ImGuiCol_DockingEmptyBg] = Rgb(12, 9, 9);
    c[ImGuiCol_ModalWindowDimBg] = Rgb(0, 0, 0, 0.6f);
}

void ApplyAccents(ImVec4* c)
{
    c[ImGuiCol_Text] = Rgb(236, 228, 210);
    c[ImGuiCol_TextDisabled] = Rgb(128, 114, 100);
    c[ImGuiCol_TextSelectedBg] = Rgb(200, 22, 31, 0.45f);
    c[ImGuiCol_CheckMark] = Rgb(212, 175, 55);
    c[ImGuiCol_SliderGrab] = Rgb(212, 175, 55);
    c[ImGuiCol_SliderGrabActive] = Rgb(240, 205, 90);
    c[ImGuiCol_Button] = Rgb(46, 20, 20);
    c[ImGuiCol_ButtonHovered] = Rgb(142, 16, 24);
    c[ImGuiCol_ButtonActive] = Rgb(200, 22, 31);
    c[ImGuiCol_Header] = Rgb(142, 16, 24, 0.75f);
    c[ImGuiCol_HeaderHovered] = Rgb(179, 18, 29, 0.85f);
    c[ImGuiCol_HeaderActive] = Rgb(200, 22, 31);
    c[ImGuiCol_Separator] = Rgb(110, 84, 30);
    c[ImGuiCol_SeparatorHovered] = Rgb(212, 175, 55);
    c[ImGuiCol_SeparatorActive] = Rgb(240, 205, 90);
    c[ImGuiCol_ResizeGrip] = Rgb(142, 16, 24, 0.4f);
    c[ImGuiCol_ResizeGripHovered] = Rgb(179, 18, 29);
    c[ImGuiCol_ResizeGripActive] = Rgb(212, 175, 55);
    c[ImGuiCol_Tab] = Rgb(20, 12, 12);
    c[ImGuiCol_TabHovered] = Rgb(179, 18, 29);
    c[ImGuiCol_TabActive] = Rgb(122, 14, 19);
    c[ImGuiCol_TabUnfocused] = Rgb(16, 11, 11);
    c[ImGuiCol_TabUnfocusedActive] = Rgb(74, 12, 15);
    c[ImGuiCol_DockingPreview] = Rgb(200, 22, 31, 0.6f);
    c[ImGuiCol_PlotLines] = Rgb(212, 175, 55);
    c[ImGuiCol_PlotHistogram] = Rgb(212, 175, 55);
    c[ImGuiCol_DragDropTarget] = Rgb(212, 175, 55);
    c[ImGuiCol_NavHighlight] = Rgb(212, 175, 55);
}

}

void Apply()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    ApplyShape(style);
    ApplySurfaces(style.Colors);
    ApplyAccents(style.Colors);
}

void LoadFonts(ImGuiIO& io)
{
    const char* preferred = "C:\\Windows\\Fonts\\bahnschrift.ttf";
    const char* fallback = "C:\\Windows\\Fonts\\segoeui.ttf";
    const char* font = std::filesystem::exists(preferred) ? preferred : fallback;
    io.Fonts->AddFontFromFileTTF(font, 18.0f);

    const char* icons = "C:\\Windows\\Fonts\\segmdl2.ttf";

    if (!std::filesystem::exists(icons))
    {
        return;
    }

    static const ImWchar ranges[] = {0xE700, 0xF8FF, 0};
    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphOffset = ImVec2(0.0f, 3.0f);
    config.GlyphMinAdvanceX = 18.0f;
    io.Fonts->AddFontFromFileTTF(icons, 15.0f, &config, ranges);
}

}
