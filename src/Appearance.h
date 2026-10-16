#pragma once

#include "Settings.h"
#include <windows.h>

constexpr int kMinAlphaFramed = 0x64;
constexpr int kMinAlphaBorderless = 0x28;
constexpr int kAlphaStep = 5;

void ApplyWindowAlpha(HWND hwnd, int alpha);
int AdjustAlpha(int alpha, int delta, bool borderless);
bool ChooseBodyFont(HWND owner, AppSettings& s);
bool ChooseBgColor(HWND owner, AppSettings& s);
COLORREF SampleScreenColor(POINT screenPt);
