#pragma once

#include <windows.h>

int RunApp(HINSTANCE inst, int nCmdShow, const wchar_t* cmdLine);
LRESULT CALLBACK AppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
