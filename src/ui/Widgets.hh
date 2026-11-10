#pragma once

#include <imgui.h>

#include <cstdint>
#include <string>

namespace tk
{

bool InputString(const char* id, std::string& value);
bool InputMultiline(const char* id, std::string& value, const ImVec2& size);
bool InputFourCC(const char* id, uint32_t& value);
bool InputArgb(const char* id, uint32_t& value);
std::string FormatHex(uint32_t value);

}
