#include "ui/NisWorkspace.hh"

#include "app/FileDialogs.hh"
#include "core/FileIo.hh"
#include "ui/Icons.hh"
#include "ui/Widgets.hh"

#include <algorithm>
#include <cctype>

namespace tk
{

namespace
{

const ImVec4 kErrorColour(0.95f, 0.35f, 0.30f, 1.0f);
const ImVec4 kWarningColour(0.95f, 0.70f, 0.30f, 1.0f);
const ImVec4 kInfoColour(0.60f, 0.62f, 0.66f, 1.0f);

std::string Lower(std::string text)
{
    std::transform(text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
    return text;
}

nis::NodePath ParentOf(const nis::NodePath& path)
{
    return nis::NodePath(path.begin(), path.end() - (path.empty() ? 0 : 1));
}

std::vector<fsc::Node>& SiblingsOf(std::vector<fsc::Node>& roots, const nis::NodePath& path)
{
    if (path.size() <= 1)
    {
        return roots;
    }

    return nis::Resolve(roots, ParentOf(path))->children;
}

void UpdateParentCounts(std::vector<fsc::Node>& roots, const nis::NodePath& path)
{
    if (path.size() > 1)
    {
        nis::UpdateCounts(*nis::Resolve(roots, ParentOf(path)));
    }
}

fsc::Node MakeFightNode(const std::string& name)
{
    fsc::Node node = fsc::Node::Block("fightNode");
    node.SetString("name", name);
    node.SetInt("trackCount", 0);
    node.SetInt("branchCount", 0);
    node.children.push_back(nis::MakeConditionGroup());
    return node;
}

std::string UniqueName(const std::vector<fsc::Node>& siblings, const std::string& base)
{
    auto taken = [&](const std::string& name)
    {
        for (const fsc::Node& sibling : siblings)
        {
            if (nis::IsBranch(sibling) && sibling.String("name") == name)
            {
                return true;
            }
        }

        return false;
    };

    if (!taken(base))
    {
        return base;
    }

    for (int i = 2;; ++i)
    {
        if (!taken(base + std::to_string(i)))
        {
            return base + std::to_string(i);
        }
    }
}

}

ImU32 NisCategoryColour(nis::Category category, int alpha)
{
    switch (category)
    {
    case nis::Category::Camera:
        return IM_COL32(110, 170, 255, alpha);
    case nis::Category::Character:
        return IM_COL32(120, 210, 140, alpha);
    case nis::Category::Weapon:
        return IM_COL32(235, 110, 90, alpha);
    case nis::Category::Vehicle:
        return IM_COL32(240, 170, 70, alpha);
    case nis::Category::Prop:
        return IM_COL32(190, 140, 240, alpha);
    case nis::Category::Screen:
        return IM_COL32(200, 200, 200, alpha);
    case nis::Category::Sound:
        return IM_COL32(90, 200, 210, alpha);
    case nis::Category::Script:
        return IM_COL32(212, 175, 55, alpha);
    case nis::Category::Flow:
        return IM_COL32(240, 120, 190, alpha);
    }

    return IM_COL32(160, 160, 160, alpha);
}

NisWorkspace::NisWorkspace(HWND owner, NisActions actions) : m_owner(owner), m_actions(std::move(actions))
{
}

bool NisWorkspace::Accepts(const std::filesystem::path& path)
{
    return Lower(path.extension().u8string()) == ".fsc";
}

std::string NisWorkspace::Title() const
{
    if (!m_open)
    {
        return "no scene";
    }

    std::string name = m_path.empty() ? nis::SceneName(m_roots) + ".fsc" : m_path.filename().u8string();
    return Dirty() ? name + " *" : name;
}

bool NisWorkspace::Open(const std::filesystem::path& path)
{
    if (!ConfirmDiscard())
    {
        return false;
    }

    std::optional<std::string> text = ReadText(path);

    if (!text)
    {
        m_actions.error("Cannot read " + path.u8string());
        return false;
    }

    std::vector<fsc::Node> roots;
    std::string error;

    if (!fsc::Parse(*text, roots, error))
    {
        m_actions.error(path.filename().u8string() + ": " + error);
        return false;
    }

    Reset(std::move(roots), path);
    return true;
}

void NisWorkspace::New()
{
    if (!ConfirmDiscard())
    {
        return;
    }

    m_newName = "MNIS_New_Scene";
    m_openNewPopup = true;
}

void NisWorkspace::Reset(std::vector<fsc::Node> roots, const std::filesystem::path& path)
{
    m_roots = std::move(roots);
    m_path = path;
    m_open = true;
    m_undo.clear();
    m_savedDepth = 0;
    m_redo.clear();
    m_editOpen = false;
    m_track = -1;
    m_node.clear();

    for (const nis::SceneNode& node : nis::ListNodes(m_roots))
    {
        if (node.isBank)
        {
            continue;
        }

        if (m_node.empty() || node.name == "start")
        {
            m_node = node.path;
        }

        if (node.name == "start")
        {
            break;
        }
    }

    m_assetsFrom.clear();
    ReloadAssets();

    if (m_assets.found)
    {
        m_assetsFrom = m_assets.folder.filename().u8string();
    }

    ++m_revision;
}

void NisWorkspace::ReloadAssets()
{
    if (!m_assetsFrom.empty() && !GameFolder().empty())
    {
        m_assets = nis::LoadSceneFolder(nis::NisFolder(GameFolder()) / std::filesystem::u8path(m_assetsFrom));
    }
    else
    {
        m_assets = m_path.empty() ? nis::SceneAssets() : nis::LoadAssets(m_path);
    }

    ++m_revision;
}

std::filesystem::path NisWorkspace::GameFolder() const
{
    return m_actions.gameFolder ? m_actions.gameFolder() : std::filesystem::path();
}

void NisWorkspace::SetAssetsFrom(const std::string& scene)
{
    m_assetsFrom = scene;
    ReloadAssets();
}

void NisWorkspace::Save()
{
    if (!m_open)
    {
        return;
    }

    if (m_path.empty())
    {
        return SaveAs();
    }

    SaveTo(m_path);
}

void NisWorkspace::SaveAs()
{
    if (!m_open)
    {
        return;
    }

    std::filesystem::path suggested =
        m_path.empty() ? std::filesystem::u8path(nis::SceneName(m_roots) + ".fsc") : m_path;
    if (auto path = ShowSaveSceneDialog(m_owner, suggested))
    {
        SaveTo(*path);
    }
}

bool NisWorkspace::SaveTo(const std::filesystem::path& path)
{
    std::string text = fsc::Write(m_roots);

    if (!WriteFile(path, Bytes(text.begin(), text.end())))
    {
        m_actions.error("Cannot write " + path.u8string());
        return false;
    }

    bool moved = path != m_path;
    m_path = path;
    m_savedDepth = m_undo.size();
    m_editOpen = false;

    if (moved)
    {
        ReloadAssets();
    }

    return true;
}

bool NisWorkspace::ConfirmDiscard()
{
    if (!Dirty())
    {
        return true;
    }

    std::wstring prompt = L"Save changes to " + std::filesystem::u8path(Title()).wstring() + L"?";
    int answer = MessageBoxW(m_owner, prompt.c_str(), L"ToolKit", MB_YESNOCANCEL | MB_ICONQUESTION);

    if (answer == IDCANCEL)
    {
        return false;
    }

    if (answer == IDYES)
    {
        Save();
        return !Dirty();
    }

    return true;
}

void NisWorkspace::BeginChange(bool discrete)
{
    if (discrete || !m_editOpen)
    {
        if (m_undo.size() < m_savedDepth)
        {
            m_savedDepth = kNeverSaved;
        }

        m_undo.push_back(m_roots);

        if (m_undo.size() > kUndoLimit)
        {
            m_undo.erase(m_undo.begin());
            m_savedDepth = m_savedDepth == 0 || m_savedDepth == kNeverSaved ? kNeverSaved : m_savedDepth - 1;
        }

        m_redo.clear();
    }
    else if (m_undo.size() == m_savedDepth)
    {
        m_savedDepth = kNeverSaved;
    }

    m_editOpen = !discrete;
    ++m_revision;
}

void NisWorkspace::Undo()
{
    if (m_undo.empty())
    {
        return;
    }

    m_redo.push_back(std::move(m_roots));
    m_roots = std::move(m_undo.back());
    m_undo.pop_back();
    m_editOpen = false;
    ++m_revision;
    ClampSelection();
}

void NisWorkspace::Redo()
{
    if (m_redo.empty())
    {
        return;
    }

    m_undo.push_back(std::move(m_roots));
    m_roots = std::move(m_redo.back());
    m_redo.pop_back();
    m_editOpen = false;
    ++m_revision;
    ClampSelection();
}

void NisWorkspace::Refresh()
{
    if (m_cacheRevision == m_revision)
    {
        return;
    }

    m_cacheRevision = m_revision;
    ClampSelection();
    m_nodes = nis::ListNodes(m_roots);
    m_cast = nis::CollectCast(m_roots);
    m_issues = nis::Validate(m_roots, m_assets);
    m_source = fsc::Write(m_roots);
    m_sceneName = nis::SceneName(m_roots);
    m_sceneFrames = m_assets.Frames();

    for (const nis::SceneNode& node : m_nodes)
    {
        if (node.isBank)
        {
            continue;
        }

        const fsc::Node& block = *nis::Resolve(m_roots, node.path);

        for (size_t index : nis::TrackIndices(block))
        {
            nis::Timing timing = nis::TrackTiming(block.children[index], m_assets);
            m_sceneFrames = std::max(m_sceneFrames, timing.begin + timing.length);
        }
    }
}

void NisWorkspace::ClampSelection()
{
    const fsc::Node* node = nis::Resolve(m_roots, m_node);

    if (!node || node->key != "fightNode")
    {
        m_node.clear();

        for (const nis::SceneNode& candidate : nis::ListNodes(m_roots))
        {
            if (!candidate.isBank)
            {
                m_node = candidate.path;
                break;
            }
        }

        node = nis::Resolve(m_roots, m_node);
    }

    if (!node || m_track < 0 || m_track >= static_cast<int>(node->children.size()) ||
        !nis::IsTrack(node->children[m_track]))
    {
        m_track = -1;
    }
}

void NisWorkspace::SelectNode(const nis::NodePath& path, int track)
{
    const fsc::Node* node = nis::Resolve(m_roots, path);

    if (!node || node->key != "fightNode")
    {
        return;
    }

    m_node = path;
    m_track = track;
    m_revealTrack = track >= 0;
    ClampSelection();
}

fsc::Node* NisWorkspace::SelectedNode()
{
    fsc::Node* node = nis::Resolve(m_roots, m_node);
    return node && node->key == "fightNode" ? node : nullptr;
}

const fsc::Node* NisWorkspace::SelectedNode() const
{
    return const_cast<NisWorkspace*>(this)->SelectedNode();
}

fsc::Node* NisWorkspace::SelectedTrack()
{
    fsc::Node* node = SelectedNode();

    if (!node || m_track < 0 || m_track >= static_cast<int>(node->children.size()))
    {
        return nullptr;
    }

    fsc::Node& track = node->children[m_track];
    return nis::IsTrack(track) ? &track : nullptr;
}

const fsc::Node* NisWorkspace::SelectedTrack() const
{
    return const_cast<NisWorkspace*>(this)->SelectedTrack();
}

std::vector<std::string> NisWorkspace::Suggestions(nis::Ref ref) const
{
    std::vector<std::string> items;
    auto animations = [&](const char* type, bool exclude)
    {
        for (const auto& entry : m_assets.animations)
        {
            if ((entry.second.type == type) != exclude)
            {
                items.push_back(entry.second.name);
            }
        }
    };

    switch (ref)
    {
    case nis::Ref::Character:
        for (const nis::CastMember& member : m_cast.characters)
        {
            items.push_back(member.name);
        }
        break;
    case nis::Ref::Vehicle:
        for (const nis::CastMember& member : m_cast.vehicles)
        {
            items.push_back(member.name);
        }
        break;
    case nis::Ref::Object:
        items = m_cast.objects;
        break;
    case nis::Ref::BodyAnimation:
        animations("PTRN", false);
        break;
    case nis::Ref::FaceAnimation:
        animations("EXP", false);
        break;
    case nis::Ref::CameraAnimation:
        animations("CAM", false);
        break;
    case nis::Ref::VehicleAnimation:
        for (const auto& entry : m_assets.animations)
        {
            if (entry.second.type != "CAM" && entry.second.type != "EXP")
            {
                items.push_back(entry.second.name);
            }
        }
        break;
    case nis::Ref::CharacterTemplate:
        for (const nis::CastMember& member : m_cast.characters)
        {
            if (!member.detail.empty() && member.name != "MainCharacter" &&
                std::find(items.begin(), items.end(), member.detail) == items.end())
            {
                items.push_back(member.detail);
            }
        }
        break;
    case nis::Ref::Branch:
        for (const nis::SceneNode& node : m_nodes)
        {
            if (!node.isBank && node.path != m_node && ParentOf(node.path) == ParentOf(m_node))
            {
                items.push_back("../" + node.name);
            }
        }
        break;
    default:
        items = nis::KnownValues(ref);
        break;
    }

    return items;
}

void NisWorkspace::AddTrack(const nis::TrackSpec& spec)
{
    fsc::Node* node = SelectedNode();

    if (!node)
    {
        return;
    }

    BeginChange(true);
    size_t at = m_track >= 0 ? static_cast<size_t>(m_track) + 1 : node->children.size();
    node->children.insert(node->children.begin() + static_cast<std::ptrdiff_t>(at), nis::MakeTrack(spec));
    nis::UpdateCounts(*node);
    m_track = static_cast<int>(at);
    m_revealTrack = true;
}

void NisWorkspace::AddAnimationTrack(const nis::AnimationInfo& animation)
{
    const char* type = animation.type == "CAM" ? "NISCameraAnimation"
        : animation.type == "EXP"              ? "NISExpAnimation"
                                               : "NISCharacterAnimation";
    AddTrack(*nis::FindTrack(type));
    fsc::Node* track = SelectedTrack();

    if (!track)
    {
        return;
    }

    if (animation.type == "CAM")
    {
        track->SetString("CameraAnimationName", animation.name);
        return;
    }

    track->SetString("AnimationName", animation.name);
    std::string actor = Lower(animation.name.substr(animation.name.find_last_of('_') + 1));

    for (const nis::CastMember& member : m_cast.characters)
    {
        if (Lower(member.name) == actor || (actor == "tony" && member.name == "MainCharacter"))
        {
            track->SetString("CharacterName", member.name);
            break;
        }
    }
}

void NisWorkspace::DuplicateTrack()
{
    fsc::Node* node = SelectedNode();

    if (!SelectedTrack())
    {
        return;
    }

    BeginChange(true);
    fsc::Node copy = node->children[m_track];
    node->children.insert(node->children.begin() + m_track + 1, std::move(copy));
    nis::UpdateCounts(*node);
    ++m_track;
    m_revealTrack = true;
}

void NisWorkspace::DeleteTrack()
{
    fsc::Node* node = SelectedNode();

    if (!SelectedTrack())
    {
        return;
    }

    BeginChange(true);
    node->children.erase(node->children.begin() + m_track);
    nis::UpdateCounts(*node);
    ClampSelection();
}

void NisWorkspace::MoveTrack(int direction)
{
    fsc::Node* node = SelectedNode();

    if (!SelectedTrack())
    {
        return;
    }

    std::vector<size_t> tracks = nis::TrackIndices(*node);
    auto at = std::find(tracks.begin(), tracks.end(), static_cast<size_t>(m_track));
    std::ptrdiff_t position = at - tracks.begin() + direction;

    if (position < 0 || position >= static_cast<std::ptrdiff_t>(tracks.size()))
    {
        return;
    }

    BeginChange(true);
    size_t other = tracks[static_cast<size_t>(position)];
    std::swap(node->children[m_track], node->children[other]);
    m_track = static_cast<int>(other);
    m_revealTrack = true;
}

void NisWorkspace::AddNode()
{
    if (m_node.empty())
    {
        return;
    }

    BeginChange(true);
    std::vector<fsc::Node>& siblings = SiblingsOf(m_roots, m_node);
    size_t at = m_node.back() + 1;
    siblings.insert(siblings.begin() + static_cast<std::ptrdiff_t>(at), MakeFightNode(UniqueName(siblings, "node")));
    UpdateParentCounts(m_roots, m_node);
    m_node.back() = at;
    m_track = -1;
}

void NisWorkspace::DuplicateNode(const nis::NodePath& path)
{
    BeginChange(true);
    std::vector<fsc::Node>& siblings = SiblingsOf(m_roots, path);
    fsc::Node copy = siblings[path.back()];
    copy.SetString("name", UniqueName(siblings, copy.String("name") + "_copy"));
    siblings.insert(siblings.begin() + static_cast<std::ptrdiff_t>(path.back() + 1), std::move(copy));
    UpdateParentCounts(m_roots, path);
    m_node = path;
    m_node.back() += 1;
    m_track = -1;
}

void NisWorkspace::DeleteNode(const nis::NodePath& path)
{
    BeginChange(true);
    std::vector<fsc::Node>& siblings = SiblingsOf(m_roots, path);
    siblings.erase(siblings.begin() + static_cast<std::ptrdiff_t>(path.back()));
    UpdateParentCounts(m_roots, path);

    if (m_node == path)
    {
        m_node.clear();
    }

    m_track = -1;
    ClampSelection();
}

void NisWorkspace::RenameNode(const nis::NodePath& path, const std::string& name)
{
    fsc::Node* target = nis::Resolve(m_roots, path);

    if (!target || name.empty() || target->String("name") == name)
    {
        return;
    }

    BeginChange(true);

    for (const nis::SceneNode& node : nis::ListNodes(m_roots))
    {
        if (node.isBank)
        {
            continue;
        }

        fsc::Node& block = *nis::Resolve(m_roots, node.path);

        for (size_t index : nis::TrackIndices(block))
        {
            fsc::Node& track = block.children[index];

            if (track.key != "sequence")
            {
                continue;
            }

            std::string branch = track.String("branch");

            if (nis::ResolveBranch(m_roots, node.path, branch) != path)
            {
                continue;
            }

            size_t slash = branch.find_last_of('/');
            track.SetString("branch",
                (slash == std::string::npos ? std::string() : branch.substr(0, slash + 1)) + name);
        }
    }

    target->SetString("name", name);
}

void NisWorkspace::Draw()
{
    if (m_open && m_detectedFor != m_path)
    {
        m_detectedFor = m_path;

        if (GameFolder().empty())
        {
            if (auto detected = nis::DetectGameFolder(m_path); !detected.empty())
            {
                m_actions.setGameFolder(detected);

                if (!m_assetsFrom.empty())
                {
                    ReloadAssets();
                }
            }
        }
    }

    Refresh();
    DrawScene();
    DrawTimeline();
    DrawTrack();
    DrawProblems();
    DrawSource();
    DrawNewScenePopup();
    DrawRenamePopup();
    DrawBuildPopup();
    HandleKeys();

    if (!ImGui::IsAnyItemActive() && !m_dragging)
    {
        m_editOpen = false;
    }
}

void NisWorkspace::DrawEmpty(const char* window)
{
    ImGui::TextDisabled("No NIS scene open.");
    ImGui::Spacing();
    ImGui::TextWrapped("Open a .fsc from art/nis/fsc (File > Open or drop it on the window), or start a new scene.");
    ImGui::Spacing();
    ImGui::PushID(window);

    if (ImGui::Button("New scene..."))
    {
        New();
    }

    ImGui::PopID();
}

void NisWorkspace::DrawScene()
{
    if (!ImGui::Begin(SceneWindow()))
    {
        return ImGui::End();
    }

    if (!m_open)
    {
        DrawEmpty(SceneWindow());
        return ImGui::End();
    }

    ImGui::SetNextItemWidth(-FLT_MIN);
    InputString("##scene", m_sceneName);

    if (ImGui::IsItemDeactivatedAfterEdit() && !m_sceneName.empty() && m_sceneName != nis::SceneName(m_roots))
    {
        BeginChange(true);
        nis::RenameScene(m_roots, m_sceneName);
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Scene name (the fightBank names)");
    }

    ImGui::TextDisabled("%s", m_path.empty() ? "not saved yet" : m_path.u8string().c_str());

    if (ImGui::Button((std::string(icon::Upload) + "  Build into game...").c_str(), ImVec2(-FLT_MIN, 0)))
    {
        OpenBuild();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Write this scene into the game folder under any name, with the animations and models of a "
                          "scene of your choice");
    }

    DrawPlayInGame();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Nodes", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawNodes();
    }

    if (ImGui::CollapsingHeader("Cast", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawCast();
    }

    if (ImGui::CollapsingHeader("Animations", ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawAnimations();
    }

    ImGui::End();
}

void NisWorkspace::DrawNodes()
{
    std::function<void()> action;

    for (size_t i = 0; i < m_nodes.size(); ++i)
    {
        const nis::SceneNode& node = m_nodes[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + node.depth * ImGui::GetStyle().IndentSpacing * 0.6f);

        if (node.isBank)
        {
            ImGui::TextDisabled("%s  %s", icon::Folder, node.name.c_str());
            ImGui::PopID();
            continue;
        }

        const fsc::Node& block = *nis::Resolve(m_roots, node.path);
        size_t problems = 0;

        for (const nis::Issue& issue : m_issues)
        {
            if (issue.node == node.path && issue.severity != nis::Severity::Info)
            {
                ++problems;
            }
        }

        std::string label = std::string(icon::Video) + "  " + node.name;

        if (ImGui::Selectable(label.c_str(), m_node == node.path))
        {
            SelectNode(node.path);
        }

        if (ImGui::BeginPopupContextItem("node"))
        {
            nis::NodePath path = node.path;

            if (ImGui::MenuItem("Rename..."))
            {
                m_renamePath = path;
                m_renameText = node.name;
                m_openRenamePopup = true;
            }

            if (ImGui::MenuItem("Duplicate"))
            {
                action = [this, path]
                {
                    DuplicateNode(path);
                };
            }

            if (ImGui::MenuItem("Delete"))
            {
                action = [this, path]
                {
                    DeleteNode(path);
                };
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Add node after"))
            {
                action = [this, path]
                {
                    m_node = path;
                    AddNode();
                };
            }

            ImGui::EndPopup();
        }

        char count[32];
        std::snprintf(count, sizeof(count), "%zu", nis::TrackIndices(block).size());
        float right = ImGui::GetWindowContentRegionMax().x;
        ImGui::SameLine(right - ImGui::CalcTextSize(count).x);
        ImGui::TextDisabled("%s", count);

        if (problems)
        {
            ImGui::SameLine(right - ImGui::CalcTextSize(count).x - ImGui::CalcTextSize("!  ").x);
            ImGui::TextColored(kWarningColour, "!");

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%zu problem(s), see the Problems panel", problems);
            }
        }

        ImGui::PopID();
    }

    if (ImGui::SmallButton((std::string(icon::Add) + " Node").c_str()))
    {
        action = [this]
        {
            AddNode();
        };
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Add an empty node after the selected one");
    }

    if (action)
    {
        action();
    }
}

void NisWorkspace::DrawCast()
{
    auto copyable = [](const std::string& text)
    {
        ImGui::TextUnformatted(text.c_str());

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Click to copy");
        }

        if (ImGui::IsItemClicked())
        {
            ImGui::SetClipboardText(text.c_str());
        }
    };
    ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("characters", 2, flags))
    {
        ImGui::TableSetupColumn("Character");
        ImGui::TableSetupColumn("Template");
        ImGui::TableHeadersRow();

        for (const nis::CastMember& member : m_cast.characters)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            copyable(member.name);
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", member.detail.c_str());
        }

        ImGui::EndTable();
    }

    if (!m_cast.vehicles.empty() && ImGui::BeginTable("vehicles", 2, flags))
    {
        ImGui::TableSetupColumn("Vehicle");
        ImGui::TableSetupColumn("Type");
        ImGui::TableHeadersRow();

        for (const nis::CastMember& member : m_cast.vehicles)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            copyable(member.name);
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", member.detail.c_str());
        }

        ImGui::EndTable();
    }

    if (!m_cast.objects.empty())
    {
        ImGui::TextDisabled("Objects");

        for (const std::string& object : m_cast.objects)
        {
            copyable(object);
        }
    }
}

void NisWorkspace::DrawAnimations()
{
    if (GameFolder().empty())
    {
        ImGui::TextWrapped("Set the game folder to take animations from any scene.");
        DrawGameFolder();
    }
    else
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("From");
        ImGui::SameLine();
        float reload =
            ImGui::CalcTextSize("Reload").x + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - reload);
        std::string source = m_assetsFrom;

        if (DrawSceneChooser("##from", source))
        {
            SetAssetsFrom(source);
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Scene folder whose animations and models this scene uses; Build copies them");
        }

        ImGui::SameLine();
    }

    if (ImGui::Button("Reload"))
    {
        ReloadAssets();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Read the stream files again");
    }

    if (!m_assets.found)
    {
        ImGui::TextWrapped("No animations: pick the scene to take them from.");
        return;
    }

    ImGui::TextDisabled("%d streams%s", m_assets.streams, m_assets.loader ? "" : ", no loader .p3d");

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", m_assets.folder.u8string().c_str());
    }

    ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;

    if (!ImGui::BeginTable("animations", 3, flags))
    {
        return;
    }

    ImGui::TableSetupColumn("Animation", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Length", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableHeadersRow();
    const nis::AnimationInfo* add = nullptr;
    int row = 0;

    for (const auto& entry : m_assets.animations)
    {
        const nis::AnimationInfo& animation = entry.second;
        ImGui::PushID(row++);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Selectable(animation.name.c_str(),
            false,
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Double click to add it to the selected node");
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            add = &animation;
        }

        if (ImGui::BeginPopupContextItem("animation"))
        {
            if (ImGui::MenuItem("Add track to the selected node"))
            {
                add = &animation;
            }

            if (ImGui::MenuItem("Copy name"))
            {
                ImGui::SetClipboardText(animation.name.c_str());
            }

            ImGui::EndPopup();
        }

        ImGui::TableNextColumn();
        ImGui::TextDisabled("%s", animation.type.c_str());
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%.1f s", animation.frames / nis::kFramesPerSecond);
        ImGui::PopID();
    }

    ImGui::EndTable();

    if (add)
    {
        AddAnimationTrack(*add);
    }
}

void NisWorkspace::DrawProblems()
{
    if (!ImGui::Begin(ProblemsWindow()))
    {
        return ImGui::End();
    }

    if (!m_open)
    {
        ImGui::TextDisabled("No NIS scene open.");
        return ImGui::End();
    }

    size_t errors = 0, warnings = 0;

    for (const nis::Issue& issue : m_issues)
    {
        if (issue.severity == nis::Severity::Error)
        {
            ++errors;
        }
        else if (issue.severity == nis::Severity::Warning)
        {
            ++warnings;
        }
    }

    if (m_issues.empty())
    {
        ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.45f, 1.0f), "No problems found.");
    }
    else
    {
        ImGui::TextDisabled("%zu errors, %zu warnings, %zu notes",
            errors,
            warnings,
            m_issues.size() - errors - warnings);
    }

    ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;

    if (!m_issues.empty() && ImGui::BeginTable("problems", 3, flags))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Node", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Problem", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        std::function<void()> action;

        for (size_t i = 0; i < m_issues.size(); ++i)
        {
            const nis::Issue& issue = m_issues[i];
            ImGui::PushID(static_cast<int>(i));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const ImVec4& colour = issue.severity == nis::Severity::Error ? kErrorColour
                : issue.severity == nis::Severity::Warning                ? kWarningColour
                                                                          : kInfoColour;
            const char* kind = issue.severity == nis::Severity::Error ? "error"
                : issue.severity == nis::Severity::Warning            ? "warning"
                                                                      : "note";
            if (ImGui::Selectable("##issue", false, ImGuiSelectableFlags_SpanAllColumns))
            {
                action = [this, issue]
                {
                    SelectNode(issue.node, issue.track);
                };
            }

            ImGui::SameLine();
            ImGui::TextColored(colour, "%s", kind);
            ImGui::TableNextColumn();
            const fsc::Node* node = nis::Resolve(m_roots, issue.node);
            ImGui::TextDisabled("%s", node ? node->String("name").c_str() : "scene");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(issue.message.c_str());
            ImGui::PopID();
        }

        ImGui::EndTable();

        if (action)
        {
            action();
        }
    }

    ImGui::End();
}

void NisWorkspace::DrawSource()
{
    if (!ImGui::Begin(SourceWindow()))
    {
        return ImGui::End();
    }

    if (!m_open)
    {
        ImGui::TextDisabled("No NIS scene open.");
        return ImGui::End();
    }

    if (ImGui::SmallButton("Copy all"))
    {
        ImGui::SetClipboardText(m_source.c_str());
    }

    ImGui::SameLine();
    ImGui::TextDisabled("This is exactly what Save writes.");
    ImGui::InputTextMultiline("##source",
        m_source.data(),
        m_source.size() + 1,
        ImVec2(-FLT_MIN, -FLT_MIN),
        ImGuiInputTextFlags_ReadOnly);
    ImGui::End();
}

void NisWorkspace::DrawNewScenePopup()
{
    if (m_openNewPopup)
    {
        ImGui::OpenPopup("New NIS scene");
        m_openNewPopup = false;
    }

    if (!ImGui::BeginPopupModal("New NIS scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    ImGui::TextUnformatted("Scene name");
    ImGui::SetNextItemWidth(360.0f);

    if (ImGui::IsWindowAppearing())
    {
        ImGui::SetKeyboardFocusHere();
    }

    InputString("##name", m_newName);
    bool enter = ImGui::IsItemDeactivated() && ImGui::IsKeyPressed(ImGuiKey_Enter);
    ImGui::TextDisabled("The file, the art/nis/<name>/ folder and the CAM_<name>_Cam animation share this name.");
    bool valid = !m_newName.empty() && m_newName.find_first_of(" \t\"{};/\\") == std::string::npos;
    ImGui::BeginDisabled(!valid);

    if (ImGui::Button("Create", ImVec2(120, 0)) || (enter && valid))
    {
        Reset(nis::MakeSceneTemplate(m_newName), {});
        m_savedDepth = kNeverSaved;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndDisabled();
    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void NisWorkspace::DrawRenamePopup()
{
    if (m_openRenamePopup)
    {
        ImGui::OpenPopup("Rename node");
        m_openRenamePopup = false;
    }

    if (!ImGui::BeginPopupModal("Rename node", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    ImGui::SetNextItemWidth(260.0f);

    if (ImGui::IsWindowAppearing())
    {
        ImGui::SetKeyboardFocusHere();
    }

    InputString("##rename", m_renameText);
    bool enter = ImGui::IsItemDeactivated() && ImGui::IsKeyPressed(ImGuiKey_Enter);
    ImGui::TextDisabled("Branches that lead here are renamed too.");
    bool valid = !m_renameText.empty() && m_renameText.find_first_of(" \t\"{};/\\") == std::string::npos;
    ImGui::BeginDisabled(!valid);

    if (ImGui::Button("Rename", ImVec2(120, 0)) || (enter && valid))
    {
        RenameNode(m_renamePath, m_renameText);
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndDisabled();
    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void NisWorkspace::HandleKeys()
{
    ImGuiIO& io = ImGui::GetIO();

    if (!m_open || !m_timelineFocused || io.WantTextInput)
    {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        DeleteTrack();
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D))
    {
        DuplicateTrack();
    }

    if (io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
    {
        MoveTrack(-1);
    }

    if (io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
    {
        MoveTrack(1);
    }
}

}
