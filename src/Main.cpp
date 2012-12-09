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
#include <sstream>
#include <iostream> // TODO Remove me

#include "Resource.h"

#include "TestScreen.hpp"
#include "Kinect.hpp"

typedef struct CurrentMonitor {
    // index of monitor in global list
    unsigned int index;
    HWND activeButton;

    // size of monitor client area
    RECT size;
} monitorUsed;

// Global because the drawing is set up to be continuous in CALLBACK OnEvent
HINSTANCE hInst = NULL;
HWND videoWindow = NULL;
HWND depthWindow = NULL;
HICON kinectON = NULL;
HICON kinectOFF = NULL;
Kinect* projectorKinectPtr = NULL;

// Used for choosing on which monitor to draw test image
std::list<RECT*> gMonitors;
CurrentMonitor currentMonitor;

LRESULT CALLBACK OnEvent( HWND Handle , UINT Message , WPARAM WParam , LPARAM LParam );

INT_PTR CALLBACK AboutCbk( HWND hDlg , UINT message , WPARAM wParam , LPARAM lParam );
INT_PTR CALLBACK MonitorCbk( HWND hDlg , UINT message , WPARAM wParam , LPARAM lParam );

BOOL CALLBACK MonitorEnumProc(
    HMONITOR hMonitor,
    HDC hdcMonitor,
    LPRECT lprcMonitor,
    LPARAM dwData
);

BOOL CALLBACK StartStreamChildCbk(
    HWND hwndChild,
    LPARAM lParam
);

BOOL CALLBACK StopStreamChildCbk(
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

LRESULT CALLBACK OnEvent( HWND handle , UINT message , WPARAM wParam , LPARAM lParam ) {
    switch ( message ) {
    case WM_CREATE: {
        HWND recalibButton = CreateWindowEx( 0,
                "BUTTON",
                "Recalibrate",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                9,
                ImageVars::height - 9 - 28,
                100,
                28,
                handle,
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
                handle,
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
        int wmId    = LOWORD( wParam );
        //int wmEvent = HIWORD( wParam );

        switch( wmId ) {
            case IDC_RECALIBRATE_BUTTON: {
                if ( projectorKinectPtr != NULL ) {
                    // If there is no Kinect connected, don't bother trying to retrieve images
                    if ( projectorKinectPtr->isVideoStreamRunning() ) {
                        TestScreen testWin( hInst , true );

                        testWin.setColor( Processing::Red );
                        testWin.display();
                        // Give Kinect time to get image w/ test pattern in it
                        Sleep( 750 );
                        projectorKinectPtr->setCalibImage( Processing::Red );

                        testWin.setColor( Processing::Blue );
                        testWin.display();
                        // Give Kinect time to get image w/ test pattern in it
                        Sleep( 750 );
                        projectorKinectPtr->setCalibImage( Processing::Blue );

                        projectorKinectPtr->calibrate();
                    }
                }

                break;
            }

            case IDC_STREAM_TOGGLE_BUTTON: {
                if ( projectorKinectPtr != NULL ) {
                    // If button was pressed from video display window
                    if ( handle == videoWindow ) {
                        if ( projectorKinectPtr->isVideoStreamRunning() ) {
                            projectorKinectPtr->stopVideoStream();
                        }
                        else {
                            projectorKinectPtr->startVideoStream();
                        }
                    }

                    // If button was pressed from depth image display window
                    else if ( handle == depthWindow ) {
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
                EnumDisplayMonitors(
                        NULL, // List all monitors
                        NULL, // Don't clip area
                        MonitorEnumProc,
                        (LPARAM)&gMonitors // User data
                );

                DialogBox( hInst , MAKEINTRESOURCE(IDD_MONITORBOX) , handle , MonitorCbk );

                unsigned short count = 0;
                for ( std::list<RECT*>::iterator i = gMonitors.begin() ; i != gMonitors.end() ; i++ ) {
                    std::cout << count << ": (" << (*i)->left << ", " << (*i)->top << ") -> (" << (*i)->right << ", " << (*i)->bottom << ")\n";
                    delete *i;
                    count++;
                }
                gMonitors.clear();

                break;
            }

            case IDM_ABOUT: {
                DialogBox( hInst , MAKEINTRESOURCE(IDD_ABOUTBOX) , handle , AboutCbk );
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

        hdc = BeginPaint( handle , &ps );

        // If we're painting the video display window
        if ( handle == videoWindow ) {
            projectorKinectPtr->displayVideo( videoWindow , 0 , 0 , hdc );
        }

        // If we're painting the depth image display window
        else if ( handle == depthWindow ) {
            projectorKinectPtr->displayDepth( depthWindow , 0 , 0 , hdc );
        }

        EndPaint( handle , &ps );

        break;
    }

    case WM_DESTROY: {
        // If a display window is being closed, exit the application
        if ( handle == videoWindow || handle == depthWindow ) {
            PostQuitMessage( 0 );
        }

        break;
    }

    case WM_KINECT_VIDEOSTART: {
        // Change video window icon to green because the stream started
        PostMessage( handle , WM_SETICON , ICON_SMALL , (LPARAM)kinectON );
        PostMessage( handle , WM_SETICON , ICON_BIG , (LPARAM)kinectON );

        // Change Start/Stop button text to "Stop"
        char* windowText = (char*)std::malloc( 16 );
        EnumChildWindows( handle , StartStreamChildCbk , (LPARAM)windowText );
        std::free( windowText );

        break;
    }

    case WM_KINECT_VIDEOSTOP: {
        // Change video window icon to red because the stream stopped
        PostMessage( handle , WM_SETICON , ICON_SMALL , (LPARAM)kinectOFF );
        PostMessage( handle , WM_SETICON , ICON_BIG , (LPARAM)kinectOFF );

        // Change Start/Stop button text to "Start"
        char* windowText = (char*)std::malloc( 16 );
        EnumChildWindows( handle , StopStreamChildCbk , (LPARAM)windowText );
        std::free( windowText );

        break;
    }

    case WM_KINECT_DEPTHSTART: {
        // Change depth window icon to green because the stream started
        PostMessage( handle , WM_SETICON , ICON_SMALL , (LPARAM)kinectON );
        PostMessage( handle , WM_SETICON , ICON_BIG , (LPARAM)kinectON );

        // Change Start/Stop button text to "Stop"
        char* windowText = (char*)std::malloc( 16 );
        EnumChildWindows( handle , StartStreamChildCbk , (LPARAM)windowText );
        std::free( windowText );

        break;
    }

    case WM_KINECT_DEPTHSTOP: {
        // Change depth window icon to red because the stream stopped
        PostMessage( handle , WM_SETICON , ICON_SMALL , (LPARAM)kinectOFF );
        PostMessage( handle , WM_SETICON , ICON_BIG , (LPARAM)kinectOFF );

        // Change Start/Stop button text to "Start"
        char* windowText = (char*)std::malloc( 16 );
        EnumChildWindows( handle , StopStreamChildCbk , (LPARAM)windowText );
        std::free( windowText );

        break;
    }

    default: {
        return DefWindowProc( handle , message , wParam , lParam );
    }
    }

    return 0;
}

// Message handler for "Change Monitor" box
INT_PTR CALLBACK MonitorCbk( HWND hDlg , UINT message , WPARAM wParam , LPARAM lParam ) {
    switch ( message ) {
    case WM_INITDIALOG: {
        for ( std::list<RECT*>::iterator i = gMonitors.begin() ; i != gMonitors.end() ; i++ ) {
            CreateWindowEx( 0,
                "BUTTON",
                "",
                WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                (*i)->left / 10,
                (*i)->top / 10,
                ( (*i)->right - (*i)->left ) / 10,
                ( (*i)->bottom - (*i)->top ) / 10,
                hDlg,
                reinterpret_cast<HMENU>( NULL ),
                hInst,
                NULL);
        }

        return (INT_PTR)TRUE;
    }

    case WM_DRAWITEM: {
         LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;

         POINT rectanglePts[4];
         HRGN rectRgn;

         rectanglePts[0] = { 0 , 0 };
         rectanglePts[1] = { pDIS->rcItem.right , 0 };
         rectanglePts[2] = { pDIS->rcItem.right , pDIS->rcItem.bottom };
         rectanglePts[3] = { 0 , pDIS->rcItem.bottom };

         rectRgn = CreatePolygonRgn( rectanglePts , 4 , ALTERNATE );

         // If button being drawn represents the active monitor
         if ( currentMonitor.activeButton == pDIS->hwndItem ) {
             HBRUSH blueBrush = CreateSolidBrush( RGB( 0 , 0 , 120 ) );

             // Draw blue background to window
             FillRgn( pDIS->hDC , rectRgn , blueBrush );

             DeleteObject( blueBrush );
         }
         else {
             HBRUSH normalBrush = CreateSolidBrush( RGB( 70 , 70 , 70 ) );

             // Draw normal background to window
             FillRgn( pDIS->hDC , rectRgn , normalBrush );

             DeleteObject( normalBrush );
         }

         DeleteObject( rectRgn );

         return (INT_PTR)TRUE;
     }

    case WM_COMMAND:
        if ( LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL ) {
            EndDialog(hDlg, LOWORD(wParam));

            return (INT_PTR)TRUE;
        }
        else {
            GetClientRect( (HWND)lParam , &currentMonitor.size );
            currentMonitor.size.left *= 10;
            currentMonitor.size.top *= 10;
            currentMonitor.size.right *= 10;
            currentMonitor.size.bottom *= 10;

            currentMonitor.activeButton = (HWND)lParam;
        }

        break;
    }

    return (INT_PTR)FALSE;
}

// Message handler for "About" box
INT_PTR CALLBACK AboutCbk( HWND hDlg , UINT message , WPARAM wParam , LPARAM lParam ) {
    UNREFERENCED_PARAMETER(lParam);
    switch ( message ) {
    case WM_INITDIALOG: {
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND: {
        if ( LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL ) {
            EndDialog( hDlg , LOWORD(wParam) );
            return (INT_PTR)TRUE;
        }
        break;
    }
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

BOOL CALLBACK StartStreamChildCbk(
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

BOOL CALLBACK StopStreamChildCbk(
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
