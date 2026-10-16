#pragma once

#include "Hotkeys.h"
#include <windows.h>
#include <string>
#include <vector>

struct AppSettings {
    std::wstring fontName = L"Microsoft YaHei";
    int fontPoint = 14;
    COLORREF fontColor = RGB(32, 32, 32);
    COLORREF bgColor = RGB(255, 255, 255);
    int alpha = 255;
    int windowX = CW_USEDEFAULT;
    int windowY = CW_USEDEFAULT;
    int windowW = 1000;
    int windowH = 700;
    int treeWidth = 220;
    int lineSpacing = 28;
    std::wstring lastFile;
    std::vector<KeyChord> hotkeys;
};

std::wstring SettingsPath();
AppSettings LoadSettings();
bool SaveSettings(const AppSettings& s);
void ClampSettings(AppSettings& s);
