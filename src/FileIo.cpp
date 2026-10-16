#include "FileIo.h"

#include <windows.h>
#include <vector>

const wchar_t* EncodingName(TextEncoding enc) {
    switch (enc) {
        case TextEncoding::Utf8: return L"UTF-8";
        case TextEncoding::Utf16LE: return L"UTF-16 LE";
        case TextEncoding::Utf16BE: return L"UTF-16 BE";
        case TextEncoding::Gbk: return L"GBK";
        default: return L"?";
    }
}

static bool LooksLikeUtf8(const unsigned char* p, size_t n) {
    size_t i = 0;
    while (i < n) {
        if (p[i] <= 0x7F) {
            ++i;
            continue;
        }
        int need = 0;
        if ((p[i] & 0xE0) == 0xC0) need = 1;
        else if ((p[i] & 0xF0) == 0xE0) need = 2;
        else if ((p[i] & 0xF8) == 0xF0) need = 3;
        else return false;
        if (i + need >= n) return false;
        for (int k = 1; k <= need; ++k) {
            if ((p[i + k] & 0xC0) != 0x80) return false;
        }
        i += (size_t)need + 1;
    }
    return true;
}

static std::wstring MultiToWide(UINT cp, const char* data, int len) {
    if (len <= 0) return {};
    int n = MultiByteToWideChar(cp, 0, data, len, nullptr, 0);
    if (n <= 0) return {};
    std::wstring w(n, 0);
    MultiByteToWideChar(cp, 0, data, len, w.data(), n);
    return w;
}

static bool ReadAllBytes(const std::wstring& path, std::vector<unsigned char>& buf) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz{};
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart < 0 || sz.QuadPart > 256ll * 1024 * 1024) {
        CloseHandle(h);
        return false;
    }
    buf.resize((size_t)sz.QuadPart);
    BOOL ok = TRUE;
    size_t off = 0;
    while (off < buf.size()) {
        DWORD chunk = 0;
        size_t remain = buf.size() - off;
        DWORD ask = (DWORD)((remain > 32u * 1024u * 1024u) ? (32u * 1024u * 1024u) : remain);
        if (!ReadFile(h, buf.data() + off, ask, &chunk, nullptr)) {
            ok = FALSE;
            break;
        }
        if (chunk == 0) break;
        off += chunk;
    }
    CloseHandle(h);
    if (!ok) return false;
    buf.resize(off);
    return true;
}

std::wstring NormalizeNewlinesToCr(const std::wstring& text) {
    std::wstring o;
    o.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == L'\r') {
            o += L'\r';
            if (i + 1 < text.size() && text[i + 1] == L'\n') ++i;
        } else if (text[i] == L'\n') {
            o += L'\r';
        } else {
            o += text[i];
        }
    }
    return o;
}

std::wstring CrToCrlf(const std::wstring& text) {
    std::wstring o;
    o.reserve(text.size() + text.size() / 8);
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == L'\r') {
            o += L"\r\n";
            if (i + 1 < text.size() && text[i + 1] == L'\n') ++i;
        } else if (text[i] == L'\n') {
            o += L"\r\n";
        } else {
            o += text[i];
        }
    }
    return o;
}

bool LoadTextFile(const std::wstring& path, std::wstring& text, TextEncoding* outEnc) {
    std::vector<unsigned char> buf;
    if (!ReadAllBytes(path, buf)) return false;

    TextEncoding enc = TextEncoding::Utf8;
    if (buf.size() >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) {
        enc = TextEncoding::Utf8;
        text = MultiToWide(CP_UTF8, reinterpret_cast<char*>(buf.data() + 3), (int)(buf.size() - 3));
    } else if (buf.size() >= 2 && buf[0] == 0xFF && buf[1] == 0xFE) {
        enc = TextEncoding::Utf16LE;
        size_t n = (buf.size() - 2) / 2;
        text.assign(reinterpret_cast<const wchar_t*>(buf.data() + 2), n);
    } else if (buf.size() >= 2 && buf[0] == 0xFE && buf[1] == 0xFF) {
        enc = TextEncoding::Utf16BE;
        size_t n = (buf.size() - 2) / 2;
        text.resize(n);
        for (size_t i = 0; i < n; ++i) {
            unsigned char hi = buf[2 + i * 2];
            unsigned char lo = buf[2 + i * 2 + 1];
            text[i] = (wchar_t)((hi << 8) | lo);
        }
    } else if (!buf.empty() && LooksLikeUtf8(buf.data(), buf.size())) {
        enc = TextEncoding::Utf8;
        text = MultiToWide(CP_UTF8, reinterpret_cast<char*>(buf.data()), (int)buf.size());
    } else {
        enc = TextEncoding::Gbk;
        text = MultiToWide(936, reinterpret_cast<char*>(buf.data()), (int)buf.size());
    }

    if (outEnc) *outEnc = enc;
    text = NormalizeNewlinesToCr(text);
    return true;
}

bool SaveTextFileUtf8(const std::wstring& path, const std::wstring& text) {
    std::wstring win = CrToCrlf(text);
    int n = WideCharToMultiByte(CP_UTF8, 0, win.data(), (int)win.size(), nullptr, 0, nullptr, nullptr);
    std::string u8;
    if (n > 0) {
        u8.resize((size_t)n);
        WideCharToMultiByte(CP_UTF8, 0, win.data(), (int)win.size(), u8.data(), n, nullptr, nullptr);
    }
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD wr = 0;
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    BOOL ok = WriteFile(h, bom, 3, &wr, nullptr);
    if (ok && !u8.empty()) ok = WriteFile(h, u8.data(), (DWORD)u8.size(), &wr, nullptr);
    CloseHandle(h);
    return ok != FALSE;
}
