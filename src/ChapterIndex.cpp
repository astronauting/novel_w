#include "ChapterIndex.h"

#include <windows.h>
#include <cwctype>

namespace {

bool IsChapterNumberChar(wchar_t c) {
    if (c >= L'0' && c <= L'9') return true;
    switch (c) {
        case L'零': case L'〇': case L'一': case L'二': case L'三': case L'四':
        case L'五': case L'六': case L'七': case L'八': case L'九': case L'十':
        case L'百': case L'千': case L'万': case L'两':
        case L'壹': case L'贰': case L'叁': case L'肆': case L'伍':
        case L'陆': case L'柒': case L'捌': case L'玖': case L'拾':
        case L'佰': case L'仟':
            return true;
        default:
            return false;
    }
}

bool IsChapterEnd(wchar_t c) {
    return c == L'章' || c == L'卷' || c == L'部' || c == L'节';
}

bool LineLooksLikeChapter(const wchar_t* line, int len, int* titleFrom) {
    int i = 0;
    while (i < len && (line[i] == L' ' || line[i] == L'\t' || line[i] == 0x3000)) ++i;
    if (i >= len) return false;

    if (i + 1 < len && line[i] == L'楔' && line[i + 1] == L'子') {
        *titleFrom = i;
        return true;
    }
    if (i + 1 < len && line[i] == L'序' && line[i + 1] == L'章') {
        *titleFrom = i;
        return true;
    }

    for (int p = i; p < len; ++p) {
        if (line[p] != L'第') continue;
        int q = p + 1;
        if (q >= len || !IsChapterNumberChar(line[q])) continue;
        while (q < len && IsChapterNumberChar(line[q])) ++q;
        if (q < len && IsChapterEnd(line[q])) {
            *titleFrom = p;
            return true;
        }
    }
    return false;
}

}  // namespace

std::vector<Chapter> ParseChapters(const std::wstring& text) {
    std::vector<Chapter> out;
    const wchar_t* p = text.c_str();
    const int n = (int)text.size();
    int i = 0;
    while (i <= n) {
        int lineStart = i;
        while (i < n && p[i] != L'\r' && p[i] != L'\n') ++i;
        int lineLen = i - lineStart;
        int titleFrom = 0;
        if (lineLen > 0 && LineLooksLikeChapter(p + lineStart, lineLen, &titleFrom)) {
            Chapter ch;
            ch.start = lineStart;
            int tlen = lineLen - titleFrom;
            if (tlen > 80) tlen = 80;
            ch.title.assign(p + lineStart + titleFrom, tlen);
            while (!ch.title.empty() && iswspace(ch.title.back())) ch.title.pop_back();
            out.push_back(std::move(ch));
        }
        if (i < n && p[i] == L'\r') ++i;
        if (i < n && p[i] == L'\n') ++i;
        if (i == lineStart) break;
    }
    return out;
}

int ChapterIndexAt(const std::vector<Chapter>& chapters, int cp) {
    if (chapters.empty()) return -1;
    int found = 0;
    for (int i = 0; i < (int)chapters.size(); ++i) {
        if (chapters[i].start <= cp) found = i;
        else break;
    }
    return found;
}

int ChapterLength(const std::vector<Chapter>& chapters, int index, int totalChars) {
    if (index < 0 || index >= (int)chapters.size()) {
        return totalChars;
    }
    int start = chapters[index].start;
    int end = (index + 1 < (int)chapters.size()) ? chapters[index + 1].start : totalChars;
    if (end < start) return 0;
    return end - start;
}

int CountNonSpaceChars(const std::wstring& text, int start, int end) {
    if (start < 0) start = 0;
    if (end > (int)text.size()) end = (int)text.size();
    if (end < start) return 0;
    int n = 0;
    for (int i = start; i < end; ++i) {
        wchar_t c = text[i];
        if (c == L' ' || c == L'\t' || c == L'\r' || c == L'\n' || c == 0x3000) continue;
        ++n;
    }
    return n;
}

std::wstring NextChapterTitle(const std::vector<Chapter>& chapters) {
    int n = (int)chapters.size() + 1;
    wchar_t buf[64];
    wsprintfW(buf, L"第%d章 ", n);
    return buf;
}
