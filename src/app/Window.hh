#pragma once

#include <windows.h>
#include <shellapi.h>

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace tk
{

class Window
{
public:
    bool Create(const wchar_t* title, int width, int height);
    void Destroy();
    bool PumpMessages();
    void SetTitle(const std::wstring& title);
    HWND Handle() const { return m_hwnd; }

    std::function<void(int, int)> onResize;
    std::function<void(const std::vector<std::filesystem::path>&)> onDropFiles;
    std::function<bool()> onCloseRequest;

private:
    static LRESULT CALLBACK StaticProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT Proc(UINT msg, WPARAM wparam, LPARAM lparam);
    void HandleDrop(HDROP drop);

    HWND m_hwnd = nullptr;
};

}
