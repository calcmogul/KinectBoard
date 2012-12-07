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

typedef struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Color;

class TestScreen : public Processing {
public:
    TestScreen( HINSTANCE instance = NULL , bool createNow = false );

    virtual ~TestScreen();

    // Create HWND
    void create();

    void setColor( Processing::ProcColor borderColor );

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

    static LRESULT CALLBACK OnEvent( HWND , UINT , WPARAM , LPARAM );

    void eventLoop();

    HWND m_window;
    HCURSOR m_prevCursor;

    Color m_outlineColor;
    ProcColor m_borderColor;
};

#endif // TEST_SCREEN_HPP
