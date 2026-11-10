#include "ui/CharacterImportPanel.hh"

#include "app/FileDialogs.hh"
#include "app/Theme.hh"
#include "rig/FbxConvert.hh"
#include "rig/NewCharacter.hh"
#include "rig/PackageWriter.hh"

#include <imgui_internal.h>

#include <algorithm>
#include <cctype>

namespace tk
{

namespace
{

std::string Extension(const std::filesystem::path& path)
{
    std::string extension = path.extension().u8string();
    std::transform(extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
    return extension;
}

bool PrimaryButton(const char* label, bool enabled)
{
    ImGui::PushStyleColor(ImGuiCol_Button, theme::kGold);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(236, 200, 80, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(180, 145, 40, 255));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(20, 14, 10, 255));
    ImGui::BeginDisabled(!enabled);
    bool pressed = ImGui::Button(label, ImVec2(160, 0));
    ImGui::EndDisabled();
    ImGui::PopStyleColor(4);
    return pressed;
}

}

bool CharacterImportPanel::Accepts(const std::filesystem::path& path)
{
    std::string extension = Extension(path);
    return extension == ".glb" || extension == ".gltf" || extension == ".fbx";
}

void CharacterImportPanel::Open()
{
    m_open = true;
    m_focusFrames = 3;
}

void CharacterImportPanel::OpenModel(const std::filesystem::path& path)
{
    Open();

    if (m_step != Step::Rigging)
    {
        StartLoad(path);
    }
}

void CharacterImportPanel::Draw()
{
    if (!m_open)
    {
        return;
    }

    if (m_dockPending)
    {
        ImGuiWindow* viewport = ImGui::FindWindowByName("Viewport");

        if (viewport && viewport->DockId)
        {
            ImGui::SetNextWindowDockID(viewport->DockId, ImGuiCond_FirstUseEver);
        }

        m_dockPending = false;
    }

    if (m_focusFrames > 0)
    {
        ImGui::SetNextWindowFocus();
        --m_focusFrames;
    }

    if (ImGui::Begin("Character Import", &m_open))
    {
        PollConversion();
        PollRigging();
        DrawStepBar();
        float footer = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y * 2.0f;
        ImVec2 available = ImGui::GetContentRegionAvail();
        float bodyHeight = std::max(available.y - footer, 50.0f);
        ImGui::BeginChild("view",
            ImVec2(std::max(available.x - 320.0f - ImGui::GetStyle().ItemSpacing.x, 50.0f), bodyHeight),
            false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        DrawView();
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("info", ImVec2(0, bodyHeight), true);
        DrawInfo();
        ImGui::EndChild();
        DrawFooter();
    }

    ImGui::End();
}

void CharacterImportPanel::DrawStepBar()
{
    const char* skeletonLabel = m_session.Request().source == RigSource::Markers ? "Place markers" : "Map bones";
    const char* labels[] = {"Upload", "Orient", skeletonLabel, "Auto-rigging", "Review"};

    for (int i = 0; i < 5; ++i)
    {
        if (i > 0)
        {
            ImGui::SameLine();
            ImGui::TextDisabled(">");
            ImGui::SameLine();
        }

        bool current = static_cast<int>(m_step) == i;
        bool done = static_cast<int>(m_step) > i;
        ImU32 colour = current ? theme::kGold : done ? theme::kBone : IM_COL32(120, 110, 100, 255);
        ImGui::PushStyleColor(ImGuiCol_Text, colour);
        ImGui::Text("%d  %s", i + 1, labels[i]);
        ImGui::PopStyleColor();
    }

    ImGui::Separator();
}

void CharacterImportPanel::DrawFooter()
{
    ImGui::Separator();
    bool canBack = m_step == Step::Orient || m_step == Step::Skeleton || m_step == Step::Review;
    ImGui::BeginDisabled(!canBack || m_conversion.valid());

    if (ImGui::Button("Back", ImVec2(100, 0)))
    {
        GoBack();
    }

    ImGui::EndDisabled();

    if (MarkerView())
    {
        ImGui::SameLine(0, 40);
        ImGui::Checkbox("Use symmetry", &m_symmetry);
        ImGui::SameLine();

        if (ImGui::Button("Auto place"))
        {
            m_session.GuessMarkers();
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear"))
        {
            m_session.ClearMarkers();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Ctrl: no snap");
    }

    if (m_step == Step::Rigging)
    {
        return;
    }

    ImGui::SameLine(std::max(ImGui::GetWindowContentRegionMax().x - 160.0f, 120.0f));
    const char* label = m_step == Step::Review ? "Apply to open file" : "Next";

    if (PrimaryButton(label, CanGoNext()))
    {
        GoNext();
    }
}

bool CharacterImportPanel::CanGoNext() const
{
    if (m_conversion.valid())
    {
        return false;
    }

    switch (m_step)
    {
    case Step::Upload:
        return m_session.Loaded();
    case Step::Orient:
        return true;
    case Step::Skeleton:
        return m_session.Request().source != RigSource::Markers || m_session.AllMarkersPlaced();
    case Step::Review:
        return FindPackageTarget(*m_document.Root()).Valid();
    default:
        return false;
    }
}

void CharacterImportPanel::GoNext()
{
    switch (m_step)
    {
    case Step::Upload:
        m_step = Step::Orient;
        break;
    case Step::Orient:
        m_step = Step::Skeleton;
        break;
    case Step::Skeleton:
        StartRigging();
        break;
    case Step::Review:
        ApplyToDocument();
        break;
    default:
        break;
    }

    m_status.clear();
}

void CharacterImportPanel::GoBack()
{
    switch (m_step)
    {
    case Step::Orient:
        m_step = Step::Upload;
        break;
    case Step::Skeleton:
        m_step = Step::Orient;
        break;
    case Step::Review:
        m_session.DiscardRig();
        m_step = Step::Skeleton;
        break;
    default:
        break;
    }

    m_status.clear();
}

void CharacterImportPanel::PickModel()
{
    if (auto path = ShowOpenModelDialog(m_owner))
    {
        StartLoad(*path);
    }
}

void CharacterImportPanel::PickTexture()
{
    auto path = ShowOpenTextureDialog(m_owner);
    std::string error;

    if (path && !m_session.LoadTexture(*path, error))
    {
        m_status = error;
    }
}

void CharacterImportPanel::StartLoad(const std::filesystem::path& path)
{
    m_status.clear();

    if (Extension(path) != ".fbx")
    {
        std::string error;

        if (m_session.Load(path, error))
        {
            m_step = Step::Orient;
        }
        else
        {
            m_status = error;
        }

        return;
    }

    m_pendingPath = std::filesystem::temp_directory_path() / L"toolkit_import.glb";
    std::filesystem::path target = m_pendingPath;
    m_conversion = std::async(std::launch::async,
        [path, target]
        {
            std::string error;
            ConvertFbx(path, target, error);
            return error;
        });
}

void CharacterImportPanel::PollConversion()
{
    if (!m_conversion.valid() || m_conversion.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    {
        return;
    }

    std::string error = m_conversion.get();

    if (error.empty() && m_session.Load(m_pendingPath, error))
    {
        m_step = Step::Orient;
        return;
    }

    m_status = error;
}

GameSkeleton CharacterImportPanel::BaseSkeleton()
{
    PackageTarget target = FindPackageTarget(*m_document.Root());

    if (target.skeleton)
    {
        GameSkeleton skeleton = ReadGameSkeleton(*target.skeleton);

        if (FindJoint(skeleton, "Pelvis") >= 0 && FindJoint(skeleton, "Wrist_L") >= 0)
        {
            return skeleton;
        }
    }

    return DefaultGameSkeleton();
}

void CharacterImportPanel::StartRigging()
{
    RigModel model = m_session.Model();
    RigRequest request = m_session.Request();
    GameSkeleton base = BaseSkeleton();
    m_rigging = std::async(std::launch::async,
        [model = std::move(model), request = std::move(request), base = std::move(base)]
        {
            RigOutcome outcome;
            outcome.rig = BuildRig(model, request, base, outcome.error);
            return outcome;
        });
    m_riggingTime = 0.0f;
    m_orbit.Frame(m_scene.bounds);
    m_step = Step::Rigging;
}

void CharacterImportPanel::PollRigging()
{
    if (!m_rigging.valid())
    {
        return;
    }

    m_riggingTime += ImGui::GetIO().DeltaTime;

    if (m_riggingTime < 1.8f || m_rigging.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    {
        return;
    }

    RigOutcome outcome = m_rigging.get();

    if (!outcome.rig)
    {
        m_status = outcome.error;
        m_step = Step::Skeleton;
        return;
    }

    m_session.SetRig(std::move(outcome.rig));
    m_orbit.Frame(m_scene.bounds);
    m_posePhase = 0.0f;
    m_step = Step::Review;
}

void CharacterImportPanel::SaveAsNewCharacter()
{
    std::optional<GameFolders> folders =
        m_document.HasPath() ? GameFoldersFromPackage(m_document.Path()) : std::nullopt;
    if (!m_session.Rig() || !folders)
    {
        return;
    }

    std::unique_ptr<Chunk> package = m_document.Root()->Clone();
    std::string report;

    if (ApplyRigToPackage(*package,
            *m_session.Rig(),
            m_session.Texture(),
            static_cast<uint32_t>(m_textureSize),
            report))
    {
        SaveNewCharacter(*package, m_newName, *folders, report);
    }

    m_status = report;
}

void CharacterImportPanel::ApplyToDocument()
{
    if (!m_session.Rig() || !FindPackageTarget(*m_document.Root()).Valid())
    {
        return;
    }

    std::string report;
    m_document.Edit(
        [&](Chunk& root)
        {
            ApplyRigToPackage(root,
                *m_session.Rig(),
                m_session.Texture(),
                static_cast<uint32_t>(m_textureSize),
                report);
        });
    m_status = report + " Save it with File > Save (Ctrl+Z undoes it).";
    PackageTarget target = FindPackageTarget(*m_document.Root());

    if (!target.skins.empty())
    {
        m_document.Select(target.skins.front());
    }
}

}
