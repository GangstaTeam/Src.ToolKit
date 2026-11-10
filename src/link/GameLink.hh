#pragma once

#include "link/LinkProtocol.hh"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace tk
{

class GameLink
{
public:
    GameLink();
    ~GameLink();
    GameLink(const GameLink&) = delete;
    GameLink& operator=(const GameLink&) = delete;

    bool Connected() const { return m_connected; }
    uint32_t Pid() const { return m_pid; }
    uint32_t Connection() const { return m_connection; }

    bool LatestSnapshot(uint32_t& frame, std::vector<link::NodeRecord>& nodes);
    std::string NameOf(uint32_t key) const;
    std::vector<link::PickRecord> TakePicks();

    void Select(uint32_t id);
    void SetField(uint32_t id, link::Field field, uint32_t value);
    void SetField(uint32_t id, link::Field field, float value);
    void Revert(uint32_t id);
    void SetPickMode(bool on);
    void SetHighlight(bool on);
    void RunScript(const std::string& script);
    bool Player(link::PlayerRecord& player) const;

private:
    void Run();
    void Serve(void* pipe);
    void Parse(std::vector<uint8_t>& input);
    template <typename T> void Queue(link::Message type, const T& record);

    std::thread m_thread;
    std::atomic<bool> m_stop{false};
    std::atomic<bool> m_connected{false};
    std::atomic<uint32_t> m_pid{0};
    std::atomic<uint32_t> m_connection{0};

    mutable std::mutex m_mutex;
    std::vector<uint8_t> m_outgoing;
    std::vector<link::NodeRecord> m_nodes;
    uint32_t m_frame = 0;
    std::unordered_map<uint32_t, std::string> m_names;
    std::vector<link::PickRecord> m_picks;
    link::PlayerRecord m_player = {};
    bool m_hasPlayer = false;
};

}
