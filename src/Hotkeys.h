#pragma once

#include "resource.h"

#include <windows.h>
#include <string>
#include <vector>

enum class KeyId : int {
    FileNew = 0,
    FileOpen,
    FileSave,
    FileSaveAs,
    EditUndo,
    EditRedo,
    EditCut,
    EditCopy,
    EditPaste,
    EditSelectAll,
    EditFind,
    ChapterRefresh,
    ChapterPrev,
    ChapterNext,
    ChapterInsert,
    ViewBorder,
    ViewHide,
    ViewFont,
    ViewColor,
    ViewPicker,
    ViewFontInc,
    ViewFontDec,
    Count
};

struct KeyChord {
    UINT vk = 0;
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
};

struct KeySpec {
    KeyId id;
    const wchar_t* jsonId;
    const wchar_t* label;
    const wchar_t* menuText;
    int commandId;
    KeyChord def;
    bool global;
};

constexpr int kKeyCount = (int)KeyId::Count;

const KeySpec& KeyAt(int index);
const KeySpec* FindKeyByCommand(int commandId);
const KeySpec* FindKeyById(KeyId id);

KeyChord DefaultChord(KeyId id);
void InitDefaultHotkeys(std::vector<KeyChord>& keys);
std::wstring FormatChord(const KeyChord& c);
bool SameChord(const KeyChord& a, const KeyChord& b);
bool ChordEmpty(const KeyChord& c);

HACCEL BuildAccelTable(const std::vector<KeyChord>& keys);
void DestroyAccel(HACCEL* accel);
void ApplyMenuHotkeys(HMENU menu, const std::vector<KeyChord>& keys);
bool RegisterGlobalHotkeys(HWND hwnd, const std::vector<KeyChord>& keys);
void UnregisterGlobalHotkeys(HWND hwnd);
std::wstring HotkeyHelpText(const std::vector<KeyChord>& keys);
bool EditHotkeysDialog(HWND owner, std::vector<KeyChord>& keys);

void ParseHotkeysJson(const std::wstring& json, std::vector<KeyChord>& keys);
void AppendHotkeysJson(std::wstring& out, const std::vector<KeyChord>& keys);
