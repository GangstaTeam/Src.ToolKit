#pragma once

#include "link/LinkProtocol.hh"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

enum ELinkServerSize : size_t
{
    kMaxQueued = 8u << 20,
};

enum ELinkServerValue : uint32_t
{
    kOutBuffer = 1 << 20,
    kInBuffer = 1 << 16,
};

class LinkServer
{
public:
    struct Command
    {
        tk::link::Message type;
        uint32_t id = 0;
        tk::link::Field field = tk::link::Field::Visible;
        uint32_t value = 0;
        std::string text;
    };

    void Start();
    bool Connected() const { return m_connected; }
    uint32_t Connection() const { return m_connection; }
    void Publish(std::vector<uint8_t> bytes);
    std::vector<Command> TakeCommands();

private:
    static unsigned long __stdcall ThreadMain(void* self);
    void Run();
    void Serve(void* pipe);
    void Parse(std::vector<uint8_t>& input);

    std::mutex m_mutex;
    std::vector<uint8_t> m_outgoing;
    std::vector<Command> m_commands;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_stopping{false};
    std::atomic<uint32_t> m_connection{0};
};

template <typename T> void AppendMessage(std::vector<uint8_t>& out, tk::link::Message type, const T& record)
{
    tk::link::Header header = {sizeof(T), static_cast<uint32_t>(type)};
    const uint8_t* h = reinterpret_cast<const uint8_t*>(&header);
    const uint8_t* r = reinterpret_cast<const uint8_t*>(&record);
    out.insert(out.end(), h, h + sizeof(header));
    out.insert(out.end(), r, r + sizeof(T));
}
