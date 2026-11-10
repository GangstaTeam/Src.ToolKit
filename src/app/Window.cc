#include "app/Window.hh"

#include <imgui.h>

#include <dwmapi.h>
#include <shellapi.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace tk
{

namespace
{

const wchar_t kClassName[] = L"ToolKitWindow";

}

bool Window::Create(const wchar_t* title, int width, int height)
{
    WNDCLASSEXW wc = {sizeof(wc)};
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = &Window::StaticProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hIcon = LoadIconW(wc.hInstance, MAKEINTRESOURCEW(1));
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;
    RegisterClassExW(&wc);
    m_hwnd = CreateWindowExW(WS_EX_ACCEPTFILES,
        kClassName,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        nullptr,
        nullptr,
        wc.hInstance,
        this);
    if (!m_hwnd)
    {
        return false;
    }

    BOOL dark = TRUE;
    DwmSetWindowAttribute(m_hwnd, 20, &dark, sizeof(dark));
    ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
    UpdateWindow(m_hwnd);
    return true;
}

void Window::Destroy()
{
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
    }

    m_hwnd = nullptr;
    UnregisterClassW(kClassName, GetModuleHandleW(nullptr));
}

bool Window::PumpMessages()
{
    MSG msg;

    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        if (msg.message == WM_QUIT)
        {
            return false;
        }
    }

    return true;
}

void Window::SetTitle(const std::wstring& title)
{
    SetWindowTextW(m_hwnd, title.c_str());
}

LRESULT CALLBACK Window::StaticProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_NCCREATE)
    {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }

    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
    {
        return 1;
    }

    auto* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (window && window->m_hwnd)
    {
        return window->Proc(msg, wparam, lparam);
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

LRESULT Window::Proc(UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_SIZE:
        if (wparam != SIZE_MINIMIZED && onResize)
        {
            onResize(LOWORD(lparam), HIWORD(lparam));
        }

        return 0;
    case WM_DROPFILES:
        HandleDrop(reinterpret_cast<HDROP>(wparam));
        return 0;
    case WM_CLOSE:
        if (onCloseRequest && !onCloseRequest())
        {
            return 0;
        }

        DestroyWindow(m_hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SYSCOMMAND:
        if ((wparam & 0xfff0) == SC_KEYMENU)
        {
            return 0;
        }
        break;
    }

    return DefWindowProcW(m_hwnd, msg, wparam, lparam);
}

void Window::HandleDrop(HDROP drop)
{
    std::vector<std::filesystem::path> paths;
    UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);

    for (UINT i = 0; i < count; ++i)
    {
        wchar_t path[MAX_PATH];

        if (DragQueryFileW(drop, i, path, MAX_PATH))
        {
            paths.emplace_back(path);
        }
    }

    DragFinish(drop);

    if (onDropFiles && !paths.empty())
    {
        onDropFiles(paths);
    }
}

}
