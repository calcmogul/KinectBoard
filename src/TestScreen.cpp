//=============================================================================
//File Name: TestScreen.cpp
//Description: Displays a fullscreen test pattern of a given color
//Author: Tyler Veness
//=============================================================================

#include "TestScreen.hpp"
#include "ImageVars.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace sf {

TestScreen::TestScreen() : RenderWindow() , m_window( NULL ) , m_cursor( NULL ) , m_borderColor( Red ) {

}

TestScreen::TestScreen( const char* className , HWND parentWin , HINSTANCE instance ) : RenderWindow() , m_cursor( NULL ) , m_borderColor( Red ) {
    m_window = CreateWindowEx( 0 ,
            className ,
            "" ,
            WS_POPUP | WS_VISIBLE ,
            0 ,
            0 ,
            GetSystemMetrics(SM_CXSCREEN) ,
            GetSystemMetrics(SM_CYSCREEN) ,
            parentWin ,
            NULL ,
            instance ,
            NULL );

    create( m_window );

    // Prevent mouse cursor covering up pattern by making it invisible
    SetCursor( NULL );
}

TestScreen::~TestScreen() {
    TestScreen::close();
}

void TestScreen::setColor( ProcColor borderColor ) {
    m_borderColor = borderColor;
}

void TestScreen::display() {
    sf::Color outlineColor( 0 , 0 , 0 );

    if ( m_borderColor == Red ) {
        outlineColor.r = 255;
    }
    else if ( m_borderColor == Green ) {
        outlineColor.g = 255;
    }
    else if ( m_borderColor == Blue ) {
        outlineColor.b = 255;
    }

    /* ===== Create and draw the calibration graphic ===== */
    // Create border for window
    static sf::RectangleShape border( sf::Vector2f( getSize().x - 40.f , getSize().y - 40.f ) );
    border.setFillColor( sf::Color( 0 , 0 , 0 , 0 ) );
    border.setOutlineThickness( 20 );
    border.setOutlineColor( outlineColor );

    // Set graphics positions before drawing them
    border.setPosition( 20.f , 20.f );

    // Make window visible before drawing to it
    ShowWindow( m_window , SW_SHOWNORMAL );

    // Draw graphics to window
    clear( sf::Color( 0 , 0 , 0 ) );
    draw( border );
    RenderWindow::display();
}

void TestScreen::close() {
    DestroyWindow( m_window );
    RenderWindow::close();
}

}
