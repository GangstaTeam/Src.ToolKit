#include "ui/FieldEditor.hh"

#include "p3d/PayloadCodec.hh"
#include "ui/Widgets.hh"

#include <imgui.h>

namespace tk
{

namespace
{

std::set<std::string> CountFields(const std::vector<Field>& fields)
{
    std::set<std::string> counts;

    for (const Field& field : fields)
    {
        if (field.isArray)
        {
            counts.insert(field.countField);
        }
    }

    return counts;
}

bool AllFloats(const Field& field, size_t maxCount)
{
    if (field.items.empty() || field.items.size() > maxCount)
    {
        return false;
    }

    for (const Field& item : field.items)
    {
        if (item.primitive != Primitive::F32)
        {
            return false;
        }
    }

    return true;
}

}

bool FieldEditor::Draw(std::vector<Field>& fields)
{
    m_commit = false;
    ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable;

    if (!ImGui::BeginTable("fields", 2, flags))
    {
        return false;
    }

    ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthFixed, 220.0f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
    DrawFields(fields);
    ImGui::EndTable();

    if (m_commit)
    {
        SyncArrayCounts(fields);
    }

    return m_commit;
}

void FieldEditor::Track(bool changed)
{
    if (changed && !ImGui::IsItemActive())
    {
        m_commit = true;
    }

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        m_commit = true;
    }
}

void FieldEditor::DrawFields(std::vector<Field>& fields)
{
    std::set<std::string> counts = CountFields(fields);

    for (size_t i = 0; i < fields.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        DrawField(fields[i], counts.count(fields[i].name) > 0);
        ImGui::PopID();
    }
}

void FieldEditor::DrawField(Field& field, bool readOnly)
{
    if (field.isArray)
    {
        return DrawArray(field);
    }

    if (field.primitive == Primitive::None && !AllFloats(field, 4))
    {
        return DrawStruct(field);
    }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TreeNodeEx(field.name.c_str(),
        ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", field.type.c_str());
    }

    ImGui::TableNextColumn();
    DrawValue(field, readOnly);
}

void FieldEditor::DrawValue(Field& field, bool readOnly)
{
    ImGui::SetNextItemWidth(-1);
    ImGui::BeginDisabled(readOnly);

    if (field.primitive == Primitive::None)
    {
        Track(DrawInlineFloats(field));
    }
    else
    {
        Track(DrawPrimitive(field));
    }

    ImGui::EndDisabled();
}

bool FieldEditor::DrawPrimitive(Field& field)
{
    switch (field.primitive)
    {
    case Primitive::U8:
        return ImGui::InputScalar("##v", ImGuiDataType_U8, &field.integer);
    case Primitive::U16:
        return ImGui::InputScalar("##v", ImGuiDataType_U16, &field.integer);
    case Primitive::I16:
        return ImGui::InputScalar("##v", ImGuiDataType_S16, &field.integer);
    case Primitive::U32:
        return ImGui::InputScalar("##v", ImGuiDataType_U32, &field.integer);
    case Primitive::I32:
        return ImGui::InputScalar("##v", ImGuiDataType_S32, &field.integer);
    case Primitive::F32:
        return ImGui::DragFloat("##v", &field.real, 0.01f, 0.0f, 0.0f, "%.6g");
    case Primitive::String:
    case Primitive::LongString:
        return InputString("##v", field.text);
    case Primitive::FourCC:
        return InputFourCC("##v", field.integer);
    case Primitive::Colour:
        return InputArgb("##v", field.integer);
    case Primitive::Raw:
        ImGui::TextDisabled("%zu bytes (edit in Hex)", field.raw.size());
        return false;
    case Primitive::None:
        return false;
    }

    return false;
}

bool FieldEditor::DrawInlineFloats(Field& field)
{
    float values[4] = {};

    for (size_t i = 0; i < field.items.size(); ++i)
    {
        values[i] = field.items[i].real;
    }

    int count = static_cast<int>(field.items.size());
    bool changed = ImGui::DragScalarN("##v", ImGuiDataType_Float, values, count, 0.01f, nullptr, nullptr, "%.5g");

    for (size_t i = 0; i < field.items.size(); ++i)
    {
        field.items[i].real = values[i];
    }

    return changed;
}

void FieldEditor::DrawStruct(Field& field)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    bool open = ImGui::TreeNodeEx(field.name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%s", field.type.c_str());

    if (!open)
    {
        return;
    }

    DrawFields(field.items);
    ImGui::TreePop();
}

void FieldEditor::DrawArray(Field& field)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    bool open = ImGui::TreeNodeEx(field.name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%s[%zu]", field.type.c_str(), field.items.size());
    ImGui::SameLine();

    if (ImGui::SmallButton("+"))
    {
        Field item = MakeDefaultField(m_schema, {field.type, "[" + std::to_string(field.items.size()) + "]"});
        field.items.push_back(std::move(item));
        m_commit = true;
    }

    ImGui::SameLine();

    if (ImGui::SmallButton("-") && !field.items.empty())
    {
        field.items.pop_back();
        m_commit = true;
    }

    if (!open)
    {
        return;
    }

    DrawArrayItems(field);
    ImGui::TreePop();
}

void FieldEditor::DrawArrayItems(Field& field)
{
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(field.items.size()));

    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
        {
            ImGui::PushID(i);
            DrawField(field.items[static_cast<size_t>(i)], false);
            ImGui::PopID();
        }
    }
}

}
