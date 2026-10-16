#include "Settings.h"

#include <windows.h>
#include <shlwapi.h>
#include <sstream>
#include <cctype>

#pragma comment(lib, "shlwapi.lib")

namespace {

std::wstring Utf8ToWide(const std::string& u8) {
    if (u8.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, u8.data(), (int)u8.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, u8.data(), (int)u8.size(), w.data(), n);
    return w;
}

std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string u8(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), u8.data(), n, nullptr, nullptr);
    return u8;
}

std::wstring EscapeJson(const std::wstring& s) {
    std::wstring o;
    o.reserve(s.size() + 8);
    for (wchar_t c : s) {
        if (c == L'\\' || c == L'"') {
            o += L'\\';
            o += c;
        } else if (c == L'\n') {
            o += L"\\n";
        } else if (c == L'\r') {
            o += L"\\r";
        } else if (c == L'\t') {
            o += L"\\t";
        } else {
            o += c;
        }
    }
    return o;
}

std::wstring UnescapeJson(const std::wstring& s) {
    std::wstring o;
    o.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == L'\\' && i + 1 < s.size()) {
            wchar_t n = s[++i];
            if (n == L'n') o += L'\n';
            else if (n == L'r') o += L'\r';
            else if (n == L't') o += L'\t';
            else o += n;
        } else {
            o += s[i];
        }
    }
    return o;
}

bool ExtractString(const std::wstring& json, const wchar_t* key, std::wstring& out) {
    std::wstring pat = std::wstring(L"\"") + key + L"\"";
    size_t p = json.find(pat);
    if (p == std::wstring::npos) return false;
    p = json.find(L':', p + pat.size());
    if (p == std::wstring::npos) return false;
    p = json.find(L'"', p);
    if (p == std::wstring::npos) return false;
    ++p;
    std::wstring raw;
    for (; p < json.size(); ++p) {
        if (json[p] == L'\\' && p + 1 < json.size()) {
            raw += json[p];
            raw += json[++p];
            continue;
        }
        if (json[p] == L'"') break;
        raw += json[p];
    }
    out = UnescapeJson(raw);
    return true;
}

bool ExtractInt(const std::wstring& json, const wchar_t* key, int& out) {
    std::wstring pat = std::wstring(L"\"") + key + L"\"";
    size_t p = json.find(pat);
    if (p == std::wstring::npos) return false;
    p = json.find(L':', p + pat.size());
    if (p == std::wstring::npos) return false;
    ++p;
    while (p < json.size() && iswspace(json[p])) ++p;
    if (p >= json.size()) return false;
    bool neg = false;
    if (json[p] == L'-') {
        neg = true;
        ++p;
    }
    if (p >= json.size() || !iswdigit(json[p])) return false;
    long v = 0;
    while (p < json.size() && iswdigit(json[p])) {
        v = v * 10 + (json[p] - L'0');
        ++p;
    }
    out = neg ? -(int)v : (int)v;
    return true;
}

bool WriteUtf8File(const std::wstring& path, const std::string& u8, bool bom) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD wr = 0;
    BOOL ok = TRUE;
    if (bom) {
        const unsigned char b[] = {0xEF, 0xBB, 0xBF};
        ok = WriteFile(h, b, 3, &wr, nullptr);
    }
    if (ok && !u8.empty()) ok = WriteFile(h, u8.data(), (DWORD)u8.size(), &wr, nullptr);
    CloseHandle(h);
    return ok != FALSE;
}

bool ReadUtf8File(const std::wstring& path, std::string& u8) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz{};
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart < 0 || sz.QuadPart > 2 * 1024 * 1024) {
        CloseHandle(h);
        return false;
    }
    u8.resize((size_t)sz.QuadPart);
    DWORD got = 0;
    BOOL ok = ReadFile(h, u8.data(), (DWORD)u8.size(), &got, nullptr);
    CloseHandle(h);
    if (!ok) return false;
    u8.resize(got);
    return true;
}

}  // namespace

void ClampSettings(AppSettings& s) {
    if (s.fontName.empty()) s.fontName = L"Microsoft YaHei";
    if (s.fontPoint < 8) s.fontPoint = 8;
    if (s.fontPoint > 72) s.fontPoint = 72;
    if (s.alpha < 1) s.alpha = 1;
    if (s.alpha > 255) s.alpha = 255;
    if (s.windowW < 400) s.windowW = 400;
    if (s.windowH < 300) s.windowH = 300;
    if (s.treeWidth < 120) s.treeWidth = 120;
    if (s.treeWidth > 480) s.treeWidth = 480;
    if (s.lineSpacing < 20) s.lineSpacing = 20;
    if (s.lineSpacing > 60) s.lineSpacing = 60;
    if ((int)s.hotkeys.size() != kKeyCount) InitDefaultHotkeys(s.hotkeys);
}

std::wstring SettingsPath() {
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    PathRemoveFileSpecW(buf);
    PathAppendW(buf, L"settings.json");
    return buf;
}

AppSettings LoadSettings() {
    AppSettings s;
    InitDefaultHotkeys(s.hotkeys);
    std::string u8;
    if (!ReadUtf8File(SettingsPath(), u8)) return s;
    std::wstring json = Utf8ToWide(u8);
    if (!json.empty() && json[0] == 0xFEFF) json.erase(0, 1);

    ExtractString(json, L"fontName", s.fontName);
    ExtractInt(json, L"fontPoint", s.fontPoint);
    int color = 0;
    if (ExtractInt(json, L"fontColor", color)) s.fontColor = (COLORREF)color;
    if (ExtractInt(json, L"bgColor", color)) s.bgColor = (COLORREF)color;
    ExtractInt(json, L"alpha", s.alpha);
    ExtractInt(json, L"windowX", s.windowX);
    ExtractInt(json, L"windowY", s.windowY);
    ExtractInt(json, L"windowW", s.windowW);
    ExtractInt(json, L"windowH", s.windowH);
    ExtractInt(json, L"treeWidth", s.treeWidth);
    ExtractInt(json, L"lineSpacing", s.lineSpacing);
    ExtractString(json, L"lastFile", s.lastFile);
    ParseHotkeysJson(json, s.hotkeys);
    ClampSettings(s);
    return s;
}

bool SaveSettings(const AppSettings& s) {
    AppSettings c = s;
    ClampSettings(c);
    std::wostringstream o;
    o << L"{\n";
    o << L"  \"fontName\": \"" << EscapeJson(c.fontName) << L"\",\n";
    o << L"  \"fontPoint\": " << c.fontPoint << L",\n";
    o << L"  \"fontColor\": " << (int)c.fontColor << L",\n";
    o << L"  \"bgColor\": " << (int)c.bgColor << L",\n";
    o << L"  \"alpha\": " << c.alpha << L",\n";
    o << L"  \"windowX\": " << c.windowX << L",\n";
    o << L"  \"windowY\": " << c.windowY << L",\n";
    o << L"  \"windowW\": " << c.windowW << L",\n";
    o << L"  \"windowH\": " << c.windowH << L",\n";
    o << L"  \"treeWidth\": " << c.treeWidth << L",\n";
    o << L"  \"lineSpacing\": " << c.lineSpacing << L",\n";
    o << L"  \"lastFile\": \"" << EscapeJson(c.lastFile) << L"\",\n";
    std::wstring hk;
    AppendHotkeysJson(hk, c.hotkeys);
    o << hk << L"\n";
    o << L"}\n";
    return WriteUtf8File(SettingsPath(), WideToUtf8(o.str()), true);
}
