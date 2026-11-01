#ifdef GEODE_IS_WINDOWS
#include <Windows.h>
#include <dwmapi.h>
#include <Geode/modify/CCEGLView.hpp>
class $modify(cocos2d::CCEGLView) {
    void setupWindow(cocos2d::CCRect rect) {
        cocos2d::CCEGLView::setupWindow(rect);

        if (HDC hdc = wglGetCurrentDC()) {
            if (HWND hwnd = WindowFromDC(hdc)) {
                SetWindowTextW(hwnd, L"RusDash GDPS");

                BOOL dark = TRUE;
                DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
            }
        }

        if (HWND console = GetConsoleWindow()) {
            SetWindowTextW(console, L"RusDash Console");
        }
    }
};
#endif