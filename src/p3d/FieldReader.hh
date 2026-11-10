#pragma once

#include "core/ByteReader.hh"
#include "p3d/Field.hh"
#include "p3d/Schema.hh"

#include <string>
#include <vector>

namespace tk
{

class FieldReader
{
public:
    FieldReader(const Schema& schema, const Bytes& payload);

    bool ReadFields(const std::vector<FieldDef>& defs, std::vector<Field>& out, std::string& error);
    Bytes Rest();

private:
    bool ReadField(const FieldDef& def, const std::vector<Field>& scope, Field& out, std::string& error);
    bool ReadArray(const FieldDef& def, const std::vector<Field>& scope, Field& out, std::string& error);
    bool ReadValue(const std::string& type, Field& out, std::string& error);
    bool ReadPrimitive(Field& out);
    size_t MinSize(const std::string& type) const;

    const Schema& m_schema;
    ByteReader m_reader;
};

}
