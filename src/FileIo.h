#pragma once

#include <string>

enum class TextEncoding {
    Utf8,
    Utf16LE,
    Utf16BE,
    Gbk
};

const wchar_t* EncodingName(TextEncoding enc);
bool LoadTextFile(const std::wstring& path, std::wstring& text, TextEncoding* outEnc);
bool SaveTextFileUtf8(const std::wstring& path, const std::wstring& text);
std::wstring NormalizeNewlinesToCr(const std::wstring& text);
std::wstring CrToCrlf(const std::wstring& text);
