#include "ChapterIndex.h"
#include "FileIo.h"

#include <windows.h>
#include <cassert>
#include <cstdio>
#include <vector>

#ifdef NDEBUG
#error These tests require assertions enabled.
#endif

int wmain(int argc, wchar_t** argv) {
    assert(argc == 2);  // Caller supplies a writable, isolated test directory.
    const std::wstring path = std::wstring(argv[1]) + L"\\core-" +
                              std::to_wstring(GetCurrentProcessId()) + L".txt";
    struct Cleanup {
        std::wstring path;
        ~Cleanup() { DeleteFileW(path.c_str()); }
    } cleanup{path};
    auto writeBytes = [&](const std::vector<unsigned char>& bytes) {
        HANDLE f = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        assert(f != INVALID_HANDLE_VALUE);
        DWORD written = 0;
        const BOOL ok = WriteFile(f, bytes.data(), (DWORD)bytes.size(), &written, nullptr);
        CloseHandle(f);
        assert(ok && written == bytes.size());
    };

    const std::wstring text = L"序章 起点\r第一章 清晨\r正文\r第2章 远行";
    const auto chapters = ParseChapters(text);
    assert(chapters.size() == 3);
    assert(chapters[0].title == L"序章 起点");
    assert(ChapterIndexAt(chapters, chapters[1].start) == 1);
    assert(ChapterLength(chapters, 1, (int)text.size()) ==
           chapters[2].start - chapters[1].start);
    assert(ChapterIndexAt({}, 0) == -1);
    assert(ParseChapters(L"普通正文\r\n没有章节").empty());
    assert(ParseChapters(L"第一卷\n第一节\n楔子").size() == 3);
    assert(CountNonSpaceChars(L"中 文\tA\r\n　", 0, 8) == 3);
    assert(NextChapterTitle(chapters) == L"第4章 ");
    assert(NormalizeNewlinesToCr(L"a\r\nb\nc\r") == L"a\rb\rc\r");
    assert(CrToCrlf(L"a\rb\nc\r\n") == L"a\r\nb\r\nc\r\n");

    assert(SaveTextFileUtf8(path, L"中文\r正文"));
    std::wstring loaded;
    TextEncoding encoding;
    assert(LoadTextFile(path, loaded, &encoding));
    assert(loaded == L"中文\r正文" && encoding == TextEncoding::Utf8);
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    assert(f != INVALID_HANDLE_VALUE);
    unsigned char bom[3] = {};
    DWORD got = 0;
    const BOOL ok = ReadFile(f, bom, 3, &got, nullptr);
    CloseHandle(f);
    assert(ok && got == 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF);

    writeBytes({0xFF, 0xFE, 0x2D, 0x4E});
    assert(LoadTextFile(path, loaded, &encoding));
    assert(loaded == L"中" && encoding == TextEncoding::Utf16LE);
    writeBytes({0xFE, 0xFF, 0x4E, 0x2D});
    assert(LoadTextFile(path, loaded, &encoding));
    assert(loaded == L"中" && encoding == TextEncoding::Utf16BE);
    writeBytes({0xD6, 0xD0, 0xCE, 0xC4});
    assert(LoadTextFile(path, loaded, &encoding));
    assert(loaded == L"中文" && encoding == TextEncoding::Gbk);
    writeBytes({0xE4, 0xB8, 0xAD});
    assert(LoadTextFile(path, loaded, &encoding));
    assert(loaded == L"中" && encoding == TextEncoding::Utf8);
    std::puts("Core tests passed: chapters, counts, newlines, UTF-8/UTF-16/GBK I/O.");
    return 0;
}
