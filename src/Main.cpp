//=============================================================================
//File Name: Main.cpp
//Description: Handles processing Kinect input
//Author: Tyler Veness
//=============================================================================

/*
 * TODO Add support for multiple monitors
 */

#include "TestScreen.hpp"

#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum {
    IDC_RECALIBRATE_BUTTON = 101,
    IDC_STREAM_TOGGLE_BUTTON = 102
};

#include "Kinect.hpp"

// global because the drawing is set up to be continuous in CALLBACK OnEvent
HWND videoWindow = NULL;
HWND depthWindow = NULL;
HICON kinectON = NULL;
HICON kinectOFF = NULL;
Kinect* projectorKinectPtr = NULL;

LRESULT CALLBACK OnEvent( HWND Handle , UINT Message , WPARAM WParam , LPARAM LParam );

BOOL CALLBACK MonitorEnumProc(
    HMONITOR hMonitor,
    HDC hdcMonitor,
    LPRECT lprcMonitor,
    LPARAM dwData
);

INT WINAPI WinMain( HINSTANCE Instance , HINSTANCE , LPSTR , INT ) {
    const char* mainClassName = "KinectBoard";

    kinectON = LoadIcon( Instance , "kinect1-ON" );
    kinectOFF = LoadIcon( Instance , "kinect2-OFF" );

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
    WindowClass.hIcon         = kinectOFF;
    WindowClass.hCursor       = NULL;
    WindowClass.hbrBackground = mainBrush;
    WindowClass.lpszMenuName  = NULL;
    WindowClass.lpszClassName = mainClassName;
    WindowClass.hIconSm       = kinectOFF;
    RegisterClassEx(&WindowClass);

    MSG Message;

    /* ===== Make a new window that isn't fullscreen ===== */
    RECT winSize = { 0 , 0 , static_cast<int>(ImageVars::width) , static_cast<int>(ImageVars::height) }; // set the size, but not the position
    AdjustWindowRect(
            &winSize ,
            WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPCHILDREN ,
            FALSE ); // adjust the size

    // Create a new window to be used for the lifetime of the application
    videoWindow = CreateWindowEx( 0 ,
            mainClassName ,
            "KinectBoard - Video" ,
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

    // Make windows receive stream events from Kinect instance
    projectorKinect.registerVideoWindow( videoWindow );
    projectorKinect.registerDepthWindow( depthWindow );

    projectorKinect.startVideoStream();
    //projectorKinect.startDepthStream();
    projectorKinect.enableColor( Processing::Red );
    projectorKinect.enableColor( Processing::Blue );

    // Calibrate Kinect
    SendMessage( videoWindow , WM_COMMAND , IDC_RECALIBRATE_BUTTON , 0 );

    while ( GetMessage( &Message , NULL , 0 , 0 ) > 0 ) {
        // If a message was waiting in the message queue, process it
        TranslateMessage( &Message );
        DispatchMessage( &Message );
    }

    UnregisterClass( mainClassName , Instance );

    return Message.wParam;
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
                    // If button was pressed from video display window
                    if ( Handle == videoWindow ) {
                        if ( projectorKinectPtr->isVideoStreamRunning() ) {
                            projectorKinectPtr->stopVideoStream();
                        }
                        else {
                            projectorKinectPtr->startVideoStream();
                        }
                    }

                    // If button was pressed from depth image display window
                    else if ( Handle == depthWindow ) {
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

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc;

        hdc = BeginPaint( Handle , &ps );

        // If we're painting the video display window
        if ( Handle == videoWindow ) {
            projectorKinectPtr->displayVideo( videoWindow , 0 , 0 , hdc );
        }

        // If we're painting the depth image display window
        else if ( Handle == depthWindow ) {
            projectorKinectPtr->displayDepth( depthWindow , 0 , 0 , hdc );
        }

        EndPaint( Handle , &ps );

        break;
    }

    case WM_DESTROY: {
        // If a display window is being closed, exit the application
        if ( Handle == videoWindow || Handle == depthWindow ) {
            PostQuitMessage( 0 );
        }

        break;
    }

    case WM_KINECT_VIDEOSTART: {
        // Change video window icon to green because the stream started
        PostMessage( Handle , WM_SETICON , ICON_SMALL , (LPARAM)kinectON );
        PostMessage( Handle , WM_SETICON , ICON_BIG , (LPARAM)kinectON );

        break;
    }

    case WM_KINECT_VIDEOSTOP: {
        // Change video window icon to red because the stream stopped
        PostMessage( Handle , WM_SETICON , ICON_SMALL , (LPARAM)kinectOFF );
        PostMessage( Handle , WM_SETICON , ICON_BIG , (LPARAM)kinectOFF );

        break;
    }

    case WM_KINECT_DEPTHSTART: {
        // Change depth window icon to green because the stream started
        PostMessage( Handle , WM_SETICON , ICON_SMALL , (LPARAM)kinectON );
        PostMessage( Handle , WM_SETICON , ICON_BIG , (LPARAM)kinectON );

        break;
    }

    case WM_KINECT_DEPTHSTOP: {
        // Change depth window icon to red because the stream stopped
        PostMessage( Handle , WM_SETICON , ICON_SMALL , (LPARAM)kinectOFF );
        PostMessage( Handle , WM_SETICON , ICON_BIG , (LPARAM)kinectOFF );

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
