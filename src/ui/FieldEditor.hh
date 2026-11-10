#pragma once

#include "p3d/Field.hh"
#include "p3d/Schema.hh"

#include <set>
#include <string>
#include <vector>

namespace tk
{

class FieldEditor
{
public:
    explicit FieldEditor(const Schema& schema) : m_schema(schema) {}

    bool Draw(std::vector<Field>& fields);

private:
    void DrawFields(std::vector<Field>& fields);
    void DrawField(Field& field, bool readOnly);
    void DrawArray(Field& field);
    void DrawArrayItems(Field& field);
    void DrawStruct(Field& field);
    void DrawValue(Field& field, bool readOnly);
    bool DrawPrimitive(Field& field);
    bool DrawInlineFloats(Field& field);
    void Track(bool changed);

    const Schema& m_schema;
    bool m_commit = false;
};

}
