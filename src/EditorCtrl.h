#pragma once

#include "Settings.h"
#include <windows.h>
#include <string>

HWND EditorCreate(HWND parent, HINSTANCE inst, int id);
void EditorLimitText(HWND editor);
std::wstring EditorGetText(HWND editor);
void EditorSetText(HWND editor, const std::wstring& text);
void EditorApplyFont(HWND editor, const AppSettings& s);
void EditorApplyColors(HWND editor, const AppSettings& s);
void EditorApplyLineSpacing(HWND editor, const AppSettings& s);
int EditorGetSelStart(HWND editor);
void EditorSetSel(HWND editor, int start, int end);
void EditorScrollToCaret(HWND editor);
void EditorInsertText(HWND editor, const wchar_t* text);
bool EditorFind(HWND editor, const wchar_t* needle, bool down);
int EditorCharCount(HWND editor);
void EditorUndo(HWND editor);
void EditorRedo(HWND editor);
void EditorZoomPoints(HWND editor, AppSettings& s, int delta);
void EditorSelectAll(HWND editor);
WNDPROC EditorSubclass(HWND editor, WNDPROC proc);
