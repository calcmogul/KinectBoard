//=============================================================================
//File Name: Main.cpp
//Description: Handles processing Kinect input
//Author: Tyler Veness
//=============================================================================

#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#ifndef _WIN32_IE
#define _WIN32_IE 0x0400
#endif
#include <commctrl.h>

#include <cstdlib>
#include <cstring>
#include <list>
#include <sstream>
#include <string>

#include "Resource.h"

#include "TestScreen.hpp"
#include "Kinect.hpp"

struct MonitorIndex {
    // dimensions of monitor
    RECT dim;

    // button used to represent this monitor
    HWND activeButton;
};

// Global because the drawing is set up to be continuous in CALLBACK OnEvent
HINSTANCE gInstance = nullptr;
HWND gDisplayWindow = nullptr;
HICON gKinectON = nullptr;
HICON gKinectOFF = nullptr;
HMENU gMainMenu = nullptr;
Kinect gProjectorKnt;

// Used for choosing on which monitor to draw test image
std::list<MonitorIndex> gMonitors;
MonitorIndex gCurrentMonitor = {{0, 0, GetSystemMetrics(SM_CXSCREEN),
                                 GetSystemMetrics(SM_CYSCREEN)}, nullptr};

LRESULT CALLBACK MainProc(HWND handle, UINT message, WPARAM wParam,
                          LPARAM lParam);

BOOL CALLBACK AboutCbk(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MonitorCbk(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK MonitorEnumProc(
    HMONITOR hMonitor,
    HDC hdcMonitor,
    LPRECT lprcMonitor,
    LPARAM dwData
);

BOOL CALLBACK StartStreamChildCbk(HWND hwndChild, LPARAM lParam);

BOOL CALLBACK StopStreamChildCbk(HWND hwndChild, LPARAM lParam);

INT WINAPI WinMain(HINSTANCE Instance, HINSTANCE, LPSTR, INT) {
    gInstance = Instance;

    INITCOMMONCONTROLSEX icc;

    // Initialise common controls.
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    // Initialize menu bar and set the check boxes' initial state
    gMainMenu = LoadMenu(Instance, "mainMenu");
    CheckMenuItem(gMainMenu, IDM_DISPLAYVIDEO, MF_BYCOMMAND | MF_CHECKED);

    const char* mainClassName = "KinectBoard";

    gKinectON = LoadIcon(Instance, "kinect1-ON");
    gKinectOFF = LoadIcon(Instance, "kinect2-OFF");

    HBRUSH mainBrush = CreateSolidBrush(RGB(0, 0, 0));

    // Define a class for our main window
    WNDCLASSEX WindowClass;
    ZeroMemory(&WindowClass, sizeof(WNDCLASSEX));
    WindowClass.cbSize        = sizeof(WNDCLASSEX);
    WindowClass.style         = 0;
    WindowClass.lpfnWndProc   = &MainProc;
    WindowClass.cbClsExtra    = 0;
    WindowClass.cbWndExtra    = 0;
    WindowClass.hInstance     = Instance;
    WindowClass.hIcon         = gKinectOFF;
    WindowClass.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    WindowClass.hbrBackground = mainBrush;
    WindowClass.lpszMenuName  = "mainMenu";
    WindowClass.lpszClassName = mainClassName;
    WindowClass.hIconSm       = gKinectOFF;
    RegisterClassEx(&WindowClass);

    MSG Message;
    HACCEL hAccel;

    /* ===== Make two windows to display ===== */
    // Set the size, but not the position
    RECT winSize = {0, 0, static_cast<int>(ImageVars::width),
                    static_cast<int>(ImageVars::height)};
    AdjustWindowRect(
        &winSize,
        WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        TRUE); // window has menu

    // Create a new window to be used for the lifetime of the application
    gDisplayWindow = CreateWindowEx(0,
        mainClassName,
        "KinectBoard",
        WS_SYSMENU | WS_CAPTION | WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        (GetSystemMetrics(SM_CXSCREEN) - (winSize.right - winSize.left)) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - (winSize.bottom - winSize.top)) / 2,
        winSize.right - winSize.left, // returns image width (resized as window)
        winSize.bottom - winSize.top, // returns image height (resized as window)
        nullptr,
        gMainMenu,
        Instance,
        nullptr);
    /* ======================================= */

    // Load keyboard accelerators
    hAccel = LoadAccelerators(gInstance, "KeyAccel");

    gProjectorKnt.setScreenRect(gCurrentMonitor.dim);

    // Make window receive video stream events and image from Kinect instance
    gProjectorKnt.registerVideoWindow(gDisplayWindow);

    gProjectorKnt.startVideoStream();
    gProjectorKnt.startDepthStream();
    gProjectorKnt.enableColor(Processing::Red);
    gProjectorKnt.enableColor(Processing::Blue);

    while (GetMessage(&Message, nullptr, 0, 0) > 0) {
        if (!TranslateAccelerator(gDisplayWindow, // Handle to receiving window
                                  hAccel, // Handle to active accelerator table
                                  &Message)) // Message data
        {
            // If a message was waiting in the message queue, process it
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
    }

    DestroyIcon(gKinectON);
    DestroyIcon(gKinectOFF);

    UnregisterClass(mainClassName, Instance);
    UnregisterClass("monitorButton", Instance);
    TestScreen::unregisterClass();

    return Message.wParam;
}

template <class T>
std::string to_string(T val) {
    std::stringstream ss;
    ss << val;
    return ss.str();
}

LRESULT CALLBACK MainProc(HWND handle, UINT message, WPARAM wParam,
                          LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        HWND recalibButton = CreateWindowEx(0,
                "BUTTON",
                "Recalibrate",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                9,
                ImageVars::height - 9 - 28,
                100,
                28,
                handle,
                reinterpret_cast<HMENU>(IDC_RECALIBRATE_BUTTON),
                gInstance,
                nullptr);

        SendMessage(recalibButton,
                WM_SETFONT,
                reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
                MAKELPARAM(FALSE, 0));


        HWND toggleStreamButton = CreateWindowEx(0,
                "BUTTON",
                "Start",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                ImageVars::width - 9 - 100,
                ImageVars::height - 9 - 28,
                100,
                28,
                handle,
                reinterpret_cast<HMENU>(IDC_STREAMTOGGLE_BUTTON),
                gInstance,
                nullptr);

        SendMessage(toggleStreamButton,
                WM_SETFONT,
                reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),
                MAKELPARAM(FALSE, 0));

        break;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);

        switch (wmId) {
            case IDC_RECALIBRATE_BUTTON: {
                // If there is no Kinect connected, don't bother trying to retrieve images
                if (gProjectorKnt.isVideoStreamRunning()) {
                    TestScreen testWin(gInstance, false);
                    testWin.create(gCurrentMonitor.dim);

                    testWin.setColor(Processing::Red);
                    testWin.display();
                    // Give Kinect time to get image w/ test pattern in it
                    std::this_thread::sleep_for(std::chrono::milliseconds(750));
                    gProjectorKnt.setCalibImage(Processing::Red);

                    testWin.setColor(Processing::Blue);
                    testWin.display();
                    // Give Kinect time to get image w/ test pattern in it
                    std::this_thread::sleep_for(std::chrono::milliseconds(750));
                    gProjectorKnt.setCalibImage(Processing::Blue);

                    gProjectorKnt.calibrate();
                }

                break;
            }

            case IDC_STREAMTOGGLE_BUTTON: {
                // If button was pressed from video display window
                if (handle == gProjectorKnt.getRegisteredVideoWindow()) {
                    if (gProjectorKnt.isVideoStreamRunning()) {
                        gProjectorKnt.stopVideoStream();
                    }
                    else {
                        gProjectorKnt.startVideoStream();
                    }
                }

                // If button was pressed from depth image display window
                else if (handle == gProjectorKnt.getRegisteredDepthWindow()) {
                    if (gProjectorKnt.isDepthStreamRunning()) {
                        gProjectorKnt.stopDepthStream();
                    }
                    else {
                        gProjectorKnt.startDepthStream();
                    }
                }

                break;
            }

            case IDM_STARTTRACK: {
                gProjectorKnt.setMouseTracking(true);
                MessageBox(handle, "Mouse tracking has been enabled",
                           "Mouse Tracking", MB_ICONINFORMATION | MB_OK);

                break;
            }

            case IDM_STOPTRACK: {
                gProjectorKnt.setMouseTracking(false);
                MessageBox(handle, "Mouse tracking has been disabled",
                           "Mouse Tracking", MB_ICONINFORMATION | MB_OK);

                break;
            }

            case IDM_DISPLAYVIDEO: {
                // Check "Display Video" and uncheck "Display Depth"
                CheckMenuItem(gMainMenu, IDM_DISPLAYVIDEO,
                              MF_BYCOMMAND | MF_CHECKED);
                CheckMenuItem(gMainMenu, IDM_DISPLAYDEPTH,
                              MF_BYCOMMAND | MF_UNCHECKED);

                // Stop depth stream from displaying and switch to video stream
                gProjectorKnt.registerVideoWindow(gDisplayWindow);
                gProjectorKnt.unregisterDepthWindow();

                /* Change button text to correctly represent status of stream
                 * displayed
                 */
                char windowText[16];
                if (gProjectorKnt.isVideoStreamRunning()) {
                    // Change Start/Stop button text to "Stop"
                    EnumChildWindows(handle, StartStreamChildCbk,
                                     reinterpret_cast<LPARAM>(windowText));
                }
                else {
                    // Change Start/Stop button text to "Start"
                    EnumChildWindows(handle, StopStreamChildCbk,
                                     reinterpret_cast<LPARAM>(windowText));
                }

                break;
            }

            case IDM_DISPLAYDEPTH: {
                // Uncheck "Display Video" and check "Display Depth"
                CheckMenuItem(gMainMenu, IDM_DISPLAYVIDEO,
                              MF_BYCOMMAND | MF_UNCHECKED);
                CheckMenuItem(gMainMenu, IDM_DISPLAYDEPTH,
                              MF_BYCOMMAND | MF_CHECKED);

                // Stop video stream from displaying and switch to depth stream
                gProjectorKnt.unregisterVideoWindow();
                gProjectorKnt.registerDepthWindow(gDisplayWindow);

                /* Change button text to correctly represent status of stream
                 * displayed
                 */
                char windowText[16];
                if (gProjectorKnt.isDepthStreamRunning()) {
                    // Change Start/Stop button text to "Stop"
                    EnumChildWindows(handle, StartStreamChildCbk,
                                     reinterpret_cast<LPARAM>(windowText));
                }
                else {
                    // Change Start/Stop button text to "Start"
                    EnumChildWindows(handle, StopStreamChildCbk,
                                     reinterpret_cast<LPARAM>(windowText));
                }

                break;
            }

            case IDM_CHANGEMONITOR: {
                DialogBox(gInstance, MAKEINTRESOURCE(IDD_MONITORBOX), handle,
                          MonitorCbk);

                break;
            }

            case IDM_HELP: {
                HINSTANCE status = ShellExecute(handle, "open", "help.html",
                                                nullptr, "help", SW_SHOWNORMAL);

                if (reinterpret_cast<int>(status) <= 32) {
                    std::string errorMsg = "Could not open help file\nError: ";
                    MessageBox(handle, (errorMsg + to_string(status)).c_str(),
                               "Error", MB_ICONERROR | MB_OK);
                }
                // TODO: Finish writing HTML help page

                break;
            }

            case IDM_ABOUT: {
                DialogBox(gInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), handle,
                          AboutCbk);

                break;
            }
        }

        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc;

        hdc = BeginPaint(handle, &ps);

        // If we're painting the video display window
        if (handle == gProjectorKnt.getRegisteredVideoWindow()) {
            gProjectorKnt.displayVideo(gDisplayWindow, 0, 0, hdc);
        }

        // If we're painting the depth image display window
        else if (handle == gProjectorKnt.getRegisteredDepthWindow()) {
            gProjectorKnt.displayDepth(gDisplayWindow, 0, 0, hdc);
        }

        EndPaint(handle, &ps);

        break;
    }

    case WM_DESTROY: {
        // If the display window is being closed, exit the application
        if (handle == gDisplayWindow) {
            PostQuitMessage(0);
        }

        break;
    }

    case WM_KINECT_VIDEOSTART: {
        // Change video window icon to green because the stream started
        PostMessage(handle, WM_SETICON, ICON_SMALL,
                    reinterpret_cast<LPARAM>(gKinectON));
        PostMessage(handle, WM_SETICON, ICON_BIG,
                    reinterpret_cast<LPARAM>(gKinectON));

        // Change Start/Stop button text to "Stop"
        char windowText[16];
        EnumChildWindows(handle, StartStreamChildCbk,
                         reinterpret_cast<LPARAM>(windowText));

        break;
    }

    case WM_KINECT_VIDEOSTOP: {
        // Change video window icon to red because the stream stopped
        PostMessage(handle, WM_SETICON, ICON_SMALL,
                    reinterpret_cast<LPARAM>(gKinectOFF));
        PostMessage(handle, WM_SETICON, ICON_BIG,
                    reinterpret_cast<LPARAM>(gKinectOFF));

        // Change Start/Stop button text to "Start"
        char windowText[16];
        EnumChildWindows(handle, StopStreamChildCbk,
                         reinterpret_cast<LPARAM>(windowText));

        break;
    }

    case WM_KINECT_DEPTHSTART: {
        // Change depth window icon to green because the stream started
        PostMessage(handle, WM_SETICON, ICON_SMALL,
                    reinterpret_cast<LPARAM>(gKinectON));
        PostMessage(handle, WM_SETICON, ICON_BIG,
                    reinterpret_cast<LPARAM>(gKinectON));

        // Change Start/Stop button text to "Stop"
        char windowText[16];
        EnumChildWindows(handle, StartStreamChildCbk,
                         reinterpret_cast<LPARAM>(windowText));

        break;
    }

    case WM_KINECT_DEPTHSTOP: {
        // Change depth window icon to red because the stream stopped
        PostMessage(handle, WM_SETICON, ICON_SMALL,
                    reinterpret_cast<LPARAM>(gKinectOFF));
        PostMessage(handle, WM_SETICON, ICON_BIG,
                    reinterpret_cast<LPARAM>(gKinectOFF));

        // Change Start/Stop button text to "Start"
        char windowText[16];
        EnumChildWindows(handle, StopStreamChildCbk,
                         reinterpret_cast<LPARAM>(windowText));

        break;
    }

    default: {
        return DefWindowProc(handle, message, wParam, lParam);
    }
    }

    return 0;
}

// Message handler for "Change Monitor" box
BOOL CALLBACK MonitorCbk(HWND hDlg, UINT message, WPARAM wParam,
                         LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        RECT windowSize;
        GetClientRect(hDlg, &windowSize);

        // Store button box's width and height
        unsigned int boxWidth = windowSize.right - windowSize.left - 18;
        unsigned int boxHeight = 89 - 9;

        // All buttons are scaled to fit within this window
        HWND buttonBox = CreateWindowEx(0,
                                        "STATIC",
                                        "",
                                        WS_VISIBLE | WS_CHILD,
                                        9,
                                        9,
                                        boxWidth,
                                        boxHeight,
                                        hDlg,
                                        reinterpret_cast<HMENU>(NULL),
                                        gInstance,
                                        nullptr);

        EnumDisplayMonitors(nullptr, // List all monitors
                            nullptr, // Don't clip area
                            MonitorEnumProc,
                            reinterpret_cast<LPARAM>(&gMonitors) // User data
        );

        /* Find coordinates of box that will fit all current desktop monitors
         * * Starts within 0 x 0 rectangle and expands it as necessary
         * * Then creates buttons within based upon that rectangle and matches
         *   them with their corresponding monitor
         */
        RECT desktopDims = {0, 0, 0, 0};

        for (auto& monitor : gMonitors) {
            if (monitor.dim.left < desktopDims.left) {
                desktopDims.left = monitor.dim.left;
            }

            if (monitor.dim.right > desktopDims.right) {
                desktopDims.right = monitor.dim.right;
            }

            if (monitor.dim.top < desktopDims.top) {
                desktopDims.top = monitor.dim.top;
            }

            if (monitor.dim.bottom > desktopDims.bottom) {
                desktopDims.bottom = monitor.dim.bottom;
            }
        }

        // Store desktop width and height
        unsigned int desktopWidth = desktopDims.right - desktopDims.left;
        unsigned int desktopHeight = desktopDims.bottom - desktopDims.top;

        char buttonText[2];
        bool isButtonClicked = false;
        // Create a button that will represent the monitor in this dialog
        for (auto& monitor : gMonitors) {
            isButtonClicked = gCurrentMonitor.dim.left == monitor.dim.left &&
                              gCurrentMonitor.dim.right == monitor.dim.right &&
                              gCurrentMonitor.dim.top == monitor.dim.top &&
                              gCurrentMonitor.dim.bottom == monitor.dim.bottom;

            if (isButtonClicked) {
                std::strcpy(buttonText, "*");
            }
            else {
                std::strcpy(buttonText, " ");
            }

            monitor.activeButton = CreateWindowEx(0,
                "BUTTON",
                (buttonText + to_string(monitor.dim.right - monitor.dim.left) +
                        " x " + to_string(monitor.dim.bottom - monitor.dim.top) +
                        " ").c_str(),
                WS_VISIBLE | WS_CHILD,
                boxWidth * (monitor.dim.left - desktopDims.left) / desktopWidth,
                boxHeight * (monitor.dim.top - desktopDims.top) / desktopHeight,
                boxWidth * (monitor.dim.right - monitor.dim.left) / desktopWidth,
                boxHeight * (monitor.dim.bottom - monitor.dim.top) / desktopHeight,
                buttonBox,
                reinterpret_cast<HMENU>(NULL),
                gInstance,
                nullptr);

            if (isButtonClicked) {
                gCurrentMonitor = monitor;
            }
        }

        return TRUE;
    }

    case WM_PARENTNOTIFY: {
        if (LOWORD(wParam) == WM_LBUTTONDOWN) {
            /* This message is only received when a button other than OK was
             * pressed in the dialog window
             */

            /* Convert cursor coordinates from dialog to desktop,
             * since the button's position is also in desktop coordinates
             */
            POINT cursorPos = {LOWORD(lParam), HIWORD(lParam)};
            MapWindowPoints(hDlg, nullptr, &cursorPos, 1);

            RECT buttonPos;

            // Change where test pattern will be drawn if button was clicked on
            for (auto& monitor : gMonitors) {
                GetWindowRect(monitor.activeButton, &buttonPos);

                // If cursor is within boundaries of button
                if (cursorPos.x > buttonPos.left &&
                    cursorPos.x < buttonPos.right &&
                    cursorPos.y > buttonPos.top &&
                    cursorPos.y < buttonPos.bottom) {
                    // Remove asterisk from previous button's text
                    SetWindowText(gCurrentMonitor.activeButton,
                            (" " + to_string(gCurrentMonitor.dim.right - gCurrentMonitor.dim.left) +
                             " x " + to_string(gCurrentMonitor.dim.bottom - gCurrentMonitor.dim.top) +
                             " ").c_str());

                    // Set new selected button
                    gCurrentMonitor = monitor;

                    // Add asterisk to new button's text
                    SetWindowText(gCurrentMonitor.activeButton,
                            ("*" + to_string(gCurrentMonitor.dim.right - gCurrentMonitor.dim.left) +
                             " x " + to_string(gCurrentMonitor.dim.bottom - gCurrentMonitor.dim.top) +
                             " ").c_str());
                }
            }
        }

        break;
    }

    case WM_COMMAND: {
        if (LOWORD(wParam) == IDOK) {
            for (auto& monitor : gMonitors) {
                DestroyWindow(monitor.activeButton);
            }
            gMonitors.clear();

            gCurrentMonitor.activeButton = nullptr;

            // Give Kinect correct monitor dimensions so mouse is moved to proper position
            gProjectorKnt.setScreenRect(gCurrentMonitor.dim);

            EndDialog(hDlg, LOWORD(wParam));
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg , LOWORD(wParam));
        }

        break;
    }

    case WM_CLOSE: {
        EndDialog(hDlg, 0);

        break;
    }

    default: {
        return FALSE;
    }
    }

    return TRUE;
}

// Message handler for "About" box
BOOL CALLBACK AboutCbk(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG: {
        return TRUE;
    }

    case WM_COMMAND: {
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        break;
    }
    }

    return FALSE;
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                              LPRECT lprcMonitor, LPARAM dwData) {
    auto& monitors = *reinterpret_cast<std::list<MonitorIndex>*>(dwData);
    monitors.push_back({{lprcMonitor->left, lprcMonitor->top,
                         lprcMonitor->right, lprcMonitor->bottom}, nullptr});

    return TRUE;
}

BOOL CALLBACK StartStreamChildCbk(HWND hwndChild, LPARAM lParam) {
    char* windowText = reinterpret_cast<char*>(lParam);

    if (GetWindowText(hwndChild, windowText, 16)) {
        if (std::strcmp(windowText, "Start") == 0) {
            SetWindowText(hwndChild, "Stop");
            return FALSE;
        }
    }

    return TRUE;
}

BOOL CALLBACK StopStreamChildCbk(HWND hwndChild, LPARAM lParam) {
    char* windowText = reinterpret_cast<char*>(lParam);

    if (GetWindowText(hwndChild, windowText, 16)) {
        if (std::strcmp(windowText, "Stop") == 0) {
            SetWindowText(hwndChild, "Start");
            return FALSE;
        }
    }

    return TRUE;
}
