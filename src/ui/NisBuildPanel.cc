#include "ui/NisWorkspace.hh"

#include "app/FileDialogs.hh"
#include "ui/Icons.hh"
#include "ui/Widgets.hh"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace tk
{

namespace
{

const ImVec4 kErrorColour(0.95f, 0.35f, 0.30f, 1.0f);
const ImVec4 kDoneColour(0.45f, 0.85f, 0.45f, 1.0f);

bool ContainsNoCase(const std::string& text, const std::string& part)
{
    auto lower = [](std::string s)
    {
        std::transform(s.begin(),
            s.end(),
            s.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
        return s;
    };
    return lower(text).find(lower(part)) != std::string::npos;
}

}

bool NisWorkspace::DrawSceneChooser(const char* id, std::string& scene)
{
    bool changed = false;

    if (!ImGui::BeginCombo(id, scene.empty() ? "(pick a scene)" : scene.c_str(), ImGuiComboFlags_HeightLarge))
    {
        return false;
    }

    if (ImGui::IsWindowAppearing())
    {
        m_sceneFolders = nis::ListSceneFolders(GameFolder());
        m_chooserFilter.clear();
        ImGui::SetKeyboardFocusHere();
    }

    ImGui::SetNextItemWidth(-FLT_MIN);
    InputString("##filter", m_chooserFilter);

    for (const std::string& folder : m_sceneFolders)
    {
        if (!m_chooserFilter.empty() && !ContainsNoCase(folder, m_chooserFilter))
        {
            continue;
        }

        bool selected = folder == scene;

        if (ImGui::Selectable(folder.c_str(), selected))
        {
            scene = folder;
            changed = true;
        }

        if (selected && ImGui::IsWindowAppearing())
        {
            ImGui::SetItemDefaultFocus();
        }
    }

    ImGui::EndCombo();
    return changed;
}

bool NisWorkspace::DrawGameFolder()
{
    std::filesystem::path folder = GameFolder();
    ImGui::TextDisabled("%s", folder.empty() ? "Game folder not set" : folder.u8string().c_str());
    ImGui::SameLine();

    if (!ImGui::SmallButton(folder.empty() ? "Set game folder..." : "Change..."))
    {
        return false;
    }

    auto picked = ShowFolderDialog(m_owner, L"Scarface game folder (the one holding art\\nis)");

    if (!picked)
    {
        return false;
    }

    std::filesystem::path detected = nis::DetectGameFolder(*picked / "x");

    if (detected.empty())
    {
        m_actions.error("No art\\nis folder in " + picked->u8string());
        return false;
    }

    m_actions.setGameFolder(detected);
    ReloadAssets();
    return true;
}

GameLink* NisWorkspace::LiveGame() const
{
    GameLink* link = m_actions.link ? m_actions.link() : nullptr;
    return link && link->Connected() ? link : nullptr;
}

void NisWorkspace::DrawPlayInGame()
{
    GameLink* link = LiveGame();
    std::filesystem::path game = GameFolder();
    bool built = !m_path.empty() && !game.empty() && m_path.parent_path() == nis::NisFolder(game) / "fsc";
    std::string scene = m_path.stem().u8string();
    link::PlayerRecord player = {};
    bool hasPlayer = link && link->Player(player);
    std::string nisModel(player.nisModel, strnlen(player.nisModel, sizeof(player.nisModel)));
    bool outfit = hasPlayer && !nisModel.empty() &&
        std::filesystem::exists(nis::NisFolder(game) / "characters" / std::filesystem::u8path(nisModel + ".p3d"));
    ImGui::BeginDisabled(!link || !built || Dirty() || !hasPlayer);

    if (ImGui::Button((std::string(icon::Video) + "  Play in game").c_str(), ImVec2(-FLT_MIN, 0)))
    {
        if (outfit)
        {
            link->RunScript("PlayNIS( \"" + scene + "\" );");
        }
        else
        {
            link->RunScript("PlayNIS( \"" + scene +
                "\", \"\", \"false\", \"0 0 0\", \"0 0 1\", \"false\", \"false\", \"false\", \"true\", \"true\", "
                "\"false\" );");
        }
    }

    ImGui::EndDisabled();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        if (!link)
        {
            ImGui::SetTooltip("Start the game with ToolKitLink.asi in plugins/ and load a save");
        }
        else if (!built)
        {
            ImGui::SetTooltip("Build the scene into the game first");
        }
        else if (Dirty())
        {
            ImGui::SetTooltip("Save first: the game reads the file");
        }
        else if (!hasPlayer)
        {
            ImGui::SetTooltip("Waiting for the game to report the main character");
        }
        else
        {
            ImGui::SetTooltip("Runs PlayNIS( \"%s\" ); in the running game", scene.c_str());
        }
    }

    if (hasPlayer)
    {
        ImGui::TextDisabled("Tony at %.1f, %.1f, %.1f", player.x, player.y, player.z);

        if (outfit)
        {
            ImGui::TextDisabled("NIS outfit: %s", nisModel.c_str());
        }
        else
        {
            ImGui::TextDisabled("No NIS outfit for this character: the scene must carry Tony's model");
        }
    }
}

void NisWorkspace::OpenBuild()
{
    m_build = nis::BuildOptions();
    m_build.scene = nis::SceneName(m_roots);
    m_build.assetsFrom = m_assetsFrom;
    m_buildKey.clear();
    m_buildDone = false;
    m_openBuildPopup = true;
}

void NisWorkspace::DrawBuildPopup()
{
    if (m_openBuildPopup)
    {
        ImGui::OpenPopup("Build into game");
        m_openBuildPopup = false;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(680.0f, 0.0f), ImVec2(680.0f, FLT_MAX));

    if (!ImGui::BeginPopupModal("Build into game", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    if (m_buildDone)
    {
        DrawBuildResult();
        return ImGui::EndPopup();
    }

    ImGui::TextUnformatted("Game folder");
    DrawGameFolder();
    ImGui::Spacing();

    if (ImGui::BeginTable("options", 2, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Scene name");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        InputString("##scene", m_build.scene);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Animations from");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        DrawSceneChooser("##assets", m_build.assetsFrom);
        ImGui::EndTable();
    }

    ImGui::Checkbox("Copy the dialogue script", &m_build.dialogue);

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("The source scene's dialogue audio and its timing, under the new name");
    }

    ImGui::SameLine();
    ImGui::Checkbox("List in callnis.txt", &m_build.listInCallnis);

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("The game's debug NIS list; the original is kept as callnis.txt.orig");
    }

    ImGui::SameLine();
    ImGui::Checkbox("Overwrite", &m_build.overwrite);

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Replace art/nis/<scene name>/ if it already exists");
    }

    std::string key = GameFolder().u8string() + "|" + m_build.scene + "|" + m_build.assetsFrom + "|" +
        std::to_string(m_build.dialogue) + std::to_string(m_build.listInCallnis) + std::to_string(m_build.overwrite);
    if (key != m_buildKey)
    {
        m_buildKey = key;
        m_buildPlan = nis::PlanBuild(GameFolder(), m_build);
    }

    ImGui::SeparatorText("Files");

    if (!m_buildPlan.error.empty())
    {
        ImGui::TextColored(kErrorColour, "%s", m_buildPlan.error.c_str());
    }
    else
    {
        if (m_buildPlan.inPlace)
        {
            ImGui::TextDisabled("Same name as the source: its art stays, only the script is written.");
        }

        float height = std::min(ImGui::GetTextLineHeightWithSpacing() * (m_buildPlan.writes.size() + 1), 220.0f);
        ImGui::BeginChild("writes", ImVec2(0, height), true);

        for (const auto& path : m_buildPlan.writes)
        {
            bool exists = std::filesystem::exists(path);
            ImGui::TextUnformatted(path.lexically_relative(GameFolder()).u8string().c_str());
            ImGui::SameLine();
            ImGui::TextDisabled(exists ? "replace" : "new");
        }

        ImGui::EndChild();

        for (const std::string& note : m_buildPlan.notes)
        {
            ImGui::TextDisabled("%s", note.c_str());
        }
    }

    ImGui::Spacing();
    ImGui::BeginDisabled(!m_buildPlan.error.empty());

    if (ImGui::Button("Build", ImVec2(120, 0)))
    {
        nis::BuildPlan result = nis::Build(GameFolder(), m_build, m_roots);

        if (!result.error.empty())
        {
            m_buildPlan = result;
            m_buildKey.clear();
            m_actions.error("Build failed: " + result.error);
        }
        else
        {
            if (nis::SceneName(m_roots) != m_build.scene)
            {
                BeginChange(true);
                nis::RenameScene(m_roots, m_build.scene);
            }

            m_path = nis::NisFolder(GameFolder()) / "fsc" / std::filesystem::u8path(m_build.scene + ".fsc");
            m_savedDepth = m_undo.size();
            m_editOpen = false;
            SetAssetsFrom(m_build.scene);
            m_buildPlan = result;
            m_buildDone = true;
        }
    }

    ImGui::EndDisabled();
    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void NisWorkspace::DrawBuildResult()
{
    ImGui::TextColored(kDoneColour,
        "%s  Built %s (%zu files).",
        icon::Save,
        m_build.scene.c_str(),
        m_buildPlan.writes.size());
    for (const std::string& note : m_buildPlan.notes)
    {
        ImGui::TextDisabled("%s", note.c_str());
    }

    ImGui::Spacing();
    ImGui::TextWrapped(
        "The game plays a NIS by name. Call this from a mission script (the way A22 plays MNIS_A23_0_Intro_To_Tranq), "
        "or pick it from the debug NIS list:");
    std::string call = "PlayNIS( \"" + m_build.scene + "\" );";
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80.0f);
    ImGui::InputText("##call", call.data(), call.size() + 1, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();

    if (ImGui::Button("Copy", ImVec2(-FLT_MIN, 0)))
    {
        ImGui::SetClipboardText(call.c_str());
    }

    DrawPlayInGame();
    ImGui::Spacing();

    if (ImGui::Button("Close", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape) ||
        ImGui::IsKeyPressed(ImGuiKey_Enter))
    {
        ImGui::CloseCurrentPopup();
    }
}

}
