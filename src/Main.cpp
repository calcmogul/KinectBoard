//=============================================================================
//File Name: Main.cpp
//Description: Handles processing Kinect input
//Author: Tyler Veness
//=============================================================================

/*
 * TODO Add support for multiple monitors
 */

#include "TestScreen.hpp"

#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct tagINPUT INPUT, *PINPUT;

enum {
    IDC_RECALIBRATE_BUTTON = 101,
    IDC_STREAM_TOGGLE_BUTTON = 102
};

#include "WinAPIWrapper.h"
#include "Kinect.hpp"

// global because the drawing is set up to be continuous in CALLBACK OnEvent
HWND mainWindow = NULL;
HWND depthWindow = NULL;
Kinect* projectorKinectPtr = NULL;
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

    HBRUSH mainBrush = CreateSolidBrush( RGB( 0 , 0 , 0 ) );

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
    RECT winSize = { 0 , 0 , static_cast<int>(ImageVars::width) , static_cast<int>(ImageVars::height) }; // set the size, but not the position
    AdjustWindowRect(
            &winSize ,
            WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPCHILDREN ,
            FALSE ); // adjust the size

    // Create a new window to be used for the lifetime of the application
    mainWindow = CreateWindowEx( 0 ,
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

    depthWindow = CreateWindowEx( 0 ,
            mainClassName ,
            "KinectBoard - Depth" ,
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

    Kinect projectorKinect;
    projectorKinectPtr = &projectorKinect;
    projectorKinect.startVideoStream();
    //projectorKinect.startDepthStream();
    projectorKinect.enableColor( Processing::Red );
    projectorKinect.enableColor( Processing::Blue );

    // Calibrate Kinect
    SendMessage( mainWindow , WM_COMMAND , IDC_RECALIBRATE_BUTTON , 0 );

    // These prevent window icons from being set every loop
    bool videoWasRunning = true;
    bool depthWasRunning = true;

    while ( isOpen ) {
        // If a message was waiting in the message queue, process it
        if ( PeekMessage( &Message , NULL , 0 , 0 , PM_REMOVE ) > 0 ) {
            TranslateMessage( &Message );
            DispatchMessage( &Message );
        }


        // Change RGB window icon to red or green depending upon stream status
        if ( projectorKinect.isVideoStreamRunning() && videoWasRunning == false ) {
            SendMessage( mainWindow , WM_SETICON , ICON_SMALL , (LPARAM)kinectON );
            SendMessage( mainWindow , WM_SETICON , ICON_BIG , (LPARAM)kinectON );

            videoWasRunning = true;
        }
        else if ( !projectorKinect.isVideoStreamRunning() && videoWasRunning == true ) {
            SendMessage( mainWindow , WM_SETICON , ICON_SMALL , (LPARAM)kinectOFF );
            SendMessage( mainWindow , WM_SETICON , ICON_BIG , (LPARAM)kinectOFF );

            videoWasRunning = false;
        }


        // Change depth window icon to red or green depending upon stream status
        if ( projectorKinect.isDepthStreamRunning() && depthWasRunning == false ) {
            SendMessage( depthWindow , WM_SETICON , ICON_SMALL , (LPARAM)kinectON );
            SendMessage( depthWindow , WM_SETICON , ICON_BIG , (LPARAM)kinectON );

            depthWasRunning = true;
        }
        else if ( !projectorKinect.isDepthStreamRunning() && depthWasRunning == true ) {
            SendMessage( depthWindow , WM_SETICON , ICON_SMALL , (LPARAM)kinectOFF );
            SendMessage( depthWindow , WM_SETICON , ICON_BIG , (LPARAM)kinectOFF );

            depthWasRunning = false;
        }


        //projectorKinect.processCalibImages( Kinect::Red );
        //projectorKinect.processCalibImages( Kinect::Green );
        //projectorKinect.processCalibImages( Kinect::Blue );
        //projectorKinect.combineCalibImages();
        projectorKinect.displayVideo( mainWindow , 0 , 0 );
        projectorKinect.displayDepth( depthWindow , 0 , 0 );

        // TODO get images from Kinect, process them, and move mouse and press mouse buttons

        //moveMouse( input , -5 , 0 ); // move mouse cursor 5 pixels left
        //moveMouse( input , 5 , 0 ); // move mouse cursor 5 pixels right

        //leftClick( input ); // left click for testing

        Sleep( 50 );
    }

    // Clean up windows
    //DestroyWindow( mainWindow );
    UnregisterClass( mainClassName , Instance );

    return EXIT_SUCCESS;
}

LRESULT CALLBACK OnEvent( HWND Handle , UINT Message , WPARAM WParam , LPARAM LParam ) {
    switch ( Message ) {
    case WM_CREATE: {
        HWND recalibButton = CreateWindowEx( 0,
                "BUTTON",
                "Recalibrate",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                9,
                ImageVars::height - 9 - 24,
                100,
                24,
                Handle,
                reinterpret_cast<HMENU>( IDC_RECALIBRATE_BUTTON ),
                GetModuleHandle( NULL ),
                NULL);

        SendMessage( recalibButton,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );


        HWND toggleStreamButton = CreateWindowEx( 0,
                "BUTTON",
                "Start/Stop",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                ImageVars::width - 9 - 100,
                ImageVars::height - 9 - 24,
                100,
                24,
                Handle,
                reinterpret_cast<HMENU>( IDC_STREAM_TOGGLE_BUTTON ),
                GetModuleHandle( NULL ),
                NULL);

        SendMessage( toggleStreamButton,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );

        break;
    }

    case WM_COMMAND: {
        switch( LOWORD(WParam) ) {
            case IDC_RECALIBRATE_BUTTON: {
                if ( projectorKinectPtr != NULL ) {
                    // if there is no Kinect connected, don't bother trying to retrieve images
                    if ( projectorKinectPtr->isVideoStreamRunning() ) {
                        sf::TestScreen testWin( "KinectBoard" , Handle , NULL );

                        testWin.setColor( Processing::Red );
                        testWin.display();
                        Sleep( 600 ); // give Kinect time to get image w/ test pattern
                        projectorKinectPtr->setCalibImage( Processing::Red );

                        testWin.setColor( Processing::Blue );
                        testWin.display();
                        Sleep( 600 ); // give Kinect time to get image w/ test pattern
                        projectorKinectPtr->setCalibImage( Processing::Blue );

                        projectorKinectPtr->calibrate();

                        testWin.close();
                    }
                }

                break;
            }

            case IDC_STREAM_TOGGLE_BUTTON: {
                if ( projectorKinectPtr != NULL ) {
                    // If button was pressed from RGB display window
                    if ( Handle == mainWindow ) {
                        if ( projectorKinectPtr->isVideoStreamRunning() ) {
                            projectorKinectPtr->saveVideo( "white.png" );
                            projectorKinectPtr->stopVideoStream();
                        }
                        else {
                            projectorKinectPtr->startVideoStream();
                        }
                    }

                    // If button was pressed from depth display window
                    if ( Handle == depthWindow ) {
                        if ( projectorKinectPtr->isDepthStreamRunning() ) {
                            projectorKinectPtr->stopDepthStream();
                        }
                        else {
                            projectorKinectPtr->startDepthStream();
                        }
                    }
                }

                break;
            }
        }

        break;
    }

    case WM_CLOSE: {
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
