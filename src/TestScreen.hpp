//=============================================================================
//File Name: TestScreen.hpp
//Description: Displays a fullscreen test pattern of a given color
//Author: Tyler Veness
//=============================================================================

#ifndef TEST_SCREEN_HPP
#define TEST_SCREEN_HPP

#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Processing.hpp"
#include "Color.hpp"


class TestScreen : public Processing {
public:
    TestScreen(HINSTANCE instance = nullptr, bool createNow = false);

    virtual ~TestScreen();

    // Create HWND
    void create(const RECT windowPos = {0, 0, GetSystemMetrics(SM_CXSCREEN),
                                        GetSystemMetrics(SM_CYSCREEN)});

    void setPosition(const RECT windowPos);

    void setColor(Processing::ProcColor borderColor);

    // Displays test pattern with previously set border color
    void display();

    // Destroy HWND
    void close();

    static bool unregisterClass();

private:
    static const char* m_windowClassName;

    static WNDCLASSEX m_windowClass;
    static HINSTANCE m_instance;
    static HBRUSH m_whiteBrush;
    static HBRUSH m_colorBrush;
    static bool m_classInitialized;

    static LRESULT CALLBACK OnEvent(HWND, UINT, WPARAM, LPARAM);

    void eventLoop();

    HWND m_window = nullptr;
    HCURSOR m_prevCursor = nullptr;

    Color m_outlineColor{255, 0, 0};
    ProcColor m_borderColor = Red;
};

#endif // TEST_SCREEN_HPP
