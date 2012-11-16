//=============================================================================
//File Name: TestScreen.hpp
//Description: Displays a fullscreen test pattern of a given color
//Author: Tyler Veness
//=============================================================================

#ifndef TEST_SCREEN_HPP
#define TEST_SCREEN_HPP

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
    virtual ~TestScreen();

    void setColor( ProcColor borderColor );

    void display();

private:
    ProcColor m_borderColor;
};

}

#endif // TEST_SCREEN_HPP
