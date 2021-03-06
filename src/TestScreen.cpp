//=============================================================================
//File Name: TestScreen.cpp
//Description: Displays a fullscreen test pattern of a given color
//Author: Tyler Veness
//=============================================================================

#include "TestScreen.hpp"
#include "ImageVars.hpp"

const char* TestScreen::m_windowClassName = "TestScreen";

WNDCLASSEX TestScreen::m_windowClass;
HINSTANCE TestScreen::m_instance;
HBRUSH TestScreen::m_whiteBrush;
HBRUSH TestScreen::m_colorBrush;
bool TestScreen::m_classInitialized = false;

TestScreen::TestScreen(HINSTANCE instance, bool createNow) {
    if (!m_classInitialized) {
        if (instance != nullptr) {
            m_instance = instance;
        }
        else {
            m_instance = GetModuleHandle(nullptr);
        }

        m_whiteBrush = CreateSolidBrush(RGB(128, 128, 128));
        m_colorBrush = CreateSolidBrush(RGB(255, 0, 0));

        ZeroMemory(&m_windowClass, sizeof(WNDCLASSEX));
        m_windowClass.cbSize        = sizeof(WNDCLASSEX);
        m_windowClass.style         = 0;
        m_windowClass.lpfnWndProc   = &TestScreen::OnEvent;
        m_windowClass.cbClsExtra    = 0;
        m_windowClass.cbWndExtra    = 0;
        m_windowClass.hInstance     = m_instance;
        m_windowClass.hIcon         = nullptr;
        m_windowClass.hCursor       = nullptr;
        m_windowClass.hbrBackground = m_whiteBrush;
        m_windowClass.lpszMenuName  = nullptr;
        m_windowClass.lpszClassName = m_windowClassName;
        m_windowClass.hIconSm       = nullptr;
        RegisterClassEx(&m_windowClass);

        m_classInitialized = true;
    }

    // If caller intended to create the window upon construction
    if (createNow) {
        create();
    }
}

TestScreen::~TestScreen() {
    TestScreen::close();
}

void TestScreen::create(const RECT windowPos) {
    // If window is closed
    if (m_window == nullptr) {
        m_window = CreateWindowEx(0,
                                  m_windowClassName,
                                  "",
                                  WS_POPUP | WS_VISIBLE,
                                  windowPos.left,
                                  windowPos.top,
                                  windowPos.right - windowPos.left,
                                  windowPos.bottom - windowPos.top,
                                  nullptr,
                                  nullptr,
                                  m_instance,
                                  nullptr);

        if (m_window != nullptr) {
            // Backup cursor before removing it
            m_prevCursor = SetCursor(nullptr);
        }
    }
}

void TestScreen::setPosition(const RECT windowPos) {
    SetWindowPos(m_window, nullptr, windowPos.left, windowPos.top,
                 windowPos.right - windowPos.left,
                 windowPos.bottom - windowPos.top,
                 SWP_NOSIZE | SWP_NOZORDER);
}

void TestScreen::setColor(ProcColor borderColor) {
    // Remove old color (make it black)
    if (m_borderColor == Red) {
        m_outlineColor.r = 0;
    }
    else if (m_borderColor == Green) {
        m_outlineColor.g = 0;
    }
    else if (m_borderColor == Blue) {
        m_outlineColor.b = 0;
    }

    // Set color indicator
    m_borderColor = borderColor;

    /* Create new color
     * Processing::Black changes nothing here, so the color remains black
     */
    if (m_borderColor == Red) {
        m_outlineColor.r = 255;
    }
    else if (m_borderColor == Green) {
        m_outlineColor.g = 255;
    }
    else if (m_borderColor == Blue) {
        m_outlineColor.b = 255;
    }

    DeleteObject(m_colorBrush);
    m_colorBrush = CreateSolidBrush(RGB(m_outlineColor.r, m_outlineColor.g,
                                        m_outlineColor.b));
}

void TestScreen::display() {
    if (m_window != nullptr) {
        InvalidateRect(m_window, nullptr, TRUE);
        UpdateWindow(m_window);
    }
}

void TestScreen::close() {
    if (m_window != nullptr) {
        DestroyWindow(m_window);
        m_window = nullptr;

        // Restore the cursor from before this window was created
        SetCursor(m_prevCursor);
    }
}

bool TestScreen::unregisterClass() {
    BOOL unregistered = UnregisterClass(m_windowClassName, m_instance);

    if (unregistered) {
        DeleteObject(m_whiteBrush);
        DeleteObject(m_colorBrush);
    }

    return unregistered;
}

LRESULT CALLBACK TestScreen::OnEvent(HWND Handle, UINT Message, WPARAM WParam,
                                     LPARAM LParam) {
    switch (Message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC windowHdc = BeginPaint(Handle, &ps);

        RECT windowSize;
        GetClientRect(Handle, &windowSize);

        POINT rectanglePts[4];
        HRGN rectRgn;

        /* ===== Draw Pattern ===== */
        // Draw colored border
        rectanglePts[0] = {0, 0};
        rectanglePts[1] = {windowSize.right, 0};
        rectanglePts[2] = {windowSize.right, windowSize.bottom};
        rectanglePts[3] = {0, windowSize.bottom};

        rectRgn = CreatePolygonRgn(rectanglePts, 4, ALTERNATE);
        FillRgn(windowHdc, rectRgn, m_colorBrush);
        DeleteObject(rectRgn);

        // Fill inside with black
        rectanglePts[0] = {20, 20};
        rectanglePts[1] = {windowSize.right - 20, 20};
        rectanglePts[2] = {windowSize.right - 20, windowSize.bottom - 20};
        rectanglePts[3] = {20, windowSize.bottom - 20};

        rectRgn = CreatePolygonRgn(rectanglePts, 4, ALTERNATE);
        FillRgn(windowHdc, rectRgn, m_whiteBrush);
        DeleteObject(rectRgn);
        /* ======================== */

        EndPaint(Handle, &ps);

        break;
    }

    default: {
        return DefWindowProc(Handle, Message, WParam, LParam);
    }
    }

    return 0;
}
