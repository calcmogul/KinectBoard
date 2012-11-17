//=============================================================================
//File Name: TestScreen.hpp
//Description: Displays a fullscreen test pattern of a given color
//Author: Tyler Veness
//=============================================================================

#ifndef TEST_SCREEN_HPP
#define TEST_SCREEN_HPP

#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <SFML/Graphics/RenderWindow.hpp>

namespace sf {

class TestScreen : public sf::RenderWindow {
public:
    enum ProcColor {
        Red = 0,
        Green = 1,
        Blue = 2
    };

    TestScreen();
    TestScreen( const char* className , HWND parentWin , HINSTANCE instance );

    virtual ~TestScreen();

    void setColor( ProcColor borderColor );

    // Displays test pattern with previously set border color
    void display();

    // Destroys HWND and closes RenderWindow
    void close();

private:
    HWND m_window;
    ProcColor m_borderColor;
};

}

#endif // TEST_SCREEN_HPP
