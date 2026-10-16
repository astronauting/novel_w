#include "Appearance.h"

#include <commdlg.h>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "msimg32.lib")

void ApplyWindowAlpha(HWND hwnd, int alpha) {
    if (alpha < 1) alpha = 1;
    if (alpha > 255) alpha = 255;
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if ((ex & WS_EX_LAYERED) == 0) {
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex | WS_EX_LAYERED);
    }
    SetLayeredWindowAttributes(hwnd, 0, (BYTE)alpha, LWA_ALPHA);
}

int AdjustAlpha(int alpha, int delta, bool borderless) {
    int minA = borderless ? kMinAlphaBorderless : kMinAlphaFramed;
    int v = alpha + delta;
    if (v < minA) v = minA;
    if (v > 255) v = 255;
    return v;
}

bool ChooseBodyFont(HWND owner, AppSettings& s) {
    LOGFONTW lf{};
    lstrcpynW(lf.lfFaceName, s.fontName.c_str(), LF_FACESIZE);
    HDC hdc = GetDC(owner);
    lf.lfHeight = -MulDiv(s.fontPoint, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(owner, hdc);
    lf.lfCharSet = DEFAULT_CHARSET;

    CHOOSEFONTW cf{};
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = owner;
    cf.lpLogFont = &lf;
    cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS;
    cf.rgbColors = s.fontColor;
    cf.iPointSize = s.fontPoint * 10;
    if (!ChooseFontW(&cf)) return false;
    s.fontName = lf.lfFaceName;
    s.fontPoint = (cf.iPointSize + 5) / 10;
    if (s.fontPoint < 8) s.fontPoint = 8;
    s.fontColor = cf.rgbColors;
    return true;
}

bool ChooseBgColor(HWND owner, AppSettings& s) {
    static COLORREF custom[16] = {};
    CHOOSECOLORW cc{};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = owner;
    cc.lpCustColors = custom;
    cc.rgbResult = s.bgColor;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (!ChooseColorW(&cc)) return false;
    s.bgColor = cc.rgbResult;
    return true;
}

COLORREF SampleScreenColor(POINT screenPt) {
    HDC hdc = GetDC(nullptr);
    COLORREF c = GetPixel(hdc, screenPt.x, screenPt.y);
    ReleaseDC(nullptr, hdc);
    return c;
}
