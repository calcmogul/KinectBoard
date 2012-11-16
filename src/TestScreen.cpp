//=============================================================================
//File Name: TestScreen.cpp
//Description: Displays a fullscreen test pattern of a given color
//Author: Tyler Veness
//=============================================================================

#include "TestScreen.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace sf {

TestScreen::TestScreen() : RenderWindow( sf::VideoMode( GetSystemMetrics(SM_CXSCREEN) , GetSystemMetrics(SM_CYSCREEN) ) , "" , sf::Style::None | sf::Style::Fullscreen ) , m_borderColor( Red ) {

}

TestScreen::~TestScreen() {

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

    // keep mouse from covering up pattern
    setMouseCursorVisible( false );

    /* ===== Create and draw the calibration graphic ===== */
    // Construct pixel data
    static sf::Image testImage;
    testImage.create( 40 , 40 , sf::Color( 0 , 0 , 0 ) );

    // Change top-left block to white
    for ( unsigned int yPos = 0 ; yPos < 20 ; yPos++ ) {
        for ( unsigned int xPos = 20 ; xPos < 40 ; xPos++ )
            testImage.setPixel( xPos , yPos , sf::Color( 255 , 255 , 255 ) );
    }

    // Change bottom-right block to white
    for ( unsigned int yPos = 20 ; yPos < 40 ; yPos++ ) {
        for ( unsigned int xPos = 0 ; xPos < 20 ; xPos++ )
            testImage.setPixel( xPos , yPos , sf::Color( 255 , 255 , 255 ) );
    }

    // Prepare the test pattern for drawing
    static sf::Texture testTexture;
    testTexture.loadFromImage( testImage );
    testTexture.setRepeated( true );

    static sf::Sprite calibrationSprite( testTexture );
    calibrationSprite.setTextureRect( sf::IntRect( 0 , 0 , getSize().x , getSize().y ) );

    // Create red border for window
    static sf::RectangleShape border( sf::Vector2f( getSize().x - 40.f , getSize().y - 40.f ) );
    border.setFillColor( sf::Color( 0 , 0 , 0 , 0 ) );
    border.setOutlineThickness( 20 );
    border.setOutlineColor( outlineColor );

    // Set graphics positions before drawing them
    calibrationSprite.setPosition( 20.f , 20.f );
    border.setPosition( 20.f , 20.f );

    // Make window visible before drawing to it
    ShowWindow( getSystemHandle() , SW_SHOWMAXIMIZED );

    // Draw graphics to window
    clear( sf::Color( 0 , 0 , 0 ) );
    draw( calibrationSprite );
    draw( border );
    display();
}

void drawTestPattern( sf::RenderWindow& targetWin , const sf::Color& outlineColor ) {
    // keep mouse from covering up pattern
    targetWin.setMouseCursorVisible( false );

    /* ===== Create and draw the calibration graphic ===== */
    // Construct pixel data
    static sf::Image testImage;
    testImage.create( 40 , 40 , sf::Color( 0 , 0 , 0 ) );

    // Change top-left block to white
    for ( unsigned int yPos = 0 ; yPos < 20 ; yPos++ ) {
        for ( unsigned int xPos = 20 ; xPos < 40 ; xPos++ )
            testImage.setPixel( xPos , yPos , sf::Color( 255 , 255 , 255 ) );
    }

    // Change bottom-right block to white
    for ( unsigned int yPos = 20 ; yPos < 40 ; yPos++ ) {
        for ( unsigned int xPos = 0 ; xPos < 20 ; xPos++ )
            testImage.setPixel( xPos , yPos , sf::Color( 255 , 255 , 255 ) );
    }

    // Prepare the test pattern for drawing
    static sf::Texture testTexture;
    testTexture.loadFromImage( testImage );
    testTexture.setRepeated( true );

    static sf::Sprite calibrationSprite( testTexture );
    calibrationSprite.setTextureRect( sf::IntRect( 0 , 0 , targetWin.getSize().x , targetWin.getSize().y ) );

    // Create red border for window
    static sf::RectangleShape border( sf::Vector2f( targetWin.getSize().x - 40.f , targetWin.getSize().y - 40.f ) );
    border.setFillColor( sf::Color( 0 , 0 , 0 , 0 ) );
    border.setOutlineThickness( 20 );
    border.setOutlineColor( outlineColor );

    // Set graphics positions before drawing them
    calibrationSprite.setPosition( 20.f , 20.f );
    border.setPosition( 20.f , 20.f );

    // Make window visible before drawing to it
    ShowWindow( targetWin.getSystemHandle() , SW_SHOWMAXIMIZED );

    // Draw graphics to window
    targetWin.clear( sf::Color( 0 , 0 , 0 ) );
    targetWin.draw( calibrationSprite );
    targetWin.draw( border );
    targetWin.display();
}

}
