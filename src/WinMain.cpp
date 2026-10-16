#include "AppWindow.h"

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>

#pragma comment(lib, "comctl32.lib")

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

int APIENTRY wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int nCmdShow) {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto setDpi = (BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT))GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        if (setDpi) setDpi(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }

    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES | ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    if (!LoadLibraryW(L"Msftedit.dll")) {
        MessageBoxW(nullptr, L"无法加载 Msftedit.dll。", L"novel_w", MB_ICONERROR);
        return 1;
    }

    return RunApp(inst, nCmdShow, GetCommandLineW());
}
