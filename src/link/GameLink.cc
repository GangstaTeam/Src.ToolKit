#include "link/GameLink.hh"

#include <windows.h>

#include <chrono>
#include <cstring>

namespace tk
{

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

GameLink::GameLink()
    : m_thread(
          [this]
          {
              Run();
          })
{
}

GameLink::~GameLink()
{
    m_stop = true;
    m_thread.join();
}

void GameLink::Run()
{
    while (!m_stop)
    {
        HANDLE pipe =
            CreateFileW(LINKPROTOCOL_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe == INVALID_HANDLE_VALUE)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        Serve(pipe);
        CloseHandle(pipe);
        m_connected = false;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_nodes.clear();
        m_picks.clear();
        ++m_frame;
    }
}

void GameLink::Serve(void* handle)
{
    HANDLE pipe = static_cast<HANDLE>(handle);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_outgoing.clear();
    }

    std::vector<uint8_t> input, output;

    while (!m_stop)
    {
        DWORD available = 0;

        if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr))
        {
            return;
        }

        if (available)
        {
            size_t old = input.size();
            input.resize(old + available);
            DWORD got = 0;

            if (!ReadFile(pipe, input.data() + old, available, &got, nullptr))
            {
                return;
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
                return;
            }

            output.clear();
        }

        if (!available)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void GameLink::Parse(std::vector<uint8_t>& input)
{
    size_t at = 0;

    while (input.size() - at >= sizeof(link::Header))
    {
        link::Header header;
        std::memcpy(&header, input.data() + at, sizeof(header));

        if (input.size() - at - sizeof(header) < header.size)
        {
            break;
        }

        const uint8_t* payload = input.data() + at + sizeof(header);

        switch (static_cast<link::Message>(header.type))
        {
        case link::Message::Hello:
            if (header.size >= sizeof(link::HelloRecord))
            {
                link::HelloRecord hello;
                std::memcpy(&hello, payload, sizeof(hello));

                if (hello.version == link::kVersion)
                {
                    m_pid = hello.pid;
                    ++m_connection;
                    m_connected = true;
                }
            }
            break;
        case link::Message::Snapshot:
            if (header.size >= sizeof(link::SnapshotHeader))
            {
                link::SnapshotHeader snapshot;
                std::memcpy(&snapshot, payload, sizeof(snapshot));
                size_t count = (header.size - sizeof(snapshot)) / sizeof(link::NodeRecord);

                if (count > snapshot.count)
                {
                    count = snapshot.count;
                }

                std::lock_guard<std::mutex> lock(m_mutex);
                m_nodes.resize(count);

                if (count)
                {
                    std::memcpy(m_nodes.data(), payload + sizeof(snapshot), count * sizeof(link::NodeRecord));
                }

                ++m_frame;
            }
            break;
        case link::Message::Names:
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            for (size_t i = 0; i + 5 <= header.size;)
            {
                uint32_t key;
                std::memcpy(&key, payload + i, 4);
                uint8_t length = payload[i + 4];

                if (i + 5 + length > header.size)
                {
                    break;
                }

                m_names[key].assign(reinterpret_cast<const char*>(payload + i + 5), length);
                i += 5 + length;
            }
            break;
        }
        case link::Message::Player:
            if (header.size >= sizeof(link::PlayerRecord))
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                std::memcpy(&m_player, payload, sizeof(m_player));
                m_hasPlayer = true;
            }
            break;
        case link::Message::Pick:
            if (header.size >= sizeof(link::PickRecord))
            {
                link::PickRecord pick;
                std::memcpy(&pick, payload, sizeof(pick));
                std::lock_guard<std::mutex> lock(m_mutex);
                m_picks.push_back(pick);
            }
            break;
        default:
            break;
        }

        at += sizeof(header) + header.size;
    }

    input.erase(input.begin(), input.begin() + static_cast<std::ptrdiff_t>(at));
}

bool GameLink::LatestSnapshot(uint32_t& frame, std::vector<link::NodeRecord>& nodes)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (frame == m_frame)
    {
        return false;
    }

    frame = m_frame;
    nodes = m_nodes;
    return true;
}

std::string GameLink::NameOf(uint32_t key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto found = m_names.find(key);
    return found == m_names.end() ? std::string() : found->second;
}

std::vector<link::PickRecord> GameLink::TakePicks()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<link::PickRecord> out;
    out.swap(m_picks);
    return out;
}

template <typename T> void GameLink::Queue(link::Message type, const T& record)
{
    if (!m_connected)
    {
        return;
    }

    link::Header header = {sizeof(T), static_cast<uint32_t>(type)};
    std::lock_guard<std::mutex> lock(m_mutex);
    const uint8_t* h = reinterpret_cast<const uint8_t*>(&header);
    const uint8_t* r = reinterpret_cast<const uint8_t*>(&record);
    m_outgoing.insert(m_outgoing.end(), h, h + sizeof(header));
    m_outgoing.insert(m_outgoing.end(), r, r + sizeof(T));
}

void GameLink::Select(uint32_t id)
{
    Queue(link::Message::Select, link::IdRecord{id});
}

void GameLink::SetField(uint32_t id, link::Field field, uint32_t value)
{
    link::FieldRecord record = {};
    record.id = id;
    record.field = static_cast<uint8_t>(field);
    record.value = value;
    Queue(link::Message::SetField, record);
}

void GameLink::SetField(uint32_t id, link::Field field, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    SetField(id, field, bits);
}

void GameLink::Revert(uint32_t id)
{
    Queue(link::Message::Revert, link::IdRecord{id});
}

void GameLink::SetPickMode(bool on)
{
    Queue(link::Message::PickMode, link::FlagRecord{static_cast<uint8_t>(on)});
}

void GameLink::SetHighlight(bool on)
{
    Queue(link::Message::Highlight, link::FlagRecord{static_cast<uint8_t>(on)});
}

void GameLink::RunScript(const std::string& script)
{
    if (!m_connected || script.empty() || script.size() > link::kMaxScript)
    {
        return;
    }

    link::Header header = {static_cast<uint32_t>(script.size()), static_cast<uint32_t>(link::Message::RunScript)};
    std::lock_guard<std::mutex> lock(m_mutex);
    const uint8_t* h = reinterpret_cast<const uint8_t*>(&header);
    m_outgoing.insert(m_outgoing.end(), h, h + sizeof(header));
    m_outgoing.insert(m_outgoing.end(), script.begin(), script.end());
}

bool GameLink::Player(link::PlayerRecord& player) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    player = m_player;
    return m_connected && m_hasPlayer;
}

}
