//=============================================================================
//File Name: Main.cpp
//Description: Handles processing Kinect input
//Author: Tyler Veness
//=============================================================================

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct tagINPUT INPUT, *PINPUT;

enum {
    IDC_RECALIBRATE_BUTTON = 101
};

#include "WinAPIWrapper.h"
#include "Kinect.hpp"

// global because the drawing is set up to be continuous in CALLBACK OnEvent
sf::RenderWindow mainWin;
sf::RenderWindow testWin;
Kinect projectorKinect;

LRESULT CALLBACK OnEvent( HWND Handle , UINT Message , WPARAM WParam , LPARAM LParam );

void drawTestPattern( sf::RenderWindow& targetWin , const sf::Color& outlineColor );

INT WINAPI WinMain( HINSTANCE Instance , HINSTANCE , LPSTR , INT ) {
    const char* mainClassName = "KinectBoard";

    HICON kinectON = LoadIcon( Instance , "kinect1-ON" );
    HICON kinectOFF = LoadIcon( Instance , "kinect2-OFF" );

    HBRUSH mainBrush = CreateSolidBrush( RGB( 160 , 160 , 160 ) );

    // Define a class for our main window
    WNDCLASSEX WindowClass;
    ZeroMemory( &WindowClass , sizeof(WNDCLASSEX) );
    WindowClass.cbSize        = sizeof(WNDCLASSEX);
    WindowClass.style         = 0;
    WindowClass.lpfnWndProc   = &OnEvent;
    WindowClass.cbClsExtra    = 0;
    WindowClass.cbWndExtra    = 0;
    WindowClass.hInstance     = Instance;
    WindowClass.hIcon         = kinectON;
    WindowClass.hCursor       = NULL;
    WindowClass.hbrBackground = mainBrush;
    WindowClass.lpszMenuName  = NULL;
    WindowClass.lpszClassName = mainClassName;
    RegisterClassEx(&WindowClass);

    // Calibration window
    HWND testWindow = CreateWindowEx( 0 , mainClassName , "KinectBoard" , WS_POPUP | WS_MINIMIZE , 0 , 0 , GetSystemMetrics(SM_CXSCREEN) , GetSystemMetrics(SM_CYSCREEN) , NULL , NULL , Instance , NULL );

    testWin.create( testWindow );

    MSG Message;
    INPUT input = { 0 };

    /* ===== Make a new window that isn't fullscreen ===== */
    RECT winSize = { 0 , 0 , 320 , 240 }; // set the size, but not the position
    AdjustWindowRect(
            &winSize ,
            WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX | WS_MINIMIZE | WS_CLIPCHILDREN ,
            FALSE ); // adjust the size

    // Create a new window to be used for the lifetime of the application
    HWND mainWindow = CreateWindow( mainClassName ,
            "KinectBoard" ,
            WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX | WS_MINIMIZE | WS_CLIPCHILDREN ,
            ( GetSystemMetrics(SM_CXSCREEN) - 200 ) / 2 ,
            ( GetSystemMetrics(SM_CYSCREEN) - 150 ) / 2 ,
            winSize.right - winSize.left , // 320
            winSize.bottom - winSize.top , // 240
            NULL ,
            NULL ,
            Instance ,
            NULL );
    mainWin.create( mainWindow );
    /* =================================================== */

    SendMessage( mainWindow , WM_COMMAND , IDC_RECALIBRATE_BUTTON , 0 ); // Calibrate Kinect

    bool lastConnection = true; // prevents window icon from being set every loop
    while ( mainWin.isOpen() ) {
        if ( PeekMessage( &Message , NULL , 0 , 0 , PM_REMOVE ) ) {
            // If a message was waiting in the message queue, process it
            TranslateMessage( &Message );
            DispatchMessage( &Message );
        }
        else {
            // change color of icon to green or red if Kinect is connected or disconnected respectively
            if ( projectorKinect.isConnected() && lastConnection == false ) { // if is connected and wasn't before
                SendMessage( mainWindow , WM_SETICON , ICON_SMALL , reinterpret_cast<LPARAM>(kinectON) );
                SendMessage( mainWindow , WM_SETICON , ICON_BIG , reinterpret_cast<LPARAM>(kinectON) );

                lastConnection = true;
            }
            else if ( !projectorKinect.isConnected() && lastConnection == true ) { // if isn't connected and was before
                SendMessage( mainWindow , WM_SETICON , ICON_SMALL , reinterpret_cast<LPARAM>(kinectOFF) );
                SendMessage( mainWindow , WM_SETICON , ICON_BIG , reinterpret_cast<LPARAM>(kinectOFF) );

                lastConnection = false;
            }

            projectorKinect.processImage( Kinect::Red );
            projectorKinect.display( mainWindow , 0 , 0 );

            // TODO get images from Kinect, process them, and move mouse and press mouse buttons

            //moveMouse( input , -5 , 0 ); // move mouse cursor 5 pixels left
            //moveMouse( input , 5 , 0 ); // move mouse cursor 5 pixels right

            //leftClick( input ); // left click for testing

            Sleep( 50 );
        }
    }

    // Clean up windows
    testWin.close();
    DestroyWindow( testWindow );
    DestroyWindow( mainWindow );
    UnregisterClass( mainClassName , Instance );

    return EXIT_SUCCESS;
}

LRESULT CALLBACK OnEvent( HWND Handle , UINT Message , WPARAM WParam , LPARAM LParam ) {
    switch ( Message ) {
    case WM_CREATE: {
        HGDIOBJ hfDefault = GetStockObject( DEFAULT_GUI_FONT );

        HWND hWndButton = CreateWindow(
                "BUTTON",
                "Recalibrate",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                9,
                240 - 9 - 24,
                100,
                24,
                Handle,
                reinterpret_cast<HMENU>( IDC_RECALIBRATE_BUTTON ),
                GetModuleHandle( NULL ),
                NULL);

        SendMessage(hWndButton,
                    WM_SETFONT,
                    reinterpret_cast<WPARAM>( hfDefault ),
                    MAKELPARAM( FALSE , 0 ) );

        break;
    }

    case WM_COMMAND: {
        switch( LOWORD(WParam) ) {
            case IDC_RECALIBRATE_BUTTON: {
                // if there is no Kinect connected, don't bother trying to retrieve images
                if ( projectorKinect.isConnected() ) {
                    drawTestPattern( testWin , sf::Color( 255 , 0 , 0 ) );
                    Sleep( 100 ); // give Kinect time to get image w/ test pattern
                    projectorKinect.processImage( Kinect::Red );

                    drawTestPattern( testWin , sf::Color( 0 , 255 , 0 ) );
                    Sleep( 100 ); // give Kinect time to get image w/ test pattern
                    projectorKinect.processImage( Kinect::Green );

                    drawTestPattern( testWin , sf::Color( 0 , 0 , 255 ) );
                    Sleep( 100 ); // give Kinect time to get image w/ test pattern
                    projectorKinect.processImage( Kinect::Blue );

                    projectorKinect.combineImages();

                    // TODO process Kinect image to find location of Kinect in 3D space
                }

                ShowWindow( testWin.getSystemHandle() , SW_MINIMIZE | SW_HIDE );
            }
            break;
        }
        break;

        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        break;
    }

    case WM_SIZE: {
        projectorKinect.display( Handle , 0 , 0 );
        break;
    }

    case WM_WINDOWPOSCHANGED: {
        projectorKinect.display( Handle , 0 , 0 );
        break;
    }

    // Quit when we close the main window
    case WM_CLOSE: {
        mainWin.close();
        PostQuitMessage(0);
        break;
    }

    default: {
        return DefWindowProc(Handle, Message, WParam, LParam);
    }
    }

    return 0;
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
    ShowWindow( targetWin.getSystemHandle() , SW_MAXIMIZE );

    // Draw graphics to window
    targetWin.clear( sf::Color( 0 , 0 , 0 ) );
    targetWin.draw( calibrationSprite );
    targetWin.draw( border );
    targetWin.display();
}
