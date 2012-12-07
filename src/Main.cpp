//=============================================================================
//File Name: Main.cpp
//Description: Handles processing Kinect input
//Author: Tyler Veness
//=============================================================================

/*
 * TODO Add support for multiple monitors
 */

#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdlib>
#include <cstring>
#include <list>
#include <iostream> // TODO Remove me

#include "Resource.h"

#include "TestScreen.hpp"
#include "Kinect.hpp"

// global because the drawing is set up to be continuous in CALLBACK OnEvent
HINSTANCE hInst = NULL;
HWND videoWindow = NULL;
HWND depthWindow = NULL;
HICON kinectON = NULL;
HICON kinectOFF = NULL;
Kinect* projectorKinectPtr = NULL;

LRESULT CALLBACK OnEvent( HWND Handle , UINT Message , WPARAM WParam , LPARAM LParam );
LRESULT CALLBACK MonitorProc( HWND Handle , UINT Message , WPARAM WParam , LPARAM LParam );

INT_PTR CALLBACK AboutCbk( HWND hDlg , UINT message , WPARAM wParam , LPARAM lParam );
INT_PTR CALLBACK MonitorCbk(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK MonitorEnumProc(
    HMONITOR hMonitor,
    HDC hdcMonitor,
    LPRECT lprcMonitor,
    LPARAM dwData
);

BOOL CALLBACK StartStreamChildProc(
    HWND hwndChild,
    LPARAM lParam
);

BOOL CALLBACK StopStreamChildProc(
    HWND hwndChild,
    LPARAM lParam
);

INT WINAPI WinMain( HINSTANCE Instance , HINSTANCE , LPSTR , INT ) {
    hInst = Instance;

    const char* mainClassName = "KinectBoard";

    kinectON = LoadIcon( Instance , "kinect1-ON" );
    kinectOFF = LoadIcon( Instance , "kinect2-OFF" );

    HBRUSH mainBrush = CreateSolidBrush( RGB( 0 , 0 , 0 ) );
    HMENU mainMenu = LoadMenu( Instance , "mainMenu" );

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
    WindowClass.lpszMenuName  = "mainMenu";
    WindowClass.lpszClassName = mainClassName;
    WindowClass.hIconSm       = kinectOFF;
    RegisterClassEx(&WindowClass);

    ZeroMemory( &WindowClass , sizeof(WNDCLASSEX) );
    WindowClass.cbSize        = sizeof(WNDCLASSEX);
    WindowClass.style         = 0;
    WindowClass.lpfnWndProc   = &MonitorProc;
    WindowClass.cbClsExtra    = 0;
    WindowClass.cbWndExtra    = 0;
    WindowClass.hInstance     = Instance;
    WindowClass.hIcon         = kinectON;
    WindowClass.hCursor       = NULL;
    WindowClass.hbrBackground = NULL;
    WindowClass.lpszMenuName  = NULL;
    WindowClass.lpszClassName = "MonitorButton";
    WindowClass.hIconSm       = kinectON;
    RegisterClassEx(&WindowClass);

    MSG Message;

    /* ===== Make two windows to display ===== */
    RECT winSize = { 0 , 0 , static_cast<int>(ImageVars::width) , static_cast<int>(ImageVars::height) }; // set the size, but not the position
    AdjustWindowRect(
            &winSize ,
            WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPCHILDREN ,
            TRUE ); // window has menu

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
            mainMenu ,
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
            mainMenu ,
            Instance ,
            NULL );

    ShowWindow( depthWindow , SW_MINIMIZE );
    /* ======================================= */

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
    //SendMessage( videoWindow , WM_COMMAND , IDC_RECALIBRATE_BUTTON , 0 );

    while ( GetMessage( &Message , NULL , 0 , 0 ) > 0 ) {
        // If a message was waiting in the message queue, process it
        TranslateMessage( &Message );
        DispatchMessage( &Message );
    }

    DestroyIcon( kinectON );
    DestroyIcon( kinectOFF );

    UnregisterClass( mainClassName , Instance );
    UnregisterClass( "monitorButton" , Instance );
    TestScreen::unregisterClass();

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
                ImageVars::height - 9 - 28,
                100,
                28,
                Handle,
                reinterpret_cast<HMENU>( IDC_RECALIBRATE_BUTTON ),
                hInst,
                NULL);

        SendMessage( recalibButton,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );


        HWND toggleStreamButton = CreateWindowEx( 0,
                "BUTTON",
                "Start",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                ImageVars::width - 9 - 100,
                ImageVars::height - 9 - 28,
                100,
                28,
                Handle,
                reinterpret_cast<HMENU>( IDC_STREAM_TOGGLE_BUTTON ),
                hInst,
                NULL);

        SendMessage( toggleStreamButton,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( GetStockObject( DEFAULT_GUI_FONT ) ),
                MAKELPARAM( FALSE , 0 ) );

        break;
    }

    case WM_COMMAND: {
        int wmId    = LOWORD( WParam );
        //int wmEvent = HIWORD( WParam );

        switch( wmId ) {
            case IDC_RECALIBRATE_BUTTON: {
                if ( projectorKinectPtr != NULL ) {
                    // If there is no Kinect connected, don't bother trying to retrieve images
                    if ( projectorKinectPtr->isVideoStreamRunning() ) {
                        TestScreen testWin( hInst , true );

                        testWin.setColor( Processing::Red );
                        testWin.display();
                        // Give Kinect time to get image w/ test pattern in it
                        Sleep( 500 );
                        projectorKinectPtr->setCalibImage( Processing::Red );

                        testWin.setColor( Processing::Blue );
                        testWin.display();
                        // Give Kinect time to get image w/ test pattern in it
                        Sleep( 500 );
                        projectorKinectPtr->setCalibImage( Processing::Blue );

                        projectorKinectPtr->calibrate();
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

            case IDM_CHANGE_MONITOR: {
                std::list<RECT*> monitors;
                EnumDisplayMonitors(
                        NULL, // List all monitors
                        NULL, // Don't clip area
                        MonitorEnumProc,
                        (LPARAM)&monitors // User data
                );

                //GetWindowLong( Handle );

                HWND monitorDialog = CreateWindowEx( 0,
                        "MonitorButton",
                        "Change Board Monitor",
                        //DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | WS_CAPTION | WS_SYSMENU,
                        WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_CLIPCHILDREN,
                        ( GetSystemMetrics(SM_CXSCREEN) - 340 ) / 2,
                        ( GetSystemMetrics(SM_CYSCREEN) - 124 ) / 2,
                        340,
                        124,
                        Handle,
                        reinterpret_cast<HMENU>( NULL ),
                        hInst,
                        NULL);

                MSG Message;

                while ( GetMessage( &Message , monitorDialog , 0 , 0 ) > 0 ) {
                    TranslateMessage( &Message );
                    DispatchMessage( &Message );
                }

                for ( std::list<RECT*>::iterator i = monitors.begin() ; i != monitors.end() ; i++ ) {
                    CreateWindowEx( 0,
                            "BUTTON",
                            "",
                            WS_POPUP | WS_VISIBLE,
                            (*i)->left / 5,
                            (*i)->top / 5,
                            ( (*i)->right - (*i)->left ) / 5,
                            ( (*i)->bottom - (*i)->top ) / 5,
                            monitorDialog,
                            reinterpret_cast<HMENU>( NULL ),
                            hInst,
                            NULL);
                }

                //DialogBox(hInst, MAKEINTRESOURCE(IDD_MONITORBOX), Handle, MonitorCbk);

                //DestroyWindow( monitorDialog );

                unsigned short count = 0;
                for ( std::list<RECT*>::iterator i = monitors.begin() ; i != monitors.end() ; i++ ) {
                    std::cout << count << ": (" << (*i)->left << ", " << (*i)->top << ") -> (" << (*i)->right << ", " << (*i)->bottom << ")\n";
                    delete *i;
                    count++;
                }
                monitors.clear();

                break;
            }

            case IDM_ABOUT: {
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), Handle, AboutCbk);
            }
        }

        break;
    }

    case WM_DISPLAYCHANGE: {


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

        // Change Start/Stop button text to "Stop"
        char* windowText = (char*)std::malloc( 16 );
        EnumChildWindows( Handle , StartStreamChildProc , (LPARAM)windowText );
        std::free( windowText );

        break;
    }

    case WM_KINECT_VIDEOSTOP: {
        // Change video window icon to red because the stream stopped
        PostMessage( Handle , WM_SETICON , ICON_SMALL , (LPARAM)kinectOFF );
        PostMessage( Handle , WM_SETICON , ICON_BIG , (LPARAM)kinectOFF );

        // Change Start/Stop button text to "Start"
        char* windowText = (char*)std::malloc( 16 );
        EnumChildWindows( Handle , StopStreamChildProc , (LPARAM)windowText );
        std::free( windowText );

        break;
    }

    case WM_KINECT_DEPTHSTART: {
        // Change depth window icon to green because the stream started
        PostMessage( Handle , WM_SETICON , ICON_SMALL , (LPARAM)kinectON );
        PostMessage( Handle , WM_SETICON , ICON_BIG , (LPARAM)kinectON );

        // Change Start/Stop button text to "Stop"
        char* windowText = (char*)std::malloc( 16 );
        EnumChildWindows( Handle , StartStreamChildProc , (LPARAM)windowText );
        std::free( windowText );

        break;
    }

    case WM_KINECT_DEPTHSTOP: {
        // Change depth window icon to red because the stream stopped
        PostMessage( Handle , WM_SETICON , ICON_SMALL , (LPARAM)kinectOFF );
        PostMessage( Handle , WM_SETICON , ICON_BIG , (LPARAM)kinectOFF );

        // Change Start/Stop button text to "Start"
        char* windowText = (char*)std::malloc( 16 );
        EnumChildWindows( Handle , StopStreamChildProc , (LPARAM)windowText );
        std::free( windowText );

        break;
    }

    default: {
        return DefWindowProc( Handle , Message , WParam , LParam );
    }
    }

    return 0;
}

LRESULT CALLBACK MonitorProc( HWND Handle , UINT Message , WPARAM WParam , LPARAM LParam ) {
    switch ( Message ) {
    default: {
        return DefWindowProc( Handle , Message , WParam , LParam );
    }
    }

    return 0;
}

// Message handler for "About" box
INT_PTR CALLBACK AboutCbk(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for "Change Monitor" box
INT_PTR CALLBACK MonitorCbk(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

BOOL CALLBACK MonitorEnumProc(
    HMONITOR hMonitor,
    HDC hdcMonitor,
    LPRECT lprcMonitor,
    LPARAM dwData
) {
    std::list<RECT*>* monitors = (std::list<RECT*>*)dwData;

    monitors->push_back( new RECT( { lprcMonitor->left , lprcMonitor->top , lprcMonitor->right , lprcMonitor->bottom } ) );

    return TRUE;
}

BOOL CALLBACK StartStreamChildProc(
    HWND hwndChild,
    LPARAM lParam
) {
    char* windowText = (char*)lParam;

    if ( GetWindowText( hwndChild , windowText , 16 ) ) {
        if ( std::strcmp( windowText , "Start" ) == 0 ) {
            SetWindowText( hwndChild , "Stop" );
            return FALSE;
        }
    }

    return TRUE;
}

BOOL CALLBACK StopStreamChildProc(
    HWND hwndChild,
    LPARAM lParam
) {
    char* windowText = (char*)lParam;

    if ( GetWindowText( hwndChild , windowText , 16 ) ) {
        if ( std::strcmp( windowText , "Stop" ) == 0 ) {
            SetWindowText( hwndChild , "Start" );
            return FALSE;
        }
    }

    return TRUE;
}
