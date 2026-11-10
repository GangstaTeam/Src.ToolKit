#include "app/App.hh"

#include <shellapi.h>
#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::filesystem::path> startupFiles(argv + (argc > 0 ? 1 : 0), argv + argc);
    LocalFree(argv);
    tk::App app;
    return app.Run(startupFiles);
}
