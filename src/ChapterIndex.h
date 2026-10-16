#pragma once

#include <string>
#include <vector>

struct Chapter {
    int start = 0;
    std::wstring title;
};

std::vector<Chapter> ParseChapters(const std::wstring& text);
int ChapterIndexAt(const std::vector<Chapter>& chapters, int cp);
int ChapterLength(const std::vector<Chapter>& chapters, int index, int totalChars);
int CountNonSpaceChars(const std::wstring& text, int start, int end);
std::wstring NextChapterTitle(const std::vector<Chapter>& chapters);
