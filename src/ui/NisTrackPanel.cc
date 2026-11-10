#include "ui/NisWorkspace.hh"

#include "ui/Widgets.hh"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tk
{

namespace
{

const ImVec4 kErrorColour(0.95f, 0.35f, 0.30f, 1.0f);
const ImVec4 kWarningColour(0.95f, 0.70f, 0.30f, 1.0f);
const ImVec4 kInfoColour(0.60f, 0.62f, 0.66f, 1.0f);

bool IsFrameField(const nis::FieldSpec& field)
{
    return std::strstr(field.key, "Frame") != nullptr;
}

std::string WithoutLeadingNewline(std::string text)
{
    if (text.rfind("\r\n", 0) == 0)
    {
        text.erase(0, 2);
    }

    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    return text;
}

}

void NisWorkspace::DrawTrack()
{
    if (!ImGui::Begin(TrackWindow()))
    {
        return ImGui::End();
    }

    if (!m_open)
    {
        ImGui::TextDisabled("No NIS scene open.");
        return ImGui::End();
    }

    fsc::Node* track = SelectedTrack();

    if (!track)
    {
        ImGui::TextDisabled("Select a track on the timeline.");
        return ImGui::End();
    }

    const nis::TrackSpec* spec = nis::FindTrack(track->key);

    if (spec)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, NisCategoryColour(spec->category));
        ImGui::TextUnformatted(spec->label);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", spec->type);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextDisabled("%s", spec->help);
        ImGui::PopTextWrapPos();
    }
    else
    {
        ImGui::Text("%s", track->key.c_str());
        ImGui::TextDisabled("Unknown track type; its fields are edited as raw text.");
    }

    ImGui::Separator();

    if (ImGui::BeginTable("fields", 2, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        if (spec)
        {
            for (const nis::FieldSpec& field : spec->fields)
            {
                DrawField(*track, *spec, field);
            }
        }

        for (size_t i = 0; i < track->children.size(); ++i)
        {
            if (!spec || !nis::FindField(*spec, track->children[i].key))
            {
                DrawRawField(*track, i);
            }
        }

        ImGui::EndTable();
    }

    if (track->key == "NISFade")
    {
        float colour[3] = {track->Int("Red") / 255.0f, track->Int("Green") / 255.0f, track->Int("Blue") / 255.0f};

        if (ImGui::ColorEdit3("Colour", colour, ImGuiColorEditFlags_Uint8))
        {
            BeginChange();
            track->SetInt("Red", std::lround(colour[0] * 255.0f));
            track->SetInt("Green", std::lround(colour[1] * 255.0f));
            track->SetInt("Blue", std::lround(colour[2] * 255.0f));
        }
    }

    if (track->key == "NISScriptFunctionCallTrack")
    {
        nis::Subtitle subtitle;

        if (nis::ParseSubtitle(track->String("ScriptFunctionCall"), subtitle))
        {
            ImGui::TextDisabled("Subtitle line %s, on screen for %d ms (%d frames)",
                subtitle.line.c_str(),
                subtitle.milliseconds,
                static_cast<int>(std::lround(subtitle.milliseconds * nis::kFramesPerSecond / 1000.0)));
        }
    }

    bool header = false;

    for (const nis::Issue& issue : m_issues)
    {
        if (issue.node != m_node || issue.track != m_track)
        {
            continue;
        }

        if (!header)
        {
            ImGui::Spacing();
            ImGui::SeparatorText("Problems");
            header = true;
        }

        const ImVec4& colour = issue.severity == nis::Severity::Error ? kErrorColour
            : issue.severity == nis::Severity::Warning                ? kWarningColour
                                                                      : kInfoColour;
        ImGui::PushStyleColor(ImGuiCol_Text, colour);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::Bullet();
        ImGui::TextUnformatted(issue.message.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("FSC text"))
    {
        DrawRawEditor(*track);
    }

    ImGui::End();
}

void NisWorkspace::DrawField(fsc::Node& track, const nis::TrackSpec& spec, const nis::FieldSpec& field)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(field.key);

    if (*field.help && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", field.help);
    }

    ImGui::TableNextColumn();
    ImGui::PushID(field.key);
    fsc::Node* value = track.Find(field.key);

    if (!value)
    {
        if (ImGui::SmallButton("Add"))
        {
            BeginChange(true);
            nis::InsertField(track, spec, field);
        }

        ImGui::SameLine();
        ImGui::TextDisabled("missing");
        ImGui::PopID();
        return;
    }

    switch (field.kind)
    {
    case nis::FieldKind::String:
    {
        std::string text = track.String(field.key);
        bool suggestions = field.ref != nis::Ref::None;
        float arrow = suggestions ? ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x : 0.0f;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - arrow);

        if (InputString("##value", text))
        {
            BeginChange();
            track.SetString(field.key, text);
        }

        if (suggestions)
        {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

            if (DrawSuggestions("suggest", field.ref, text))
            {
                BeginChange(true);
                track.SetString(field.key, text);
            }
        }
        break;
    }
    case nis::FieldKind::Int:
    {
        int number = static_cast<int>(track.Int(field.key));
        char format[64] = "%d";

        if (field.milliseconds)
        {
            std::snprintf(format,
                sizeof(format),
                "%%d ms  (%d frames)",
                static_cast<int>(std::lround(number * nis::kFramesPerSecond / 1000.0)));
        }
        else if (IsFrameField(field))
        {
            std::snprintf(format, sizeof(format), "%%d  (%.2f s)", static_cast<float>(number) / nis::kFramesPerSecond);
        }

        ImGui::SetNextItemWidth(-FLT_MIN);

        if (ImGui::DragInt("##value", &number, field.milliseconds ? 5.0f : 0.25f, 0, 0, format))
        {
            BeginChange();
            track.SetInt(field.key, number);
        }
        break;
    }
    case nis::FieldKind::Float:
    {
        double number = track.Number(field.key);
        ImGui::SetNextItemWidth(-FLT_MIN);

        if (ImGui::DragScalar("##value", ImGuiDataType_Double, &number, 0.1f, nullptr, nullptr, "%.6g"))
        {
            BeginChange();
            track.SetNumber(field.key, number);
        }
        break;
    }
    case nis::FieldKind::Bool:
    {
        bool on = track.Int(field.key) != 0;

        if (ImGui::Checkbox("##value", &on))
        {
            BeginChange(true);
            track.SetInt(field.key, on ? 1 : 0);
        }
        break;
    }
    case nis::FieldKind::Vector:
    {
        if (!value->block)
        {
            ImGui::TextDisabled("expected a block");
            break;
        }

        double xyz[3] = {value->Number("x"), value->Number("y"), value->Number("z")};
        ImGui::SetNextItemWidth(-FLT_MIN);

        if (ImGui::DragScalarN("##value", ImGuiDataType_Double, xyz, 3, 0.05f, nullptr, nullptr, "%.6g"))
        {
            BeginChange();
            value->SetNumber("x", xyz[0]);
            value->SetNumber("y", xyz[1]);
            value->SetNumber("z", xyz[2]);
        }
        break;
    }
    case nis::FieldKind::Conditions:
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%lld conditions", value->Int("conditionCount"));
        break;
    }

    ImGui::PopID();
}

void NisWorkspace::DrawRawField(fsc::Node& track, size_t index)
{
    fsc::Node& child = track.children[index];
    ImGui::PushID(static_cast<int>(index) + 1000);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(kInfoColour, "%s", child.key.c_str());
    ImGui::TableNextColumn();

    if (child.block)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("{ %zu entries }", child.children.size());

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", WithoutLeadingNewline(fsc::Write(child)).c_str());
        }
    }
    else
    {
        std::string raw = child.value;
        ImGui::SetNextItemWidth(-FLT_MIN);

        if (InputString("##raw", raw) && raw.find_first_of(";{}\r\n") == std::string::npos)
        {
            BeginChange();
            child.value = raw;
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Raw value: strings keep their quotes");
        }
    }

    ImGui::PopID();
}

bool NisWorkspace::DrawSuggestions(const char* id, nis::Ref ref, std::string& value)
{
    std::vector<std::string> items = Suggestions(ref);
    ImGui::BeginDisabled(items.empty());

    if (ImGui::ArrowButton(id, ImGuiDir_Down))
    {
        ImGui::OpenPopup(id);
    }

    ImGui::EndDisabled();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip(items.empty() ? "Nothing to suggest for this scene" : "Pick from this scene");
    }

    bool picked = false;

    if (ImGui::BeginPopup(id))
    {
        for (size_t i = 0; i < items.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));

            if (ImGui::Selectable(items[i].empty() ? "(none)" : items[i].c_str(), items[i] == value))
            {
                value = items[i];
                picked = true;
            }

            ImGui::PopID();
        }

        ImGui::EndPopup();
    }

    return picked;
}

void NisWorkspace::DrawRawEditor(fsc::Node& track)
{
    if (m_rawRevision != m_revision || m_rawTrack != m_track)
    {
        m_raw = WithoutLeadingNewline(fsc::Write(track));
        m_rawRevision = m_revision;
        m_rawTrack = m_track;
        m_rawError.clear();
    }

    float height = ImGui::GetTextLineHeight() *
        std::clamp(static_cast<float>(std::count(m_raw.begin(), m_raw.end(), '\n') + 2), 4.0f, 24.0f);
    InputMultiline("##raw", m_raw, ImVec2(-FLT_MIN, height));

    if (ImGui::Button("Apply text"))
    {
        std::vector<fsc::Node> parsed;
        std::string error;

        if (!fsc::Parse(m_raw, parsed, error))
        {
            m_rawError = error;
        }
        else if (parsed.size() != 1 || !parsed[0].block)
        {
            m_rawError = "the text must be exactly one track block";
        }
        else
        {
            BeginChange(true);
            track = std::move(parsed[0]);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Revert"))
    {
        m_rawRevision = UINT64_MAX;
    }

    if (!m_rawError.empty())
    {
        ImGui::TextColored(kErrorColour, "%s", m_rawError.c_str());
    }
}

}
