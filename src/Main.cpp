//=============================================================================
//File Name: Main.cpp
//Description: Handles processing Kinect input
//Author: Tyler Veness
//=============================================================================

/*
 * TODO Use default fonts and SystemParametersInfo to avoid need to load
 *      external .ttf files
 * TODO Add support for multiple monitors
 */

#include <SFML/Graphics/RenderWindow.hpp>

#include "TestScreen.hpp"

#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct tagINPUT INPUT, *PINPUT;

enum {
    IDC_RECALIBRATE_BUTTON = 101
};

#include "WinAPIWrapper.h"
#include "Kinect.hpp"
#include <iostream> // TODO Remove me

// global because the drawing is set up to be continuous in CALLBACK OnEvent
sf::RenderWindow testWin;
Kinect projectorKinect;
volatile bool isOpen = true;

LRESULT CALLBACK OnEvent( HWND Handle , UINT Message , WPARAM WParam , LPARAM LParam );

BOOL CALLBACK MonitorEnumProc(
    HMONITOR hMonitor,
    HDC hdcMonitor,
    LPRECT lprcMonitor,
    LPARAM dwData
);

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

    MSG Message;
    INPUT input = { 0 };

    /* ===== Make a new window that isn't fullscreen ===== */
    RECT winSize = { 0 , 0 , Image::Width , Image::Height }; // set the size, but not the position
    AdjustWindowRect(
            &winSize ,
            WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPCHILDREN ,
            FALSE ); // adjust the size

    // Create a new window to be used for the lifetime of the application
    HWND mainWindow = CreateWindowEx( 0 ,
            mainClassName ,
            "KinectBoard" ,
            WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPCHILDREN ,
            ( GetSystemMetrics(SM_CXSCREEN) - ( winSize.right - winSize.left ) ) / 2 ,
            ( GetSystemMetrics(SM_CYSCREEN) - ( winSize.bottom - winSize.top ) ) / 2 ,
            winSize.right - winSize.left , // returns image width (resized as window)
            winSize.bottom - winSize.top , // returns image height (resized as window)
            NULL ,
            NULL ,
            Instance ,
            NULL );
    /* =================================================== */

    HWND recalibButton = CreateWindowEx( 0,
            "BUTTON",
            "Recalibrate",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            9,
            Image::Height - 9 - 24,
            100,
            24,
            mainWindow,
            reinterpret_cast<HMENU>( IDC_RECALIBRATE_BUTTON ),
            GetModuleHandle( NULL ),
            NULL);

    SendMessage( recalibButton,
            WM_SETFONT,
            reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
            MAKELPARAM( FALSE , 0 ) );

    SendMessage( mainWindow , WM_COMMAND , IDC_RECALIBRATE_BUTTON , 0 ); // Calibrate Kinect

    bool lastConnection = true; // prevents window icon from being set every loop

    while ( isOpen ) {
        // If a message was waiting in the message queue, process it
        if ( PeekMessage( &Message , NULL , 0 , 0 , PM_REMOVE ) > 0 ) {
            TranslateMessage( &Message );
            DispatchMessage( &Message );
        }

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

        //projectorKinect.processImage( Kinect::Red );
        //projectorKinect.processImage( Kinect::Green );
        //projectorKinect.processImage( Kinect::Blue );
        //projectorKinect.combineProcessedImages();
        projectorKinect.display( mainWindow , 0 , 0 );

        // TODO get images from Kinect, process them, and move mouse and press mouse buttons

        //moveMouse( input , -5 , 0 ); // move mouse cursor 5 pixels left
        //moveMouse( input , 5 , 0 ); // move mouse cursor 5 pixels right

        //leftClick( input ); // left click for testing

        Sleep( 50 );
    }

    // Clean up windows
    testWin.close();
    //DestroyWindow( testWindow );
    //DestroyWindow( mainWindow );
    UnregisterClass( mainClassName , Instance );

    return Message.wParam;
}

LRESULT CALLBACK OnEvent( HWND Handle , UINT Message , WPARAM WParam , LPARAM LParam ) {
    switch ( Message ) {
    case WM_COMMAND: {
        switch( LOWORD(WParam) ) {
            case IDC_RECALIBRATE_BUTTON: {
                // if there is no Kinect connected, don't bother trying to retrieve images
                /*if ( projectorKinect.isConnected() ) {
                    sf::TestScreen test;

                    test.setColor( sf::TestScreen::Red );
                    test.display();
                    Sleep( 100 ); // give Kinect time to get image w/ test pattern
                    projectorKinect.processImage( Kinect::Red );

                    test.setColor( sf::TestScreen::Red );
                    test.display();
                    Sleep( 100 ); // give Kinect time to get image w/ test pattern
                    projectorKinect.processImage( Kinect::Green );

                    test.setColor( sf::TestScreen::Red );
                    test.display();
                    Sleep( 100 ); // give Kinect time to get image w/ test pattern
                    projectorKinect.processImage( Kinect::Blue );

                    projectorKinect.combineProcessedImages();

                    // TODO process Kinect image to find location of Kinect in 3D space
                }*/

                ShowWindow( testWin.getSystemHandle() , SW_MINIMIZE );
                break;
            }

            case WM_DESTROY: {
                PostQuitMessage( 0 );
                break;
            }
        }
        break;
    }

    case WM_DESTROY: {
        PostQuitMessage( 0 );
        isOpen = false;
        break;
    }

    default: {
        return DefWindowProc( Handle , Message , WParam , LParam );
    }
    }

    return 0;
}

BOOL CALLBACK MonitorEnumProc(
    HMONITOR hMonitor,
    HDC hdcMonitor,
    LPRECT lprcMonitor,
    LPARAM dwData
) {
    return FALSE;
}
