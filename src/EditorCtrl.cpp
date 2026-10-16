#include "EditorCtrl.h"

#include <richedit.h>
#include <commctrl.h>

HWND EditorCreate(HWND parent, HINSTANCE inst, int id) {
    HWND hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        MSFTEDIT_CLASS,
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_AUTOVSCROLL | ES_WANTRETURN,
        0, 0, 0, 0,
        parent,
        (HMENU)(INT_PTR)id,
        inst,
        nullptr);
    if (!hwnd) return nullptr;
    EditorLimitText(hwnd);
    SendMessageW(hwnd, EM_SETEVENTMASK, 0, ENM_CHANGE | ENM_SELCHANGE);
    SendMessageW(hwnd, EM_SETTARGETDEVICE, 0, 0);
    SendMessageW(hwnd, EM_SETUNDOLIMIT, 200, 0);
    return hwnd;
}

void EditorLimitText(HWND editor) {
    SendMessageW(editor, EM_EXLIMITTEXT, 0, 256 * 1024 * 1024);
}

std::wstring EditorGetText(HWND editor) {
    GETTEXTLENGTHEX gtl{};
    gtl.flags = GTL_DEFAULT | GTL_NUMCHARS;
    gtl.codepage = 1200;
    int len = (int)SendMessageW(editor, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    if (len < 0) len = 0;
    std::wstring s(len + 1, 0);
    GETTEXTEX gt{};
    gt.cb = (DWORD)((len + 1) * sizeof(wchar_t));
    gt.flags = GT_DEFAULT;
    gt.codepage = 1200;
    int got = (int)SendMessageW(editor, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)s.data());
    if (got < 0) got = 0;
    s.resize((size_t)got);
    return s;
}

void EditorSetText(HWND editor, const std::wstring& text) {
    SETTEXTEX st{};
    st.flags = ST_DEFAULT;
    st.codepage = 1200;
    SendMessageW(editor, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)text.c_str());
}

void EditorApplyFont(HWND editor, const AppSettings& s) {
    CHARFORMAT2W cf{};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE | CFM_SIZE | CFM_COLOR | CFM_CHARSET | CFM_BOLD | CFM_ITALIC;
    cf.yHeight = s.fontPoint * 20;
    cf.crTextColor = s.fontColor;
    cf.bCharSet = DEFAULT_CHARSET;
    cf.bPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    lstrcpynW(cf.szFaceName, s.fontName.c_str(), LF_FACESIZE);
    SendMessageW(editor, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    SendMessageW(editor, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
    EditorApplyLineSpacing(editor, s);
}

void EditorApplyColors(HWND editor, const AppSettings& s) {
    SendMessageW(editor, EM_SETBKGNDCOLOR, 0, (LPARAM)s.bgColor);
    CHARFORMAT2W cf{};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = s.fontColor;
    SendMessageW(editor, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    SendMessageW(editor, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
}

void EditorApplyLineSpacing(HWND editor, const AppSettings& s) {
    PARAFORMAT2 pf{};
    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_LINESPACING | PFM_SPACEAFTER;
    pf.bLineSpacingRule = 5;
    pf.dyLineSpacing = s.lineSpacing;
    pf.dySpaceAfter = 0;
    SendMessageW(editor, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
}

int EditorGetSelStart(HWND editor) {
    CHARRANGE cr{};
    SendMessageW(editor, EM_EXGETSEL, 0, (LPARAM)&cr);
    return cr.cpMin;
}

void EditorSetSel(HWND editor, int start, int end) {
    CHARRANGE cr{};
    cr.cpMin = start;
    cr.cpMax = end;
    SendMessageW(editor, EM_EXSETSEL, 0, (LPARAM)&cr);
}

void EditorScrollToCaret(HWND editor) {
    SendMessageW(editor, EM_SCROLLCARET, 0, 0);
}

void EditorInsertText(HWND editor, const wchar_t* text) {
    SendMessageW(editor, EM_REPLACESEL, TRUE, (LPARAM)text);
}

bool EditorFind(HWND editor, const wchar_t* needle, bool down) {
    if (!needle || !needle[0]) return false;
    CHARRANGE cr{};
    SendMessageW(editor, EM_EXGETSEL, 0, (LPARAM)&cr);
    FINDTEXTEXW ft{};
    ft.lpstrText = needle;
    if (down) {
        ft.chrg.cpMin = cr.cpMax;
        ft.chrg.cpMax = -1;
    } else {
        ft.chrg.cpMin = cr.cpMin;
        ft.chrg.cpMax = 0;
    }
    DWORD flags = FR_DOWN;
    if (!down) flags = 0;
    LRESULT r = SendMessageW(editor, EM_FINDTEXTEXW, flags, (LPARAM)&ft);
    if (r == -1) {
        if (down) {
            ft.chrg.cpMin = 0;
            ft.chrg.cpMax = -1;
        } else {
            GETTEXTLENGTHEX gtl{GTL_DEFAULT | GTL_NUMCHARS, 1200};
            ft.chrg.cpMin = (LONG)SendMessageW(editor, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
            ft.chrg.cpMax = 0;
        }
        r = SendMessageW(editor, EM_FINDTEXTEXW, flags, (LPARAM)&ft);
    }
    if (r == -1) return false;
    SendMessageW(editor, EM_EXSETSEL, 0, (LPARAM)&ft.chrgText);
    EditorScrollToCaret(editor);
    return true;
}

int EditorCharCount(HWND editor) {
    GETTEXTLENGTHEX gtl{};
    gtl.flags = GTL_DEFAULT | GTL_NUMCHARS;
    gtl.codepage = 1200;
    return (int)SendMessageW(editor, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
}

void EditorUndo(HWND editor) {
    SendMessageW(editor, EM_UNDO, 0, 0);
}

void EditorRedo(HWND editor) {
    SendMessageW(editor, EM_REDO, 0, 0);
}

void EditorZoomPoints(HWND editor, AppSettings& s, int delta) {
    s.fontPoint += delta;
    if (s.fontPoint < 8) s.fontPoint = 8;
    if (s.fontPoint > 72) s.fontPoint = 72;
    EditorApplyFont(editor, s);
}

void EditorSelectAll(HWND editor) {
    EditorSetSel(editor, 0, -1);
}

WNDPROC EditorSubclass(HWND editor, WNDPROC proc) {
    return (WNDPROC)SetWindowLongPtrW(editor, GWLP_WNDPROC, (LONG_PTR)proc);
}
