#include "p3d/SchemaParser.hh"

#include "core/FileIo.hh"
#include "p3d/SchemaTokenizer.hh"

#include <algorithm>
#include <cstdlib>
#include <vector>

namespace tk
{

namespace
{

class Parser
{
public:
    Parser(std::vector<std::string> tokens, Schema& schema) : m_tokens(std::move(tokens)), m_schema(schema) {}

    bool Run(std::string& error)
    {
        while (m_index < m_tokens.size())
        {
            const std::string kind = Next();

            if (kind == "chunk")
            {
                if (!ParseChunk())
                {
                    break;
                }
            }
            else if (kind == "struct")
            {
                if (!ParseStruct())
                {
                    break;
                }
            }
            else
            {
                m_error = "unexpected '" + kind + "'";
                break;
            }
        }

        error = m_error;
        return m_error.empty();
    }

private:
    const std::string& Peek() const
    {
        static const std::string kEnd;
        return m_index < m_tokens.size() ? m_tokens[m_index] : kEnd;
    }

    std::string Next() { return m_index < m_tokens.size() ? m_tokens[m_index++] : std::string(); }

    bool Expect(const std::string& token)
    {
        if (Next() == token)
        {
            return true;
        }

        m_error = "expected '" + token + "' near token " + std::to_string(m_index);
        return false;
    }

    bool ParseChunk()
    {
        ChunkDef chunk;
        chunk.name = Next();

        if (!Expect(":"))
        {
            return false;
        }

        bool hasId = false;

        while (Peek() != "{" && m_index < m_tokens.size())
        {
            const std::string attribute = Next();

            if (attribute == ",")
            {
                continue;
            }

            if (!Expect("("))
            {
                return false;
            }

            uint32_t value = static_cast<uint32_t>(std::strtoul(Next().c_str(), nullptr, 0));

            if (!Expect(")"))
            {
                return false;
            }

            if (attribute == "chunk_id")
            {
                chunk.id = value;
                hasId = true;
            }
            else if (attribute == "subchunk")
            {
                chunk.subchunks.push_back(value);
            }
        }

        if (!hasId)
        {
            m_error = "chunk " + chunk.name + " has no chunk_id";
            return false;
        }

        if (!ParseFields(chunk.fields))
        {
            return false;
        }

        m_schema.AddChunk(std::move(chunk));
        return true;
    }

    bool ParseStruct()
    {
        StructDef def;
        def.name = Next();

        if (!ParseFields(def.fields))
        {
            return false;
        }

        m_schema.AddStruct(std::move(def));
        return true;
    }

    bool ParseFields(std::vector<FieldDef>& fields)
    {
        if (!Expect("{"))
        {
            return false;
        }

        while (Peek() != "}" && m_index < m_tokens.size())
        {
            FieldDef field;
            field.type = Next();

            if (field.type == "array")
            {
                field.isArray = true;

                if (!Expect("("))
                {
                    return false;
                }

                field.type = Next();

                if (!Expect(","))
                {
                    return false;
                }

                field.countField = Next();

                if (!Expect(")"))
                {
                    return false;
                }
            }

            field.name = Next();

            if (!Expect(";"))
            {
                return false;
            }

            fields.push_back(std::move(field));
        }

        return Expect("}");
    }

    std::vector<std::string> m_tokens;
    Schema& m_schema;
    size_t m_index = 0;
    std::string m_error;
};

}

bool ParseSchemaText(const std::string& text, Schema& schema, std::string& error)
{
    return Parser(TokenizeSchema(text), schema).Run(error);
}

bool LoadSchemaDirectory(const std::filesystem::path& directory, Schema& schema, std::string& error)
{
    std::vector<std::filesystem::path> files;

    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.path().extension() == ".sc")
        {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());

    for (const auto& file : files)
    {
        auto text = ReadText(file);

        if (!text)
        {
            error = "cannot read " + file.string();
            return false;
        }

        if (!ParseSchemaText(*text, schema, error))
        {
            error = file.filename().string() + ": " + error;
            return false;
        }
    }

    return true;
}

}
