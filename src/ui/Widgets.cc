#include "ui/Widgets.hh"

#include <imgui.h>

#include <cstdio>
#include <cstring>

namespace tk
{

namespace
{

int ResizeCallback(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag != ImGuiInputTextFlags_CallbackResize)
    {
        return 0;
    }

    auto* text = static_cast<std::string*>(data->UserData);
    text->resize(static_cast<size_t>(data->BufTextLen));
    data->Buf = text->data();
    return 0;
}

}

bool InputString(const char* id, std::string& value)
{
    return ImGui::InputText(id,
        value.data(),
        value.capacity() + 1,
        ImGuiInputTextFlags_CallbackResize,
        ResizeCallback,
        &value);
}

bool InputMultiline(const char* id, std::string& value, const ImVec2& size)
{
    return ImGui::InputTextMultiline(id,
        value.data(),
        value.capacity() + 1,
        size,
        ImGuiInputTextFlags_CallbackResize,
        ResizeCallback,
        &value);
}

bool InputFourCC(const char* id, uint32_t& value)
{
    char text[5] = {};
    std::memcpy(text, &value, 4);

    if (!ImGui::InputText(id, text, sizeof(text)))
    {
        return false;
    }

    value = 0;
    std::memcpy(&value, text, std::strlen(text));
    return true;
}

bool InputArgb(const char* id, uint32_t& value)
{
    float colour[4] = {((value >> 16) & 0xFF) / 255.0f,
        ((value >> 8) & 0xFF) / 255.0f,
        (value & 0xFF) / 255.0f,
        ((value >> 24) & 0xFF) / 255.0f};
    if (!ImGui::ColorEdit4(id, colour, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Uint8))
    {
        return false;
    }

    auto channel = [](float f)
    {
        return static_cast<uint32_t>(f * 255.0f + 0.5f) & 0xFF;
    };
    value = (channel(colour[3]) << 24) | (channel(colour[0]) << 16) | (channel(colour[1]) << 8) | channel(colour[2]);
    return true;
}

std::string FormatHex(uint32_t value)
{
    char text[16];
    std::snprintf(text, sizeof(text), "0x%X", value);
    return text;
}

}
