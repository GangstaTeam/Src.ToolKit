#include "LinkServer.hh"

#include <windows.h>

#include <cstring>

using namespace tk::link;

namespace
{

bool WriteAll(HANDLE pipe, const std::vector<uint8_t>& bytes)
{
    size_t done = 0;

    while (done < bytes.size())
    {
        DWORD wrote = 0;

        if (!WriteFile(pipe, bytes.data() + done, static_cast<DWORD>(bytes.size() - done), &wrote, nullptr))
        {
            return false;
        }

        done += wrote;
    }

    return true;
}

}

void LinkServer::Start()
{
    HANDLE thread = CreateThread(nullptr, 0, &LinkServer::ThreadMain, this, 0, nullptr);

    if (thread)
    {
        CloseHandle(thread);
    }
}

unsigned long __stdcall LinkServer::ThreadMain(void* self)
{
    static_cast<LinkServer*>(self)->Run();
    return 0;
}

void LinkServer::Run()
{
    while (!m_stopping)
    {
        HANDLE pipe = CreateNamedPipeW(LINKPROTOCOL_PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,
            kOutBuffer,
            kInBuffer,
            0,
            nullptr);
        if (pipe == INVALID_HANDLE_VALUE)
        {
            Sleep(1000);
            continue;
        }

        if (ConnectNamedPipe(pipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED)
        {
            Serve(pipe);
        }

        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
}

void LinkServer::Serve(void* handle)
{
    HANDLE pipe = static_cast<HANDLE>(handle);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_outgoing.clear();
        m_commands.clear();
    }

    std::vector<uint8_t> hello;
    AppendMessage(hello, Message::Hello, HelloRecord{kVersion, GetCurrentProcessId()});

    if (!WriteAll(pipe, hello))
    {
        return;
    }

    ++m_connection;
    m_connected = true;
    std::vector<uint8_t> input, output;

    for (;;)
    {
        DWORD available = 0;

        if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr))
        {
            break;
        }

        if (available)
        {
            size_t old = input.size();
            input.resize(old + available);
            DWORD got = 0;

            if (!ReadFile(pipe, input.data() + old, available, &got, nullptr))
            {
                break;
            }

            input.resize(old + got);
            Parse(input);
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            output.swap(m_outgoing);
        }

        if (!output.empty())
        {
            if (!WriteAll(pipe, output))
            {
                break;
            }

            output.clear();
        }

        Sleep(8);
    }

    m_connected = false;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_outgoing.clear();
}

void LinkServer::Parse(std::vector<uint8_t>& input)
{
    size_t at = 0;
    std::vector<Command> parsed;

    while (input.size() - at >= sizeof(Header))
    {
        Header header;
        std::memcpy(&header, input.data() + at, sizeof(header));

        if (input.size() - at - sizeof(header) < header.size)
        {
            break;
        }

        const uint8_t* payload = input.data() + at + sizeof(header);
        Command command;
        command.type = static_cast<Message>(header.type);

        switch (command.type)
        {
        case Message::Select:
        case Message::Revert:
            if (header.size >= sizeof(IdRecord))
            {
                IdRecord record;
                std::memcpy(&record, payload, sizeof(record));
                command.id = record.id;
                parsed.push_back(command);
            }
            break;
        case Message::SetField:
            if (header.size >= sizeof(FieldRecord))
            {
                FieldRecord record;
                std::memcpy(&record, payload, sizeof(record));
                command.id = record.id;
                command.field = static_cast<Field>(record.field);
                command.value = record.value;
                parsed.push_back(command);
            }
            break;
        case Message::PickMode:
        case Message::Highlight:
            if (header.size >= sizeof(FlagRecord))
            {
                command.value = payload[0];
                parsed.push_back(command);
            }
            break;
        case Message::RunScript:
            if (header.size > 0 && header.size <= kMaxScript)
            {
                command.text.assign(reinterpret_cast<const char*>(payload), header.size);
                parsed.push_back(command);
            }
            break;
        default:
            break;
        }

        at += sizeof(header) + header.size;
    }

    input.erase(input.begin(), input.begin() + static_cast<std::ptrdiff_t>(at));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_commands.insert(m_commands.end(), parsed.begin(), parsed.end());
}

void LinkServer::Publish(std::vector<uint8_t> bytes)
{
    if (!m_connected)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_outgoing.size() + bytes.size() > kMaxQueued)
    {
        return;
    }

    m_outgoing.insert(m_outgoing.end(), bytes.begin(), bytes.end());
}

std::vector<LinkServer::Command> LinkServer::TakeCommands()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Command> out;
    out.swap(m_commands);
    return out;
}
