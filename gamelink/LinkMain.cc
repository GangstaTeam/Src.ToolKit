#include "LinkServer.hh"
#include "LiveFrontend.hh"

#include <SDK/.SDK.hh>

#include <MinHook.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace tk::link;
namespace frontend = pure3d::frontend;

namespace
{

enum ELinkMainSize : size_t
{
    kMaxNames = 400000,
};

using AppDrawFn = void(__thiscall*)(frontend::App*, bool);
using KeySetFn = core::Key(__thiscall*)(core::Key*, const char*);
using ScheduleFn = int(__cdecl*)(int, const char*, int, char);

AppDrawFn g_appDraw = nullptr;
KeySetFn g_keySet = nullptr;

LinkServer& g_server = *new LinkServer;
LiveFrontend g_frontend;

std::mutex g_namesMutex;
std::unordered_map<core::Key, std::string> g_names;

void Log(const char* format, ...)
{
    char path[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);

    if (!length || length >= MAX_PATH)
    {
        return;
    }

    std::string file(path, length);
    file = file.substr(0, file.find_last_of("\\/") + 1) + "ToolKitLink.log";
    FILE* out = std::fopen(file.c_str(), "a");

    if (!out)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    std::vfprintf(out, format, args);
    va_end(args);
    std::fputc('\n', out);
    std::fclose(out);
}

double Seconds()
{
    static LARGE_INTEGER frequency = []
    {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return f;
    }();
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return static_cast<double>(now.QuadPart) / static_cast<double>(frequency.QuadPart);
}

BOOL CALLBACK FindGameWindow(HWND window, LPARAM result)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);

    if (pid != GetCurrentProcessId() || !IsWindowVisible(window) || GetWindow(window, GW_OWNER))
    {
        return TRUE;
    }

    RECT rect;
    GetClientRect(window, &rect);
    HWND& best = *reinterpret_cast<HWND*>(result);
    RECT bestRect = {};

    if (best)
    {
        GetClientRect(best, &bestRect);
    }

    if (!best || rect.right * rect.bottom > bestRect.right * bestRect.bottom)
    {
        best = window;
    }

    return TRUE;
}

bool ReadPlayer(PlayerRecord& record)
{
    __try
    {
        CharacterObject* player = CharacterObject::GetMainCharacter();

        if (!player)
        {
            return false;
        }

        const math::Matrix& transform = player->m_Transform;
        record =
            {transform.m3[0], transform.m3[1], transform.m3[2], transform.m2[0], transform.m2[1], transform.m2[2], {}};
        const char* name = nullptr;

        if (const uint8_t* characterTemplate =
                *reinterpret_cast<const uint8_t* const*>(reinterpret_cast<const uint8_t*>(player) + 0xB4))
        {
            name = *reinterpret_cast<const char* const*>(characterTemplate + 0xA4);
        }

        for (size_t i = 0; name && i + 1 < sizeof(record.nisModel); ++i)
        {
            char c = name[i];

            if (!c)
            {
                break;
            }

            if (c < 0x20 || c > 0x7E)
            {
                record.nisModel[0] = '\0';
                break;
            }

            record.nisModel[i] = c;
        }

        return true;
    }

    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

HWND GameWindow()
{
    static HWND window = nullptr;

    if (window && IsWindow(window))
    {
        return window;
    }

    window = nullptr;
    EnumWindows(FindGameWindow, reinterpret_cast<LPARAM>(&window));
    return window;
}

class Link
{
public:
    void OnFrame()
    {
        bool connected = g_server.Connected();

        if (!connected && !m_wasConnected)
        {
            return;
        }

        g_frontend.Walk();

        if (!connected)
        {
            g_frontend.ClearHighlight();
            g_frontend.Select(0);
            m_pickMode = false;
            m_wasConnected = false;
            return;
        }

        if (g_server.Connection() != m_connection)
        {
            m_connection = g_server.Connection();
            m_sentKeys.clear();
            m_lastSnapshot = 0.0;
        }

        m_wasConnected = true;

        for (const LinkServer::Command& command : g_server.TakeCommands())
        {
            Apply(command);
        }

        double now = Seconds();
        g_frontend.UpdateHighlight(now);

        if (m_pickMode)
        {
            CheckPick();
        }

        if (now - m_lastSnapshot >= 0.1)
        {
            m_lastSnapshot = now;
            SendSnapshot();
        }
    }

private:
    void Apply(const LinkServer::Command& command)
    {
        switch (command.type)
        {
        case Message::Select:
            g_frontend.Select(command.id);
            break;
        case Message::SetField:
            g_frontend.SetField(command.id, command.field, command.value);
            break;
        case Message::Revert:
            g_frontend.Revert(command.id);
            break;
        case Message::PickMode:
            m_pickMode = command.value != 0;
            m_mouseDown = true;
            break;
        case Message::Highlight:
            g_frontend.SetHighlight(command.value != 0);
            break;
        case Message::RunScript:
            reinterpret_cast<ScheduleFn>(0x00489B70)(0, command.text.c_str(), 0, 0);
            Log("Scheduled: %s", command.text.c_str());
            break;
        default:
            break;
        }

        m_lastSnapshot = 0.0;
    }

    void CheckPick()
    {
        HWND window = GameWindow();
        bool down = window && GetForegroundWindow() == window && (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
        bool pressed = down && !m_mouseDown;
        m_mouseDown = down;

        if (!pressed)
        {
            return;
        }

        POINT cursor;
        RECT client;

        if (!GetCursorPos(&cursor) || !ScreenToClient(window, &cursor) || !GetClientRect(window, &client))
        {
            return;
        }

        float width = static_cast<float>(client.right), height = static_cast<float>(client.bottom);

        if (width <= 0 || height <= 0)
        {
            return;
        }

        float pageWidth = static_cast<float>(frontend::App::GetScreenWidth());
        float scale = 0.93333334f * std::min(width / pageWidth, height / 480.0f);
        PickRecord pick = {pageWidth * 0.5f + (cursor.x - width * 0.5f) / scale,
            480.0f * 0.5f + (height * 0.5f - cursor.y) / scale};
        std::vector<uint8_t> bytes;
        AppendMessage(bytes, Message::Pick, pick);
        g_server.Publish(std::move(bytes));
    }

    void SendSnapshot()
    {
        const std::vector<NodeRecord>& nodes = g_frontend.Nodes();
        std::vector<uint8_t> bytes;
        PlayerRecord player;

        if (ReadPlayer(player))
        {
            AppendMessage(bytes, Message::Player, player);
        }

        AppendNames(bytes, nodes);
        SnapshotHeader snapshot = {++m_frame, static_cast<uint32_t>(nodes.size())};
        Header header = {static_cast<uint32_t>(sizeof(snapshot) + nodes.size() * sizeof(NodeRecord)),
            static_cast<uint32_t>(Message::Snapshot)};
        const uint8_t* h = reinterpret_cast<const uint8_t*>(&header);
        const uint8_t* s = reinterpret_cast<const uint8_t*>(&snapshot);
        const uint8_t* n = reinterpret_cast<const uint8_t*>(nodes.data());
        bytes.insert(bytes.end(), h, h + sizeof(header));
        bytes.insert(bytes.end(), s, s + sizeof(snapshot));
        bytes.insert(bytes.end(), n, n + nodes.size() * sizeof(NodeRecord));
        g_server.Publish(std::move(bytes));
    }

    void AppendNames(std::vector<uint8_t>& bytes, const std::vector<NodeRecord>& nodes)
    {
        std::vector<uint8_t> payload;
        {
            std::lock_guard<std::mutex> lock(g_namesMutex);

            for (const NodeRecord& node : nodes)
            {
                if (m_sentKeys.count(node.key))
                {
                    continue;
                }

                auto found = g_names.find(node.key);

                if (found == g_names.end())
                {
                    continue;
                }

                m_sentKeys.insert(node.key);
                const std::string& name = found->second;
                uint8_t length = static_cast<uint8_t>(std::min<size_t>(name.size(), 255));
                const uint8_t* key = reinterpret_cast<const uint8_t*>(&node.key);
                payload.insert(payload.end(), key, key + 4);
                payload.push_back(length);
                payload.insert(payload.end(), name.begin(), name.begin() + length);
            }
        }

        if (payload.empty())
        {
            return;
        }

        Header header = {static_cast<uint32_t>(payload.size()), static_cast<uint32_t>(Message::Names)};
        const uint8_t* h = reinterpret_cast<const uint8_t*>(&header);
        bytes.insert(bytes.end(), h, h + sizeof(header));
        bytes.insert(bytes.end(), payload.begin(), payload.end());
    }

    bool m_wasConnected = false;
    bool m_pickMode = false;
    bool m_mouseDown = false;
    uint32_t m_connection = 0;
    uint32_t m_frame = 0;
    double m_lastSnapshot = 0.0;
    std::unordered_set<uint32_t> m_sentKeys;
};

Link g_link;

void __fastcall AppDraw(frontend::App* app, void*, bool loading)
{
    g_link.OnFrame();
    g_appDraw(app, loading);
}

core::Key __fastcall KeySet(core::Key* key, void*, const char* name)
{
    core::Key result = g_keySet(key, name);

    if (name && *name)
    {
        std::lock_guard<std::mutex> lock(g_namesMutex);

        if (g_names.size() < kMaxNames)
        {
            g_names[result] = name;
        }
    }

    return result;
}

bool Install()
{
    if (!SDK::IsGameVersionValid())
    {
        Log("Unsupported scarface.exe, only US 1.00.2 is supported.");
        return false;
    }

    MH_STATUS status = MH_Initialize();

    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
    {
        Log("MinHook init failed (%d).", static_cast<int>(status));
        return false;
    }

    struct Hook
    {
        uintptr_t target;
        void* detour;
        void** original;
        const char* name;
    } hooks[] = {
        {0x00678C50,
            reinterpret_cast<void*>(&AppDraw),
            reinterpret_cast<void**>(&g_appDraw),
            "pure3d::frontend::App::Draw"},
        {0x006683C0, reinterpret_cast<void*>(&KeySet), reinterpret_cast<void**>(&g_keySet), "core::Key::Set"},
    };

    for (const Hook& hook : hooks)
    {
        status = MH_CreateHook(reinterpret_cast<void*>(hook.target), hook.detour, hook.original);

        if (status != MH_OK)
        {
            Log("Hook %s at %08X failed (%d).",
                hook.name,
                static_cast<unsigned>(hook.target),
                static_cast<int>(status));
            return false;
        }
    }

    g_server.Start();
    Log("ToolKitLink ready, waiting for the ToolKit.");
    return true;
}

}

BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        Install();
    }

    return TRUE;
}
