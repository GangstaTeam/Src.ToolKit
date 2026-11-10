#include "ui/CharacterImportPanel.hh"

#include "app/Theme.hh"
#include "rig/BoneMapping.hh"
#include "rig/LoadGroup.hh"
#include "rig/PackageWriter.hh"

#include <algorithm>
#include <cmath>

namespace tk
{

namespace
{

const ImVec4 kWarning(0.95f, 0.65f, 0.2f, 1.0f);

void Title(const char* text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, theme::kGold);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

void Paragraph(const char* text)
{
    ImGui::TextWrapped("%s", text);
    ImGui::Spacing();
}

void TargetInfo(Document& document)
{
    PackageTarget target = FindPackageTarget(*document.Root());
    ImGui::SeparatorText("Target package");

    if (!target.Valid())
    {
        ImGui::TextColored(kWarning, "No character package open.");
        ImGui::TextWrapped("Open the one to replace, e.g. packages/characters/pc/MCP_*.p3d.");
        return;
    }

    std::string file = document.HasPath() ? document.Path().filename().u8string() : "untitled.p3d";
    ImGui::TextWrapped("%s", file.c_str());
    ImGui::TextDisabled("skin %s, skeleton %s", target.skinName.c_str(), target.skeletonName.c_str());
}

}

void CharacterImportPanel::DrawInfo()
{
    switch (m_step)
    {
    case Step::Upload:
        DrawUploadInfo();
        break;
    case Step::Orient:
        DrawOrientInfo();
        break;
    case Step::Skeleton:
        DrawSkeletonInfo();
        break;
    case Step::Rigging:
        DrawRiggingInfo();
        break;
    case Step::Review:
        DrawReviewInfo();
        break;
    }

    if (m_status.empty())
    {
        return;
    }

    ImGui::Separator();
    ImGui::TextWrapped("%s", m_status.c_str());
}

void CharacterImportPanel::DrawUploadInfo()
{
    Title("Upload");
    Paragraph("Drop a .glb, .gltf or .fbx on the window, or open one.");
    ImGui::BeginDisabled(m_conversion.valid());

    if (ImGui::Button("Open model...", ImVec2(-1, 0)))
    {
        PickModel();
    }

    ImGui::EndDisabled();

    if (m_conversion.valid())
    {
        ImGui::TextColored(kWarning, "Importing FBX...");
    }

    TargetInfo(m_document);
}

void CharacterImportPanel::DrawOrientInfo()
{
    Title("Orient");
    Paragraph("Use the arrow buttons to rotate the model until it stands upright with its front facing you. "
              "A T-pose or A-pose works best.");
    const SourceModel& source = m_session.Source();
    ImGui::TextWrapped("%s", m_session.SourcePath().filename().u8string().c_str());
    ImGui::TextDisabled("%zu vertices, %zu triangles, %zu bones",
        source.VertexCount(),
        source.indices.size() / 3,
        source.bones.size());
    float height = m_session.Height();
    ImGui::SetNextItemWidth(120);

    if (ImGui::InputFloat("Height (m)", &height, 0.05f, 0.1f, "%.2f"))
    {
        m_session.SetHeight(height);
    }

    if (ImGui::Button("Reset rotation"))
    {
        m_session.ResetOrientation();
    }

    ImGui::SeparatorText("Texture");

    if (m_session.Texture().bgra.empty())
    {
        ImGui::TextColored(kWarning, "This model has no texture.");
    }
    else
    {
        ImGui::TextDisabled("%u x %u", m_session.Texture().width, m_session.Texture().height);
    }

    if (ImGui::Button("Texture from file...", ImVec2(-1, 0)))
    {
        PickTexture();
    }
}

void CharacterImportPanel::DrawSkeletonInfo()
{
    RigRequest& request = m_session.Request();

    if (m_session.Source().HasSkin())
    {
        RigSource before = request.source;

        if (ImGui::RadioButton("Place markers", request.source == RigSource::Markers))
        {
            request.source = RigSource::Markers;
        }

        ImGui::SameLine();

        if (ImGui::RadioButton("Use its skeleton", request.source == RigSource::ModelSkeleton))
        {
            request.source = RigSource::ModelSkeleton;
        }

        if (request.source != before)
        {
            m_session.DiscardRig();
        }

        ImGui::Separator();
    }

    if (request.source == RigSource::Markers)
    {
        Title("Place markers");
        Paragraph("Drag each marker from the left onto the model: chin, wrists, elbows, knees and groin. "
                  "Then press Next to start the auto-rigging.");
        Paragraph("With symmetry on, placing one side places the other.");
        Paragraph(
            "Shoulders are optional: without them they are worked out from the arm. Place them to fix the armpits.");
        Paragraph("Markers snap to the middle of the limb under the cursor. Hold Ctrl to place them freely.");
    }
    else
    {
        Title("Map bones");
        Paragraph("Each bone of the model drives the game joint chosen here. '-' follows its parent bone.");
        DrawBoneMapping();
    }

    ImGui::Checkbox("Straighten arms to T-pose", &request.straightenArms);
}

void CharacterImportPanel::DrawBoneMapping()
{
    std::vector<std::string>& mapping = m_session.Request().mapping;
    const std::vector<SourceBone>& bones = m_session.Source().bones;
    ImGui::BeginChild("mapping", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);

    for (size_t i = 0; i < bones.size() && i < mapping.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        ImGui::SetNextItemWidth(110);

        if (ImGui::BeginCombo("##joint", mapping[i].empty() ? "-" : mapping[i].c_str()))
        {
            if (ImGui::Selectable("-", mapping[i].empty()))
            {
                mapping[i].clear();
            }

            for (const std::string& joint : MappableJoints())
            {
                if (ImGui::Selectable(joint.c_str(), mapping[i] == joint))
                {
                    mapping[i] = joint;
                }
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::TextUnformatted(bones[i].name.c_str());
        ImGui::PopID();
    }

    ImGui::EndChild();
}

void CharacterImportPanel::DrawRiggingInfo()
{
    Title("Auto-rigging");
    Paragraph("Fitting the game skeleton to the model and computing the skin weights...");
    float progress = std::min(m_riggingTime / 1.8f, 0.97f);
    ImGui::ProgressBar(progress, ImVec2(-1, 0), "");
}

void CharacterImportPanel::DrawReviewInfo()
{
    Title("Review");
    Paragraph("The model is on the game skeleton. Check how it bends, then apply it to the open package.");
    ImGui::Checkbox("Test pose", &m_testPose);

    if (ImGui::Checkbox("Bone colours", &m_boneColours))
    {
        m_sceneRevision = 0;
    }

    ImGui::Checkbox("Joint names", &m_showNames);
    DrawPaintControls();
    ImGui::SeparatorText("Texture size");

    for (int size : {256, 512, 1024})
    {
        if (size != 256)
        {
            ImGui::SameLine();
        }

        ImGui::RadioButton(std::to_string(size).c_str(), &m_textureSize, size);
    }

    TargetInfo(m_document);
    ImGui::SeparatorText("New character");
    bool inGame = m_document.HasPath() && GameFoldersFromPackage(m_document.Path());
    ImGui::TextWrapped(
        "Saves a new package with its own name and adds it to pc_loadgroup.p3d. The open file is not changed.");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##newName", m_newName, sizeof(m_newName));
    ImGui::BeginDisabled(!inGame || !FindPackageTarget(*m_document.Root()).Valid());

    if (ImGui::Button("Save as new character", ImVec2(-1, 0)))
    {
        SaveAsNewCharacter();
    }

    ImGui::EndDisabled();

    if (!inGame)
    {
        ImGui::TextDisabled("Open a package from the game's packages/characters/pc folder.");
    }
}

}
