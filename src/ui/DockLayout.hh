#pragma once

#include <imgui.h>

namespace tk
{

bool ApplyPure3DLayoutIfEmpty(ImGuiID dockspace);
bool ApplyArchiveLayoutIfEmpty(ImGuiID dockspace);
bool ApplyFrontendLayoutIfEmpty(ImGuiID dockspace);
bool ApplyGameLayoutIfEmpty(ImGuiID dockspace);
bool ApplyNisLayoutIfEmpty(ImGuiID dockspace);

}
