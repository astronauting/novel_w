#include "Hotkeys.h"

#include <commctrl.h>
#include <algorithm>
#include <cwctype>

namespace {

constexpr wchar_t kDlgClass[] = L"novel_wHotkeyDlg";
constexpr int IDC_KEY_LIST = 5001;
constexpr int IDC_KEY_HINT = 5002;
constexpr int IDC_KEY_RESET = 5003;
constexpr int IDC_KEY_CURRENT = 5004;
constexpr int IDC_KEY_OK = IDOK;
constexpr int IDC_KEY_CANCEL = IDCANCEL;

const KeySpec kSpecs[kKeyCount] = {
    {KeyId::FileNew,        L"fileNew",        L"新建",         L"新建(&N)",       IDM_FILE_NEW,        { 'N', true,  false, false }, false},
    {KeyId::FileOpen,       L"fileOpen",       L"打开",         L"打开(&O)",       IDM_FILE_OPEN,       { 'O', true,  false, false }, false},
    {KeyId::FileSave,       L"fileSave",       L"保存",         L"保存(&S)",       IDM_FILE_SAVE,       { 'S', true,  false, false }, false},
    {KeyId::FileSaveAs,     L"fileSaveAs",     L"另存为",       L"另存为(&A)",     IDM_FILE_SAVEAS,     { 'S', true,  false, true  }, false},
    {KeyId::EditUndo,       L"editUndo",       L"撤销",         L"撤销(&U)",       IDM_EDIT_UNDO,       { 'Z', true,  false, false }, false},
    {KeyId::EditRedo,       L"editRedo",       L"重做",         L"重做(&R)",       IDM_EDIT_REDO,       { 'Y', true,  false, false }, false},
    {KeyId::EditCut,        L"editCut",        L"剪切",         L"剪切(&T)",       IDM_EDIT_CUT,        { 'X', true,  false, false }, false},
    {KeyId::EditCopy,       L"editCopy",       L"复制",         L"复制(&C)",       IDM_EDIT_COPY,       { 'C', true,  false, false }, false},
    {KeyId::EditPaste,      L"editPaste",      L"粘贴",         L"粘贴(&P)",       IDM_EDIT_PASTE,      { 'V', true,  false, false }, false},
    {KeyId::EditSelectAll,  L"editSelectAll",  L"全选",         L"全选(&A)",       IDM_EDIT_SELECTALL,  { 'A', true,  false, false }, false},
    {KeyId::EditFind,       L"editFind",       L"查找",         L"查找(&F)",       IDM_EDIT_FIND,       { 'F', true,  false, false }, false},
    {KeyId::ChapterRefresh, L"chapterRefresh", L"刷新目录",     L"刷新目录(&R)",   IDM_CHAPTER_REFRESH, { 'R', true,  false, true  }, false},
    {KeyId::ChapterPrev,    L"chapterPrev",    L"上一章",       L"上一章(&P)",     IDM_CHAPTER_PREV,    { VK_PRIOR, true, false, false }, false},
    {KeyId::ChapterNext,    L"chapterNext",    L"下一章",       L"下一章(&N)",     IDM_CHAPTER_NEXT,    { VK_NEXT,  true, false, false }, false},
    {KeyId::ChapterInsert,  L"chapterInsert",  L"插入章节标题", L"插入章节标题(&I)", IDM_CHAPTER_INSERT, { 'I', true, false, true }, false},
    {KeyId::ViewBorder,     L"viewBorder",     L"隐藏边框",     L"隐藏边框(&B)",   IDM_VIEW_BORDER,     { VK_F12, false, false, false }, false},
    {KeyId::ViewHide,       L"viewHide",       L"一键隐藏",     L"一键隐藏(&H)",   IDM_VIEW_HIDE,       { 'H', false, true,  false }, true},
    {KeyId::ViewFont,       L"viewFont",       L"字体",         L"字体(&F)...",    IDM_VIEW_FONT,       { 0, false, false, false }, false},
    {KeyId::ViewColor,      L"viewColor",      L"背景颜色",     L"背景颜色(&C)...", IDM_VIEW_COLOR,     { 0, false, false, false }, false},
    {KeyId::ViewPicker,     L"viewPicker",     L"吸管取色",     L"吸管取色(&P)",   IDM_VIEW_PICKER,     { 'P', true, false, true }, false},
    {KeyId::ViewFontInc,    L"viewFontInc",    L"增大字号",     L"增大字号",       IDM_VIEW_FONT_INC,   { VK_OEM_PLUS, true, false, false }, false},
    {KeyId::ViewFontDec,    L"viewFontDec",    L"减小字号",     L"减小字号",       IDM_VIEW_FONT_DEC,   { VK_OEM_MINUS, true, false, false }, false},
};

std::wstring VkName(UINT vk) {
    if (vk >= 'A' && vk <= 'Z') return std::wstring(1, (wchar_t)vk);
    if (vk >= '0' && vk <= '9') return std::wstring(1, (wchar_t)vk);
    if (vk >= VK_F1 && vk <= VK_F24) {
        wchar_t buf[8];
        wsprintfW(buf, L"F%d", vk - VK_F1 + 1);
        return buf;
    }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
        wchar_t buf[12];
        wsprintfW(buf, L"Num%d", vk - VK_NUMPAD0);
        return buf;
    }
    switch (vk) {
        case VK_ADD: return L"Num+";
        case VK_OEM_PLUS: return L"+";
        case VK_SUBTRACT: return L"Num-";
        case VK_OEM_MINUS: return L"-";
        case VK_MULTIPLY: return L"Num*";
        case VK_DIVIDE: return L"Num/";
        case VK_SPACE: return L"Space";
        case VK_TAB: return L"Tab";
        case VK_ESCAPE: return L"Esc";
        case VK_RETURN: return L"Enter";
        case VK_BACK: return L"Backspace";
        case VK_INSERT: return L"Insert";
        case VK_DELETE: return L"Delete";
        case VK_HOME: return L"Home";
        case VK_END: return L"End";
        case VK_PRIOR: return L"PageUp";
        case VK_NEXT: return L"PageDown";
        case VK_LEFT: return L"Left";
        case VK_RIGHT: return L"Right";
        case VK_UP: return L"Up";
        case VK_DOWN: return L"Down";
        case VK_OEM_COMMA: return L",";
        case VK_OEM_PERIOD: return L".";
        case VK_OEM_1: return L";";
        case VK_OEM_2: return L"/";
        case VK_OEM_3: return L"`";
        case VK_OEM_4: return L"[";
        case VK_OEM_6: return L"]";
        case VK_OEM_7: return L"'";
        case VK_OEM_5: return L"\\";
        default: {
            wchar_t buf[16];
            wsprintfW(buf, L"Vk%d", vk);
            return buf;
        }
    }
}

bool IsModifierVk(UINT vk) {
    return vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
           vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
           vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
           vk == VK_LWIN || vk == VK_RWIN || vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL;
}

bool ModifyMenuById(HMENU menu, UINT id, const wchar_t* text) {
    int n = GetMenuItemCount(menu);
    for (int i = 0; i < n; ++i) {
        if (GetMenuItemID(menu, i) == id) {
            MENUITEMINFOW mii{};
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_STRING;
            mii.dwTypeData = const_cast<wchar_t*>(text);
            mii.cch = (UINT)wcslen(text);
            return SetMenuItemInfoW(menu, i, TRUE, &mii) != FALSE;
        }
        HMENU sub = GetSubMenu(menu, i);
        if (sub && ModifyMenuById(sub, id, text)) return true;
    }
    return false;
}

int ExtractIntNear(const std::wstring& json, size_t from, size_t to, const wchar_t* key, int def) {
    std::wstring pat = std::wstring(L"\"") + key + L"\"";
    size_t p = json.find(pat, from);
    if (p == std::wstring::npos || p >= to) return def;
    p = json.find(L':', p + pat.size());
    if (p == std::wstring::npos || p >= to) return def;
    ++p;
    while (p < to && iswspace(json[p])) ++p;
    bool neg = false;
    if (p < to && json[p] == L'-') {
        neg = true;
        ++p;
    }
    if (p >= to || !iswdigit(json[p])) return def;
    long v = 0;
    while (p < to && iswdigit(json[p])) {
        v = v * 10 + (json[p] - L'0');
        ++p;
    }
    return neg ? -(int)v : (int)v;
}

struct DlgState {
    HWND owner = nullptr;
    HWND dlg = nullptr;
    HWND list = nullptr;
    std::vector<KeyChord>* keys = nullptr;
    std::vector<KeyChord> original;
    std::vector<KeyChord> working;
    bool accepted = false;
};

void SetListSubItem(HWND list, int row, int col, const std::wstring& text) {
    LVITEMW it{};
    it.mask = LVIF_TEXT;
    it.iItem = row;
    it.iSubItem = col;
    it.pszText = const_cast<wchar_t*>(text.c_str());
    SendMessageW(list, LVM_SETITEMTEXTW, (WPARAM)row, (LPARAM)&it);
}

void FillList(HWND list, const std::vector<KeyChord>& keys) {
    SendMessageW(list, LVM_DELETEALLITEMS, 0, 0);
    for (int i = 0; i < kKeyCount; ++i) {
        const KeySpec& spec = kSpecs[i];
        LVITEMW item{};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.pszText = const_cast<wchar_t*>(spec.label);
        item.lParam = i;
        int row = (int)SendMessageW(list, LVM_INSERTITEMW, 0, (LPARAM)&item);
        if (row < 0) row = i;
        SetListSubItem(list, row, 1, FormatChord(keys[i]));
        SetListSubItem(list, row, 2, spec.global ? L"全局" : L"窗口内");
    }
}

int SelectedRow(HWND list) {
    return (int)SendMessageW(list, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
}

void SetHint(HWND dlg, const wchar_t* text) {
    SetWindowTextW(GetDlgItem(dlg, IDC_KEY_HINT), text);
}

void UpdateCurrentLabel(DlgState* st) {
    if (!st || !st->dlg) return;
    int row = SelectedRow(st->list);
    std::wstring s = L"当前：先在列表里点选一项，然后按下新的快捷键。菜单上的键位会立刻更新。";
    if (row >= 0 && row < kKeyCount && row < (int)st->working.size()) {
        s = L"当前：";
        s += kSpecs[row].label;
        s += L"  →  ";
        s += FormatChord(st->working[row]);
        if (kSpecs[row].global) s += L"  （全局，隐藏后靠它唤回）";
    }
    SetWindowTextW(GetDlgItem(st->dlg, IDC_KEY_CURRENT), s.c_str());
}

void PreviewMenu(DlgState* st) {
    if (!st || !st->owner) return;
    HMENU menu = GetMenu(st->owner);
    if (!menu) return;
    ApplyMenuHotkeys(menu, st->working);
    DrawMenuBar(st->owner);
}

bool ShouldNavigateList(UINT vk, bool hasMod) {
    if (hasMod) return false;
    return vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT ||
           vk == VK_PRIOR || vk == VK_NEXT || vk == VK_HOME || vk == VK_END;
}

bool AssignHotkey(DlgState* st, UINT vk) {
    if (!st || !st->dlg) return false;
    if (IsModifierVk(vk)) return false;
    if (vk == VK_TAB) return false;
    if (vk == VK_RETURN) return false;
    if (vk == VK_ESCAPE) {
        DestroyWindow(st->dlg);
        return true;
    }

    bool hasMod = (GetKeyState(VK_CONTROL) & 0x8000) ||
                  (GetKeyState(VK_MENU) & 0x8000) ||
                  (GetKeyState(VK_SHIFT) & 0x8000);
    if (ShouldNavigateList(vk, hasMod)) return false;

    int row = SelectedRow(st->list);
    if (row < 0 || row >= kKeyCount) {
        SetHint(st->dlg, L"请先点选列表里的一项，再按下新快捷键。");
        return true;
    }

    KeyChord chord;
    chord.vk = vk;
    chord.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    chord.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    chord.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    for (int i = 0; i < kKeyCount; ++i) {
        if (i != row && SameChord(st->working[i], chord) && !ChordEmpty(chord)) {
            std::wstring msg = L"该组合已用于「";
            msg += kSpecs[i].label;
            msg += L"」，请换一个。";
            SetHint(st->dlg, msg.c_str());
            return true;
        }
    }

    st->working[row] = chord;
    SetListSubItem(st->list, row, 1, FormatChord(chord));
    UpdateCurrentLabel(st);
    PreviewMenu(st);

    std::wstring ok = L"「";
    ok += kSpecs[row].label;
    ok += L"」已改为 ";
    ok += FormatChord(chord);
    ok += L"。菜单已同步，点确定保存。";
    SetHint(st->dlg, ok.c_str());
    return true;
}

LRESULT CALLBACK HotkeyDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void EnsureDlgClass(HINSTANCE inst) {
    static bool done = false;
    if (done) return;
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = HotkeyDlgProc;
    wc.hInstance = inst;
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = kDlgClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);
    done = true;
}

LRESULT CALLBACK HotkeyDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    DlgState* st = (DlgState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (msg) {
        case WM_CREATE: {
            auto* cs = (CREATESTRUCTW*)lParam;
            st = (DlgState*)cs->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)st);
            st->dlg = hwnd;

            CreateWindowW(L"STATIC", L"",
                          WS_CHILD | WS_VISIBLE,
                          12, 10, 500, 36, hwnd, (HMENU)(INT_PTR)IDC_KEY_CURRENT, cs->hInstance, nullptr);

            HWND list = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                        12, 50, 500, 250, hwnd, (HMENU)(INT_PTR)IDC_KEY_LIST, cs->hInstance, nullptr);
            SendMessageW(list, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            LVCOLUMNW col{};
            col.mask = LVCF_TEXT | LVCF_WIDTH;
            col.pszText = const_cast<wchar_t*>(L"功能");
            col.cx = 160;
            SendMessageW(list, LVM_INSERTCOLUMNW, 0, (LPARAM)&col);
            col.pszText = const_cast<wchar_t*>(L"快捷键");
            col.cx = 180;
            SendMessageW(list, LVM_INSERTCOLUMNW, 1, (LPARAM)&col);
            col.pszText = const_cast<wchar_t*>(L"范围");
            col.cx = 140;
            SendMessageW(list, LVM_INSERTCOLUMNW, 2, (LPARAM)&col);
            st->list = list;
            FillList(list, st->working);

            LVITEMW sel{};
            sel.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
            sel.state = LVIS_SELECTED | LVIS_FOCUSED;
            SendMessageW(list, LVM_SETITEMSTATE, 0, (LPARAM)&sel);

            CreateWindowW(L"STATIC",
                          L"点选一行后直接按键盘。Esc 取消，确定后写入设置。",
                          WS_CHILD | WS_VISIBLE,
                          12, 308, 500, 24, hwnd, (HMENU)(INT_PTR)IDC_KEY_HINT, cs->hInstance, nullptr);
            CreateWindowW(L"BUTTON", L"恢复默认", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                          12, 344, 100, 28, hwnd, (HMENU)(INT_PTR)IDC_KEY_RESET, cs->hInstance, nullptr);
            CreateWindowW(L"BUTTON", L"确定", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                          312, 344, 90, 28, hwnd, (HMENU)(INT_PTR)IDC_KEY_OK, cs->hInstance, nullptr);
            CreateWindowW(L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                          414, 344, 90, 28, hwnd, (HMENU)(INT_PTR)IDC_KEY_CANCEL, cs->hInstance, nullptr);
            UpdateCurrentLabel(st);
            SetFocus(list);
            return 0;
        }
        case WM_COMMAND:
            if (!st) break;
            if (LOWORD(wParam) == IDC_KEY_RESET) {
                InitDefaultHotkeys(st->working);
                FillList(st->list, st->working);
                LVITEMW sel{};
                sel.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                sel.state = LVIS_SELECTED | LVIS_FOCUSED;
                SendMessageW(st->list, LVM_SETITEMSTATE, 0, (LPARAM)&sel);
                UpdateCurrentLabel(st);
                PreviewMenu(st);
                SetHint(hwnd, L"已恢复默认。菜单已同步，点确定保存。");
                SetFocus(st->list);
                return 0;
            }
            if (LOWORD(wParam) == IDOK) {
                *st->keys = st->working;
                st->accepted = true;
                DestroyWindow(hwnd);
                return 0;
            }
            if (LOWORD(wParam) == IDCANCEL) {
                DestroyWindow(hwnd);
                return 0;
            }
            return 0;
        case WM_NOTIFY:
            if (st && ((NMHDR*)lParam)->idFrom == IDC_KEY_LIST &&
                ((NMHDR*)lParam)->code == LVN_ITEMCHANGED) {
                UpdateCurrentLabel(st);
            }
            return 0;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

}  // namespace

const KeySpec& KeyAt(int index) {
    return kSpecs[index];
}

const KeySpec* FindKeyByCommand(int commandId) {
    for (int i = 0; i < kKeyCount; ++i) {
        if (kSpecs[i].commandId == commandId) return &kSpecs[i];
    }
    return nullptr;
}

const KeySpec* FindKeyById(KeyId id) {
    int i = (int)id;
    if (i < 0 || i >= kKeyCount) return nullptr;
    return &kSpecs[i];
}

KeyChord DefaultChord(KeyId id) {
    const KeySpec* s = FindKeyById(id);
    return s ? s->def : KeyChord{};
}

void InitDefaultHotkeys(std::vector<KeyChord>& keys) {
    keys.resize(kKeyCount);
    for (int i = 0; i < kKeyCount; ++i) keys[i] = kSpecs[i].def;
}

std::wstring FormatChord(const KeyChord& c) {
    if (ChordEmpty(c)) return L"（无）";
    std::wstring s;
    if (c.ctrl) s += L"Ctrl+";
    if (c.alt) s += L"Alt+";
    if (c.shift) s += L"Shift+";
    s += VkName(c.vk);
    return s;
}

bool SameChord(const KeyChord& a, const KeyChord& b) {
    return a.vk == b.vk && a.ctrl == b.ctrl && a.alt == b.alt && a.shift == b.shift;
}

bool ChordEmpty(const KeyChord& c) {
    return c.vk == 0;
}

HACCEL BuildAccelTable(const std::vector<KeyChord>& keys) {
    ACCEL accels[kKeyCount + 4]{};
    int n = 0;
    int count = (int)keys.size();
    if (count > kKeyCount) count = kKeyCount;
    for (int i = 0; i < count; ++i) {
        const KeyChord& c = keys[i];
        if (ChordEmpty(c)) continue;
        ACCEL a{};
        a.fVirt = FVIRTKEY;
        if (c.ctrl) a.fVirt |= FCONTROL;
        if (c.alt) a.fVirt |= FALT;
        if (c.shift) a.fVirt |= FSHIFT;
        a.key = (WORD)c.vk;
        a.cmd = (WORD)kSpecs[i].commandId;
        accels[n++] = a;
        if (kSpecs[i].id == KeyId::ViewFontInc && c.vk == VK_OEM_PLUS && c.ctrl && !c.alt && !c.shift) {
            ACCEL extra = a;
            extra.key = VK_ADD;
            accels[n++] = extra;
        }
        if (kSpecs[i].id == KeyId::ViewFontDec && c.vk == VK_OEM_MINUS && c.ctrl && !c.alt && !c.shift) {
            ACCEL extra = a;
            extra.key = VK_SUBTRACT;
            accels[n++] = extra;
        }
    }
    if (n <= 0) return nullptr;
    return CreateAcceleratorTableW(accels, n);
}

void DestroyAccel(HACCEL* accel) {
    if (accel && *accel) {
        DestroyAcceleratorTable(*accel);
        *accel = nullptr;
    }
}

void ApplyMenuHotkeys(HMENU menu, const std::vector<KeyChord>& keys) {
    if (!menu) return;
    int count = (int)keys.size();
    if (count > kKeyCount) count = kKeyCount;
    for (int i = 0; i < count; ++i) {
        std::wstring text = kSpecs[i].menuText;
        if (!ChordEmpty(keys[i])) {
            text += L"\t";
            text += FormatChord(keys[i]);
        }
        ModifyMenuById(menu, (UINT)kSpecs[i].commandId, text.c_str());
    }
}

bool RegisterGlobalHotkeys(HWND hwnd, const std::vector<KeyChord>& keys) {
    UnregisterGlobalHotkeys(hwnd);
    bool ok = true;
    int count = (int)keys.size();
    if (count > kKeyCount) count = kKeyCount;
    for (int i = 0; i < count; ++i) {
        if (!kSpecs[i].global) continue;
        const KeyChord& c = keys[i];
        if (ChordEmpty(c)) continue;
        UINT mods = MOD_NOREPEAT;
        if (c.ctrl) mods |= MOD_CONTROL;
        if (c.alt) mods |= MOD_ALT;
        if (c.shift) mods |= MOD_SHIFT;
        if (!RegisterHotKey(hwnd, HOTKEY_BASE + i, mods, c.vk)) ok = false;
    }
    return ok;
}

void UnregisterGlobalHotkeys(HWND hwnd) {
    for (int i = 0; i < kKeyCount; ++i) {
        UnregisterHotKey(hwnd, HOTKEY_BASE + i);
    }
}

std::wstring HotkeyHelpText(const std::vector<KeyChord>& keys) {
    std::wstring s;
    int count = (int)keys.size();
    if (count > kKeyCount) count = kKeyCount;
    for (int i = 0; i < count; ++i) {
        s += FormatChord(keys[i]);
        s += L"  ";
        s += kSpecs[i].label;
        if (kSpecs[i].global) s += L"（全局）";
        s += L"\n";
    }
    s += L"Ctrl+滚轮  背景透明度\n";
    s += L"\n可在「设置 → 快捷键」里修改。";
    return s;
}

bool EditHotkeysDialog(HWND owner, std::vector<KeyChord>& keys) {
    HINSTANCE inst = (HINSTANCE)GetWindowLongPtrW(owner, GWLP_HINSTANCE);
    EnsureDlgClass(inst);
    DlgState st;
    st.owner = owner;
    st.keys = &keys;
    st.original = keys;
    st.working = keys;
    if ((int)st.working.size() != kKeyCount) InitDefaultHotkeys(st.working);
    if ((int)st.original.size() != kKeyCount) st.original = st.working;

    UnregisterGlobalHotkeys(owner);

    RECT orc{};
    GetWindowRect(owner, &orc);
    int w = 540, h = 420;
    int x = orc.left + ((orc.right - orc.left) - w) / 2;
    int y = orc.top + ((orc.bottom - orc.top) - h) / 2;

    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, kDlgClass, L"快捷键",
                               WS_CAPTION | WS_SYSMENU | WS_POPUP,
                               x, y, w, h, owner, nullptr, inst, &st);
    if (!dlg) {
        RegisterGlobalHotkeys(owner, st.original);
        return false;
    }
    EnableWindow(owner, FALSE);
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    SetFocus(st.list);

    MSG msg{};
    while (IsWindow(dlg) && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) {
            HWND target = msg.hwnd;
            if (target == dlg || IsChild(dlg, target)) {
                if (AssignHotkey(&st, (UINT)msg.wParam)) continue;
            }
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);
    if (!st.accepted) {
        HMENU menu = GetMenu(owner);
        if (menu) {
            ApplyMenuHotkeys(menu, st.original);
            DrawMenuBar(owner);
        }
        RegisterGlobalHotkeys(owner, st.original);
    }
    return st.accepted;
}

void ParseHotkeysJson(const std::wstring& json, std::vector<KeyChord>& keys) {
    if ((int)keys.size() != kKeyCount) InitDefaultHotkeys(keys);
    size_t block = json.find(L"\"hotkeys\"");
    if (block == std::wstring::npos) return;
    size_t brace = json.find(L'{', block);
    if (brace == std::wstring::npos) return;
    int depth = 0;
    size_t end = brace;
    for (; end < json.size(); ++end) {
        if (json[end] == L'{') ++depth;
        else if (json[end] == L'}') {
            --depth;
            if (depth == 0) {
                ++end;
                break;
            }
        }
    }
    for (int i = 0; i < kKeyCount; ++i) {
        std::wstring pat = std::wstring(L"\"") + kSpecs[i].jsonId + L"\"";
        size_t p = json.find(pat, brace);
        if (p == std::wstring::npos || p >= end) continue;
        size_t ob = json.find(L'{', p);
        if (ob == std::wstring::npos || ob >= end) continue;
        size_t oe = json.find(L'}', ob);
        if (oe == std::wstring::npos || oe >= end) continue;
        keys[i].vk = (UINT)ExtractIntNear(json, ob, oe, L"vk", keys[i].vk);
        keys[i].ctrl = ExtractIntNear(json, ob, oe, L"ctrl", keys[i].ctrl ? 1 : 0) != 0;
        keys[i].alt = ExtractIntNear(json, ob, oe, L"alt", keys[i].alt ? 1 : 0) != 0;
        keys[i].shift = ExtractIntNear(json, ob, oe, L"shift", keys[i].shift ? 1 : 0) != 0;
    }
}

void AppendHotkeysJson(std::wstring& out, const std::vector<KeyChord>& keys) {
    out += L"  \"hotkeys\": {\n";
    int count = (int)keys.size();
    if (count > kKeyCount) count = kKeyCount;
    for (int i = 0; i < count; ++i) {
        wchar_t line[192];
        wsprintfW(line,
                  L"    \"%s\": { \"vk\": %u, \"ctrl\": %d, \"alt\": %d, \"shift\": %d }%s\n",
                  kSpecs[i].jsonId,
                  keys[i].vk,
                  keys[i].ctrl ? 1 : 0,
                  keys[i].alt ? 1 : 0,
                  keys[i].shift ? 1 : 0,
                  (i + 1 < count) ? L"," : L"");
        out += line;
    }
    out += L"  }";
}
