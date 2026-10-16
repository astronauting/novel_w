#include "AppWindow.h"

#include "Appearance.h"
#include "ChapterIndex.h"
#include "EditorCtrl.h"
#include "FileIo.h"
#include "Hotkeys.h"
#include "resource.h"
#include "Settings.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <richedit.h>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

namespace {

constexpr wchar_t kClassName[] = L"novel_wWindow";
constexpr wchar_t kAppTitle[] = L"novel_w";
constexpr int kSplitterW = 5;
constexpr int kBorderMargin = 8;
constexpr UINT kChapterDebounceMs = 350;
constexpr UINT kAutosaveMs = 30000;

struct AppState {
    HINSTANCE inst = nullptr;
    HWND hwnd = nullptr;
    HWND tree = nullptr;
    HWND editor = nullptr;
    HWND status = nullptr;
    HWND splitter = nullptr;
    HMENU menu = nullptr;
    HACCEL accel = nullptr;
    AppSettings settings;
    std::wstring filePath;
    TextEncoding encoding = TextEncoding::Utf8;
    bool dirty = false;
    bool borderless = false;
    bool picking = false;
    bool hidden = false;
    bool updatingTree = false;
    bool draggingSplit = false;
    bool ignoreChange = false;
    bool hotkeyOk = false;
    int dragStartX = 0;
    int treeWidthStart = 0;
    std::vector<Chapter> chapters;
    int totalWords = 0;
    int chapterWords = 0;
    HWND findDlg = nullptr;
    FINDREPLACEW fr{};
    wchar_t findBuf[256]{};
    UINT findMsg = 0;
    WNDPROC editorPrev = nullptr;
    WNDPROC splitterPrev = nullptr;
    std::wstring pendingOpen;
};

AppState* g_app = nullptr;

std::wstring FileTitle(const AppState* a) {
    if (a->filePath.empty()) return L"未命名";
    return PathFindFileNameW(a->filePath.c_str());
}

void UpdateCaption(AppState* a) {
    std::wstring t = FileTitle(a);
    if (a->dirty) t += L" *";
    t += L" - ";
    t += kAppTitle;
    SetWindowTextW(a->hwnd, t.c_str());
}

void Layout(AppState* a) {
    RECT rc{};
    GetClientRect(a->hwnd, &rc);
    int cw = rc.right - rc.left;
    int ch = rc.bottom - rc.top;
    if (cw < 1 || ch < 1) return;

    int statusH = 0;
    if (!a->borderless && a->status) {
        SendMessageW(a->status, WM_SIZE, 0, 0);
        RECT sr{};
        GetWindowRect(a->status, &sr);
        statusH = sr.bottom - sr.top;
        int parts[4];
        parts[0] = (std::max)(80, cw - 420);
        parts[1] = (std::max)(parts[0] + 90, cw - 300);
        parts[2] = (std::max)(parts[1] + 110, cw - 160);
        parts[3] = -1;
        SendMessageW(a->status, SB_SETPARTS, 4, (LPARAM)parts);
    }

    if (a->borderless) {
        MoveWindow(a->editor, kBorderMargin, kBorderMargin,
                   (std::max)(20, cw - kBorderMargin * 2),
                   (std::max)(20, ch - kBorderMargin * 2), TRUE);
        return;
    }

    int tw = a->settings.treeWidth;
    if (tw > cw - 80) tw = cw - 80;
    if (tw < 120) tw = 120;
    a->settings.treeWidth = tw;
    int bodyH = (std::max)(20, ch - statusH);
    MoveWindow(a->tree, 0, 0, tw, bodyH, TRUE);
    MoveWindow(a->splitter, tw, 0, kSplitterW, bodyH, TRUE);
    MoveWindow(a->editor, tw + kSplitterW, 0, (std::max)(20, cw - tw - kSplitterW), bodyH, TRUE);
}

void UpdateStatus(AppState* a) {
    if (!a->status || a->borderless) return;
    wchar_t p0[MAX_PATH + 8];
    wchar_t p1[32];
    wchar_t p2[64];
    wchar_t p3[72];
    lstrcpynW(p0, a->filePath.empty() ? L"未保存" : a->filePath.c_str(), MAX_PATH);
    wsprintfW(p1, L"%s", EncodingName(a->encoding));
    wsprintfW(p2, L"全书 %d 字", a->totalWords);
    wsprintfW(p3, L"本章 %d 字  透明 %d%%", a->chapterWords, (a->settings.alpha * 100) / 255);
    SendMessageW(a->status, SB_SETTEXTW, 0, (LPARAM)p0);
    SendMessageW(a->status, SB_SETTEXTW, 1, (LPARAM)p1);
    SendMessageW(a->status, SB_SETTEXTW, 2, (LPARAM)p2);
    SendMessageW(a->status, SB_SETTEXTW, 3, (LPARAM)p3);
}

void RefreshChapters(AppState* a, bool jumpKeep) {
    std::wstring text = EditorGetText(a->editor);
    a->chapters = ParseChapters(text);
    int cp = EditorGetSelStart(a->editor);
    int cur = ChapterIndexAt(a->chapters, cp);
    a->totalWords = CountNonSpaceChars(text, 0, (int)text.size());
    if (cur >= 0) {
        int len = ChapterLength(a->chapters, cur, (int)text.size());
        a->chapterWords = CountNonSpaceChars(text, a->chapters[cur].start, a->chapters[cur].start + len);
    } else {
        a->chapterWords = a->totalWords;
    }

    a->updatingTree = true;
    TreeView_DeleteAllItems(a->tree);
    for (int i = 0; i < (int)a->chapters.size(); ++i) {
        TVINSERTSTRUCTW is{};
        is.hParent = TVI_ROOT;
        is.hInsertAfter = TVI_LAST;
        is.item.mask = TVIF_TEXT | TVIF_PARAM;
        is.item.pszText = const_cast<wchar_t*>(a->chapters[i].title.c_str());
        is.item.lParam = a->chapters[i].start;
        HTREEITEM h = TreeView_InsertItem(a->tree, &is);
        if (jumpKeep && i == cur) TreeView_SelectItem(a->tree, h);
    }
    a->updatingTree = false;
    UpdateStatus(a);
}

void ApplyAllAppearance(AppState* a) {
    a->ignoreChange = true;
    EditorApplyFont(a->editor, a->settings);
    EditorApplyColors(a->editor, a->settings);
    SendMessageW(a->editor, EM_SETMODIFY, FALSE, 0);
    a->ignoreChange = false;
    a->settings.alpha = AdjustAlpha(a->settings.alpha, 0, a->borderless);
    ApplyWindowAlpha(a->hwnd, a->settings.alpha);
    UpdateStatus(a);
}

void JumpToChapter(AppState* a, int start) {
    EditorSetSel(a->editor, start, start);
    EditorScrollToCaret(a->editor);
    SetFocus(a->editor);
    RefreshChapters(a, true);
}

bool PromptSaveIfDirty(AppState* a);

bool DoSaveAs(AppState* a);
bool DoSave(AppState* a) {
    if (a->filePath.empty()) return DoSaveAs(a);
    std::wstring text = EditorGetText(a->editor);
    if (!SaveTextFileUtf8(a->filePath, text)) {
        MessageBoxW(a->hwnd, L"保存失败。", kAppTitle, MB_ICONERROR);
        return false;
    }
    a->encoding = TextEncoding::Utf8;
    a->dirty = false;
    a->settings.lastFile = a->filePath;
    UpdateCaption(a);
    UpdateStatus(a);
    return true;
}

bool DoSaveAs(AppState* a) {
    wchar_t path[MAX_PATH] = {};
    if (!a->filePath.empty()) lstrcpynW(path, a->filePath.c_str(), MAX_PATH);
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = a->hwnd;
    ofn.lpstrFilter = L"文本文件 (*.txt)\0*.txt\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn)) return false;
    a->filePath = path;
    return DoSave(a);
}

void LoadPath(AppState* a, const std::wstring& path) {
    std::wstring text;
    TextEncoding enc = TextEncoding::Utf8;
    if (!LoadTextFile(path, text, &enc)) {
        MessageBoxW(a->hwnd, L"无法打开文件。", kAppTitle, MB_ICONERROR);
        return;
    }
    a->ignoreChange = true;
    EditorSetText(a->editor, text);
    a->filePath = path;
    a->encoding = enc;
    a->dirty = false;
    a->settings.lastFile = path;
    ApplyAllAppearance(a);
    a->ignoreChange = false;
    a->dirty = false;
    SendMessageW(a->editor, EM_SETMODIFY, FALSE, 0);
    RefreshChapters(a, false);
    UpdateCaption(a);
}

bool PromptSaveIfDirty(AppState* a) {
    if (!a->dirty) return true;
    std::wstring msg = L"是否保存对“" + FileTitle(a) + L"”的更改？";
    int r = MessageBoxW(a->hwnd, msg.c_str(), kAppTitle, MB_YESNOCANCEL | MB_ICONWARNING);
    if (r == IDCANCEL) return false;
    if (r == IDYES) return DoSave(a);
    return true;
}

void DoNew(AppState* a) {
    if (!PromptSaveIfDirty(a)) return;
    a->ignoreChange = true;
    EditorSetText(a->editor, L"");
    a->filePath.clear();
    a->encoding = TextEncoding::Utf8;
    a->dirty = false;
    ApplyAllAppearance(a);
    a->ignoreChange = false;
    a->dirty = false;
    SendMessageW(a->editor, EM_SETMODIFY, FALSE, 0);
    RefreshChapters(a, false);
    UpdateCaption(a);
}

void DoOpen(AppState* a) {
    if (!PromptSaveIfDirty(a)) return;
    wchar_t path[MAX_PATH] = {};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = a->hwnd;
    ofn.lpstrFilter = L"文本文件 (*.txt)\0*.txt\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;
    LoadPath(a, path);
}

void DoFind(AppState* a) {
    if (a->findDlg) {
        SetFocus(a->findDlg);
        return;
    }
    ZeroMemory(&a->fr, sizeof(a->fr));
    a->fr.lStructSize = sizeof(a->fr);
    a->fr.hwndOwner = a->hwnd;
    a->fr.lpstrFindWhat = a->findBuf;
    a->fr.wFindWhatLen = 256;
    a->fr.Flags = FR_DOWN;
    a->findDlg = FindTextW(&a->fr);
}

void InsertChapter(AppState* a) {
    std::wstring title = NextChapterTitle(a->chapters);
    int cp = EditorGetSelStart(a->editor);
    std::wstring ins;
    if (cp <= 0) ins = title + L"\r";
    else ins = L"\r" + title + L"\r";
    EditorInsertText(a->editor, ins.c_str());
    SetFocus(a->editor);
}

void GoChapter(AppState* a, int delta) {
    int cp = EditorGetSelStart(a->editor);
    int cur = ChapterIndexAt(a->chapters, cp);
    if (a->chapters.empty()) return;
    int next = cur + delta;
    if (cur < 0) next = (delta > 0) ? 0 : (int)a->chapters.size() - 1;
    if (next < 0) next = 0;
    if (next >= (int)a->chapters.size()) next = (int)a->chapters.size() - 1;
    JumpToChapter(a, a->chapters[next].start);
}

void SetBorderless(AppState* a, bool on) {
    a->borderless = on;
    LONG_PTR style = GetWindowLongPtrW(a->hwnd, GWL_STYLE);
    if (on) {
        style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        style |= WS_POPUP;
        SetMenu(a->hwnd, nullptr);
        ShowWindow(a->tree, SW_HIDE);
        ShowWindow(a->status, SW_HIDE);
        ShowWindow(a->splitter, SW_HIDE);
        SetWindowLongPtrW(a->editor, GWL_EXSTYLE,
                          GetWindowLongPtrW(a->editor, GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
        SetWindowPos(a->editor, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    } else {
        style &= ~WS_POPUP;
        style |= WS_OVERLAPPEDWINDOW;
        SetMenu(a->hwnd, a->menu);
        ShowWindow(a->tree, SW_SHOW);
        ShowWindow(a->status, SW_SHOW);
        ShowWindow(a->splitter, SW_SHOW);
        SetWindowLongPtrW(a->editor, GWL_EXSTYLE,
                          GetWindowLongPtrW(a->editor, GWL_EXSTYLE) | WS_EX_CLIENTEDGE);
        SetWindowPos(a->editor, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    SetWindowLongPtrW(a->hwnd, GWL_STYLE, style);
    SetWindowPos(a->hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    ApplyAllAppearance(a);
    Layout(a);
    SetFocus(a->editor);
}

void ToggleHide(AppState* a) {
    a->hidden = !a->hidden;
    ShowWindow(a->hwnd, a->hidden ? SW_HIDE : SW_SHOW);
    if (!a->hidden) {
        ShowWindow(a->hwnd, SW_SHOW);
        SetForegroundWindow(a->hwnd);
        SetFocus(a->editor);
    }
}

void StartPicker(AppState* a) {
    a->picking = true;
    SetCapture(a->hwnd);
    SetCursor(LoadCursorW(nullptr, IDC_CROSS));
}

void StopPicker(AppState* a) {
    if (!a->picking) return;
    a->picking = false;
    ReleaseCapture();
    SetCursor(LoadCursorW(nullptr, IDC_ARROW));
}

void PickerSample(AppState* a) {
    POINT pt{};
    GetCursorPos(&pt);
    COLORREF c = SampleScreenColor(pt);
    if (c == CLR_INVALID) return;
    a->settings.bgColor = c;
    EditorApplyColors(a->editor, a->settings);
}

void PersistWindow(AppState* a) {
    if (IsIconic(a->hwnd) || IsZoomed(a->hwnd) || a->borderless) return;
    RECT r{};
    GetWindowRect(a->hwnd, &r);
    a->settings.windowX = r.left;
    a->settings.windowY = r.top;
    a->settings.windowW = r.right - r.left;
    a->settings.windowH = r.bottom - r.top;
}

void ApplyHotkeys(AppState* a) {
    if ((int)a->settings.hotkeys.size() != kKeyCount) InitDefaultHotkeys(a->settings.hotkeys);
    UnregisterGlobalHotkeys(a->hwnd);
    DestroyAccel(&a->accel);
    a->accel = BuildAccelTable(a->settings.hotkeys);
    HMENU menu = GetMenu(a->hwnd);
    if (!menu) menu = a->menu;
    ApplyMenuHotkeys(menu, a->settings.hotkeys);
    a->hotkeyOk = RegisterGlobalHotkeys(a->hwnd, a->settings.hotkeys);
    if (!a->borderless && a->menu) SetMenu(a->hwnd, a->menu);
    DrawMenuBar(a->hwnd);
}

void ShowKeysHelp(AppState* a) {
    std::wstring text = HotkeyHelpText(a->settings.hotkeys);
    MessageBoxW(a->hwnd, text.c_str(), L"快捷键一览", MB_OK | MB_ICONINFORMATION);
}

LRESULT CALLBACK EditorProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState* a = g_app;
    if (!a || !a->editorPrev) return DefWindowProcW(hwnd, msg, wParam, lParam);
    if (msg == WM_MOUSEWHEEL && (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL)) {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? kAlphaStep : -kAlphaStep;
        a->settings.alpha = AdjustAlpha(a->settings.alpha, delta, a->borderless);
        ApplyWindowAlpha(a->hwnd, a->settings.alpha);
        UpdateStatus(a);
        return 0;
    }
    if (a->picking) {
        if (msg == WM_SETCURSOR) {
            SetCursor(LoadCursorW(nullptr, IDC_CROSS));
            return TRUE;
        }
        if (msg == WM_LBUTTONDOWN || msg == WM_MOUSEMOVE || msg == WM_LBUTTONUP) {
            return SendMessageW(a->hwnd, msg, wParam, lParam);
        }
    }
    return CallWindowProcW(a->editorPrev, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK SplitterProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState* a = g_app;
    if (!a || !a->splitterPrev) return DefWindowProcW(hwnd, msg, wParam, lParam);
    switch (msg) {
        case WM_SETCURSOR:
            SetCursor(LoadCursorW(nullptr, IDC_SIZEWE));
            return TRUE;
        case WM_LBUTTONDOWN:
            a->draggingSplit = true;
            a->dragStartX = GET_X_LPARAM(lParam);
            {
                POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ClientToScreen(hwnd, &pt);
                ScreenToClient(a->hwnd, &pt);
                a->dragStartX = pt.x;
            }
            a->treeWidthStart = a->settings.treeWidth;
            SetCapture(hwnd);
            return 0;
        case WM_MOUSEMOVE:
            if (a->draggingSplit && (wParam & MK_LBUTTON)) {
                POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ClientToScreen(hwnd, &pt);
                ScreenToClient(a->hwnd, &pt);
                a->settings.treeWidth = a->treeWidthStart + (pt.x - a->dragStartX);
                ClampSettings(a->settings);
                Layout(a);
            }
            return 0;
        case WM_LBUTTONUP:
            if (a->draggingSplit) {
                a->draggingSplit = false;
                ReleaseCapture();
            }
            return 0;
        default:
            break;
    }
    return CallWindowProcW(a->splitterPrev, hwnd, msg, wParam, lParam);
}

void OnCommand(AppState* a, int id) {
    switch (id) {
        case IDM_FILE_NEW: DoNew(a); break;
        case IDM_FILE_OPEN: DoOpen(a); break;
        case IDM_FILE_SAVE: DoSave(a); break;
        case IDM_FILE_SAVEAS: DoSaveAs(a); break;
        case IDM_FILE_EXIT: PostMessageW(a->hwnd, WM_CLOSE, 0, 0); break;
        case IDM_EDIT_UNDO: EditorUndo(a->editor); break;
        case IDM_EDIT_REDO: EditorRedo(a->editor); break;
        case IDM_EDIT_CUT: SendMessageW(a->editor, WM_CUT, 0, 0); break;
        case IDM_EDIT_COPY: SendMessageW(a->editor, WM_COPY, 0, 0); break;
        case IDM_EDIT_PASTE: SendMessageW(a->editor, WM_PASTE, 0, 0); break;
        case IDM_EDIT_SELECTALL: EditorSelectAll(a->editor); break;
        case IDM_EDIT_FIND: DoFind(a); break;
        case IDM_CHAPTER_REFRESH: RefreshChapters(a, true); break;
        case IDM_CHAPTER_PREV: GoChapter(a, -1); break;
        case IDM_CHAPTER_NEXT: GoChapter(a, 1); break;
        case IDM_CHAPTER_INSERT: InsertChapter(a); break;
        case IDM_VIEW_BORDER: SetBorderless(a, !a->borderless); break;
        case IDM_VIEW_HIDE: {
            if (!a->hotkeyOk) {
                std::wstring chord = L"（未设置）";
                int hi = (int)KeyId::ViewHide;
                if (hi < (int)a->settings.hotkeys.size()) chord = FormatChord(a->settings.hotkeys[hi]);
                std::wstring msg = L"全局热键 " + chord + L" 注册失败。隐藏后可能无法用热键唤回，请在「设置 → 快捷键」里换一个未被占用的组合。";
                MessageBoxW(a->hwnd, msg.c_str(), kAppTitle, MB_ICONWARNING);
            }
            ToggleHide(a);
            break;
        }
        case IDM_VIEW_FONT:
            if (ChooseBodyFont(a->hwnd, a->settings)) ApplyAllAppearance(a);
            break;
        case IDM_VIEW_FONT_INC:
            a->ignoreChange = true;
            EditorZoomPoints(a->editor, a->settings, 1);
            a->ignoreChange = false;
            break;
        case IDM_VIEW_FONT_DEC:
            a->ignoreChange = true;
            EditorZoomPoints(a->editor, a->settings, -1);
            a->ignoreChange = false;
            break;
        case IDM_VIEW_COLOR:
            if (ChooseBgColor(a->hwnd, a->settings)) ApplyAllAppearance(a);
            break;
        case IDM_VIEW_PICKER: StartPicker(a); break;
        case IDM_VIEW_ALPHA:
            MessageBoxW(a->hwnd,
                        L"写作时按住 Ctrl 并滚动鼠标滚轮，即可调节窗口透明度。\n"
                        L"有边框时最低约 40%，无边框时可更低。\n"
                        L"吸管取色会实时同步为编辑器背景色。",
                        L"透明度", MB_OK | MB_ICONINFORMATION);
            break;
        case IDM_SET_HOTKEYS:
            if (EditHotkeysDialog(a->hwnd, a->settings.hotkeys)) {
                ApplyHotkeys(a);
                SaveSettings(a->settings);
            }
            break;
        case IDM_HELP_KEYS: ShowKeysHelp(a); break;
        default: break;
    }
}

bool OnCreate(AppState* a, HWND hwnd) {
    a->hwnd = hwnd;
    a->menu = LoadMenuW(a->inst, MAKEINTRESOURCEW(IDR_MAINMENU));
    SetMenu(hwnd, a->menu);
    ApplyHotkeys(a);
    a->findMsg = RegisterWindowMessageW(FINDMSGSTRING);

    a->tree = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
                              WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_FULLROWSELECT | WS_VSCROLL,
                              0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_CHAPTER_TREE, a->inst, nullptr);
    a->splitter = CreateWindowExW(0, L"STATIC", L"",
                                  WS_CHILD | WS_VISIBLE | SS_NOTIFY | SS_ETCHEDVERT,
                                  0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_SPLITTER, a->inst, nullptr);
    a->editor = EditorCreate(hwnd, a->inst, IDC_EDITOR);
    a->status = CreateWindowExW(0, STATUSCLASSNAMEW, L"",
                                WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)IDC_STATUS, a->inst, nullptr);
    if (!a->tree || !a->editor || !a->status || !a->splitter) return false;

    a->editorPrev = EditorSubclass(a->editor, EditorProc);
    a->splitterPrev = (WNDPROC)SetWindowLongPtrW(a->splitter, GWLP_WNDPROC, (LONG_PTR)SplitterProc);

    DragAcceptFiles(hwnd, TRUE);
    a->ignoreChange = true;
    ApplyAllAppearance(a);
    a->ignoreChange = false;
    a->dirty = false;
    SendMessageW(a->editor, EM_SETMODIFY, FALSE, 0);
    SetTimer(hwnd, IDT_AUTOSAVE, kAutosaveMs, nullptr);

    if (!a->pendingOpen.empty() && PathFileExistsW(a->pendingOpen.c_str())) {
        LoadPath(a, a->pendingOpen);
    } else if (!a->settings.lastFile.empty() && PathFileExistsW(a->settings.lastFile.c_str())) {
        LoadPath(a, a->settings.lastFile);
    } else {
        RefreshChapters(a, false);
        UpdateCaption(a);
    }
    SetFocus(a->editor);
    return true;
}

}  // namespace

LRESULT CALLBACK AppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState* a = (AppState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (msg == WM_NCCREATE) {
        auto* cs = (CREATESTRUCTW*)lParam;
        a = (AppState*)cs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)a);
        a->hwnd = hwnd;
    }
    if (a && a->findMsg && msg == a->findMsg) {
        auto* fr = (FINDREPLACEW*)lParam;
        if (fr->Flags & FR_DIALOGTERM) {
            a->findDlg = nullptr;
            return 0;
        }
        if (fr->Flags & FR_FINDNEXT) {
            bool down = (fr->Flags & FR_DOWN) != 0;
            if (!EditorFind(a->editor, fr->lpstrFindWhat, down)) {
                MessageBoxW(hwnd, L"找不到指定内容。", kAppTitle, MB_OK | MB_ICONINFORMATION);
            }
        }
        return 0;
    }

    switch (msg) {
        case WM_CREATE:
            if (!OnCreate(a, hwnd)) return -1;
            return 0;
        case WM_ERASEBKGND:
            if (a && a->borderless) {
                RECT rc{};
                GetClientRect(hwnd, &rc);
                HBRUSH br = CreateSolidBrush(a->settings.bgColor);
                FillRect((HDC)wParam, &rc, br);
                DeleteObject(br);
                return 1;
            }
            break;
        case WM_SIZE:
            if (a) Layout(a);
            return 0;
        case WM_GETMINMAXINFO: {
            auto* m = (MINMAXINFO*)lParam;
            m->ptMinTrackSize.x = 480;
            m->ptMinTrackSize.y = 320;
            return 0;
        }
        case WM_COMMAND:
            if (a && lParam == 0) OnCommand(a, LOWORD(wParam));
            else if (a && HIWORD(wParam) == EN_CHANGE && (HWND)lParam == a->editor) {
                if (!a->ignoreChange) {
                    a->dirty = true;
                    UpdateCaption(a);
                    SetTimer(hwnd, IDT_CHAPTER_REFRESH, kChapterDebounceMs, nullptr);
                }
            }
            return 0;
        case WM_NOTIFY: {
            auto* nmh = (NMHDR*)lParam;
            if (a && nmh->idFrom == IDC_CHAPTER_TREE && nmh->code == TVN_SELCHANGEDW) {
                auto* nmtv = (NMTREEVIEWW*)lParam;
                if (!a->updatingTree) JumpToChapter(a, (int)nmtv->itemNew.lParam);
            }
            if (a && nmh->idFrom == IDC_EDITOR && nmh->code == EN_SELCHANGE) {
                SetTimer(hwnd, IDT_CHAPTER_REFRESH, kChapterDebounceMs, nullptr);
            }
            return 0;
        }
        case WM_TIMER:
            if (!a) return 0;
            if (wParam == IDT_CHAPTER_REFRESH) {
                KillTimer(hwnd, IDT_CHAPTER_REFRESH);
                RefreshChapters(a, true);
            } else if (wParam == IDT_AUTOSAVE) {
                if (a->dirty && !a->filePath.empty()) DoSave(a);
            }
            return 0;
        case WM_MOUSEWHEEL:
            if (a && (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL)) {
                int delta = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? kAlphaStep : -kAlphaStep;
                a->settings.alpha = AdjustAlpha(a->settings.alpha, delta, a->borderless);
                ApplyWindowAlpha(hwnd, a->settings.alpha);
                UpdateStatus(a);
                return 0;
            }
            break;
        case WM_SETCURSOR:
            if (a && a->picking) {
                SetCursor(LoadCursorW(nullptr, IDC_CROSS));
                return TRUE;
            }
            break;
        case WM_LBUTTONDOWN:
            if (a && a->picking) {
                PickerSample(a);
                return 0;
            }
            break;
        case WM_MOUSEMOVE:
            if (a && a->picking) {
                PickerSample(a);
                return 0;
            }
            break;
        case WM_LBUTTONUP:
            if (a && a->picking) {
                PickerSample(a);
                StopPicker(a);
                return 0;
            }
            break;
        case WM_CAPTURECHANGED:
            if (a && a->picking && (HWND)lParam != hwnd) StopPicker(a);
            return 0;
        case WM_KEYDOWN:
            if (a && wParam == VK_ESCAPE && a->picking) {
                StopPicker(a);
                return 0;
            }
            break;
        case WM_NCHITTEST:
            if (a && a->borderless && !a->picking) {
                LRESULT hit = DefWindowProcW(hwnd, msg, wParam, lParam);
                POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                RECT wr{};
                GetWindowRect(hwnd, &wr);
                const int m = kBorderMargin;
                bool left = pt.x < wr.left + m;
                bool right = pt.x >= wr.right - m;
                bool top = pt.y < wr.top + m;
                bool bottom = pt.y >= wr.bottom - m;
                if (top && left) return HTTOPLEFT;
                if (top && right) return HTTOPRIGHT;
                if (bottom && left) return HTBOTTOMLEFT;
                if (bottom && right) return HTBOTTOMRIGHT;
                if (left) return HTLEFT;
                if (right) return HTRIGHT;
                if (bottom) return HTBOTTOM;
                if (top) return HTCAPTION;
                return hit;
            }
            break;
        case WM_DROPFILES:
            if (a) {
                HDROP drop = (HDROP)wParam;
                wchar_t path[MAX_PATH]{};
                if (DragQueryFileW(drop, 0, path, MAX_PATH) && PromptSaveIfDirty(a)) {
                    LoadPath(a, path);
                }
                DragFinish(drop);
            }
            return 0;
        case WM_HOTKEY:
            if (a && wParam >= HOTKEY_BASE && wParam < HOTKEY_BASE + kKeyCount) {
                int i = (int)wParam - HOTKEY_BASE;
                OnCommand(a, KeyAt(i).commandId);
            }
            return 0;
        case WM_CLOSE:
            if (a) {
                if (!PromptSaveIfDirty(a)) return 0;
                PersistWindow(a);
                a->settings.lastFile = a->filePath;
                SaveSettings(a->settings);
                UnregisterGlobalHotkeys(hwnd);
            }
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int RunApp(HINSTANCE inst, int nCmdShow, const wchar_t* cmdLine) {
    AppState app;
    app.inst = inst;
    app.settings = LoadSettings();
    g_app = &app;

    if (cmdLine && cmdLine[0]) {
        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argv) {
            if (argc >= 2 && argv[1] && PathFileExistsW(argv[1])) {
                app.pendingOpen = argv[1];
            }
            LocalFree(argv);
        }
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = AppWndProc;
    wc.hInstance = inst;
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(IDI_APP));
    if (!wc.hIcon) wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassName;
    wc.hIconSm = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (!wc.hIconSm) wc.hIconSm = wc.hIcon;
    if (!RegisterClassExW(&wc)) return 1;

    DWORD style = WS_OVERLAPPEDWINDOW;
    int x = app.settings.windowX;
    int y = app.settings.windowY;
    int w = app.settings.windowW;
    int h = app.settings.windowH;
    if (x == (int)CW_USEDEFAULT) {
        x = CW_USEDEFAULT;
        y = CW_USEDEFAULT;
    }

    HWND hwnd = CreateWindowExW(WS_EX_LAYERED, kClassName, kAppTitle, style,
                                x, y, w, h, nullptr, nullptr, inst, &app);
    if (!hwnd) return 1;

    ApplyWindowAlpha(hwnd, app.settings.alpha);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (app.findDlg && IsDialogMessageW(app.findDlg, &msg)) continue;
        if (app.accel && TranslateAcceleratorW(hwnd, app.accel, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    g_app = nullptr;
    return (int)msg.wParam;
}
