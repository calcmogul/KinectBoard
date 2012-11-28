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

#include <SFML/Graphics/RenderWindow.hpp>
#include "Processing.hpp"

namespace sf {

class TestScreen : public sf::RenderWindow , public Processing {
public:
    TestScreen();
    TestScreen( const char* className , HWND parentWin , HINSTANCE instance );

    virtual ~TestScreen();

    void setColor( Processing::ProcColor borderColor );

    // Displays test pattern with previously set border color
    void display();

    // Destroys HWND and closes RenderWindow
    void close();

private:
    HWND m_window;
    HCURSOR m_cursor;
    ProcColor m_borderColor;
};

}

#endif // TEST_SCREEN_HPP
