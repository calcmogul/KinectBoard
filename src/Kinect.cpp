//=============================================================================
//File Name: Kinect.cpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

#include <chrono>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "CKinect/parse.h"
#include "Kinect.hpp"
#include "HIDinput.h"
#include "SFML/Graphics/Color.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

sf::Color HSVtoRGB(unsigned short hue, unsigned short saturation, unsigned short value);

Kinect::Kinect() {
    rgb.newframe = newVideoFrame;
    rgb.callbackarg = this;

    depth.newframe = newDepthFrame;
    depth.callbackarg = this;

    m_imageSize = {static_cast<int>(ImageVars::width), static_cast<int>(ImageVars::height)};

    m_cvVidImage = cvCreateImage(m_imageSize, IPL_DEPTH_8U, 3);
    m_cvDepthImage = cvCreateImage(m_imageSize, IPL_DEPTH_8U, 4);
    m_cvBitmapDest = cvCreateImage(m_imageSize, IPL_DEPTH_8U, 4);

    // 3 bytes per pixel
    m_vidBuffer.resize(ImageVars::width * ImageVars::height * 3);

    // Each value in the depth image is 2 bytes long per pixel
    m_depthBuffer.resize(ImageVars::width * ImageVars::height * 2);

    for (unsigned int index = 0; index < ProcColor::Size; index++) {
        m_calibImages.push_back(new IplImage);
        m_calibImages.at(m_calibImages.size() - 1) = nullptr;
    }
}

Kinect::~Kinect() {
    threadrunning = false;
    stopVideoStream();
    stopDepthStream();

    /* join to the thread: maybe this should be in the stopstream functions?
       the entire idea that stopstream can be called both from inside and
       outside of the thread is a bit jacked up. I'll probably fix this soon */
    thread.join();

    // wait for Kinect thread to close before destroying instance
    while (threadrunning == true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    DeleteObject(m_vidImage);
    DeleteObject(m_depthImage);

    cvReleaseImage(&m_cvVidImage);
    cvReleaseImage(&m_cvDepthImage);
    cvReleaseImage(&m_cvBitmapDest);

    for (unsigned int index = m_calibImages.size(); index > 0; index--) {
        cvReleaseImage(&m_calibImages.at(index - 1));
        delete m_calibImages.at(index - 1);
    }

    std::free(m_quad);
}

void Kinect::startVideoStream() {
    // If rgb image stream is down, start it
    if (rgb.state == NSTREAM_DOWN) {
        if (startstream(rgb) == 0) {
            std::lock_guard<std::mutex> lock(m_vidWindowMutex);
            if (m_vidWindow != nullptr) {
                PostMessage(m_vidWindow, WM_KINECT_VIDEOSTART, 0, 0);
            }
        }
    }
}

void Kinect::startDepthStream() {
    // If depth image stream is down, start it
    if (depth.state == NSTREAM_DOWN) {
        if (startstream(depth) == 0) {
            std::lock_guard<std::mutex> lock(m_depthWindowMutex);
            if (m_depthWindow != nullptr) {
                PostMessage(m_depthWindow, WM_KINECT_DEPTHSTART, 0, 0);
            }
        }
    }
}

void Kinect::stopVideoStream() {
    auto oldState = NSTREAM_UP;

    // If this function was able to switch rgb's state from up to down
    if (rgb.state.compare_exchange_strong(oldState, NSTREAM_DOWN)) {
        // If other stream is down too, stop the thread
        if (depth.state == NSTREAM_DOWN) {
            threadrunning = false;
        }

        // Call the callback
        if (rgb.streamstopping != nullptr) {
            rgb.streamstopping(rgb, rgb.callbackarg);
        }
    }

    m_foundScreen = false;

    std::lock_guard<std::mutex> lock(m_vidWindowMutex);
    if (m_vidWindow != nullptr) {
        PostMessage(m_vidWindow, WM_KINECT_VIDEOSTOP, 0, 0);
    }
}

void Kinect::stopDepthStream() {
    auto oldState = NSTREAM_UP;

    // If this function was able to switch depth's state from up to down
    if (depth.state.compare_exchange_strong(oldState, NSTREAM_DOWN)) {
        // If other stream is down too, stop the thread
        if (rgb.state == NSTREAM_DOWN) {
            threadrunning = false;
        }

        // Call the callback
        if (depth.streamstopping != nullptr) {
            depth.streamstopping(depth, depth.callbackarg);
        }
    }

    std::lock_guard<std::mutex> lock(m_depthWindowMutex);
    if (m_depthWindow != nullptr) {
        PostMessage(m_depthWindow, WM_KINECT_DEPTHSTOP, 0, 0);
    }
}

bool Kinect::isVideoStreamRunning() {
    return rgb.state == NSTREAM_UP;
}

bool Kinect::isDepthStreamRunning() {
    return depth.state == NSTREAM_UP;
}

void Kinect::setVideoStreamFPS(unsigned int fps) {
    m_vidFrameRate = fps;
}

void Kinect::setDepthStreamFPS(unsigned int fps) {
    m_depthFrameRate = fps;
}

void Kinect::registerVideoWindow(HWND window) {
    std::lock_guard<std::mutex> lock(m_vidWindowMutex);
    m_vidWindow = window;
}

void Kinect::registerDepthWindow(HWND window) {
    std::lock_guard<std::mutex> lock(m_depthWindowMutex);
    m_depthWindow = window;
}

void Kinect::unregisterVideoWindow() {
    std::lock_guard<std::mutex> lock(m_vidWindowMutex);
    m_vidWindow = nullptr;
}

void Kinect::unregisterDepthWindow() {
    std::lock_guard<std::mutex> lock(m_depthWindowMutex);
    m_depthWindow = nullptr;
}

const HWND Kinect::getRegisteredVideoWindow() {
    std::lock_guard<std::mutex> lock(m_depthWindowMutex);
    return m_depthWindow;
}

const HWND Kinect::getRegisteredDepthWindow() {
    std::lock_guard<std::mutex> lock(m_vidWindowMutex);
    return m_vidWindow;
}

void Kinect::displayVideo(HWND window, int x, int y, HDC deviceContext) {
    display(window, x, y, m_vidImage, m_vidDisplayMutex, deviceContext);
}

void Kinect::displayDepth(HWND window, int x, int y, HDC deviceContext) {
    display(window, x, y, m_depthImage, m_depthDisplayMutex, deviceContext);
}

bool Kinect::saveVideo(const std::string& fileName) {
    cv::Mat img(ImageVars::height, ImageVars::width, CV_8UC(3), &m_vidBuffer[0]);
    return cv::imwrite(fileName, img);
}

bool Kinect::saveDepth(const std::string& fileName) {
    cv::Mat img(ImageVars::height, ImageVars::width, CV_8UC(3), m_cvDepthImage);
    return cv::imwrite(fileName, img);
}

void Kinect::setCalibImage(Processing::ProcColor colorWanted) {
    if (isVideoStreamRunning()) {
        std::lock_guard<std::mutex> lock(m_vidImageMutex);
        std::memcpy(m_calibImages[colorWanted]->imageData, &m_vidBuffer[0],
                    ImageVars::width * ImageVars::height * 3);
    }
}

void Kinect::calibrate() {
    IplImage* redCalib = nullptr;
    IplImage* greenCalib = nullptr;
    IplImage* blueCalib = nullptr;

    if (isEnabled(Red)) {
        redCalib = m_calibImages[Red];
    }

    if (isEnabled(Green)) {
        greenCalib = m_calibImages[Green];
    }

    if (isEnabled(Blue)) {
        blueCalib = m_calibImages[Blue];
    }

    std::free(m_quad);
    m_plistRaw.clear();
    m_plistProc.clear();

    // If image was disabled, nullptr is passed instead, so it's ignored

    /* Use the calibration images to locate a quadrilateral in the
     * image which represents the screen (returns 1 on failure)
     */
    //saveRGBimage(redCalib, (char *)"redCalib-start.data"); // TODO
    //saveRGBimage(blueCalib, (char *)"blueCalib-start.data"); // TODO
    findScreenBox(redCalib, greenCalib, blueCalib, &m_quad);

    // If no box was found, m_quad will be nullptr
    m_foundScreen = (m_quad != nullptr);
}

void Kinect::lookForCursors() {
    // We can't look for cursors if we never found a screen on which to look
    if (m_foundScreen) {
        {
            std::lock_guard<std::mutex> lock(m_vidImageMutex);

            /* Create a list of points which represent potential locations
               of the pointer */
            IplImage* tempImage = RGBtoIplImage(&m_vidBuffer[0],
                                                ImageVars::width,
                                                ImageVars::height);
            m_plistRaw = findImageLocation(tempImage, FLT_RED);
            cvReleaseImage(&tempImage);
        }

        /* Identify the points in m_plistRaw which are located inside the
         * boundary defined by m_quad, and scale them to the size of the
         * computer's main screen. These are mouse pointer candidates.
         */
        if (!m_plistRaw.empty() && m_moveMouse) {
            m_plistProc = findScreenLocation(m_plistRaw, m_quad, m_screenRect.right - m_screenRect.left, m_screenRect.bottom - m_screenRect.top);

            if (!m_plistProc.empty()) {
                auto& point = m_plistProc.front();
                moveMouse(&m_input,
                          65535.f * (m_screenRect.left + point.x) / (m_screenRect.right - m_screenRect.left),
                          65535.f * (m_screenRect.top + point.y) / (m_screenRect.bottom - m_screenRect.top),
                          MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE );
            }
        }
    }
}

void Kinect::setMouseTracking(bool on) {
    m_moveMouse = on;
}

void Kinect::enableColor(ProcColor color) {
    if (!isEnabled(color)) {
        m_enabledColors |= (1 << color);
        m_calibImages[color] = cvCreateImage(m_imageSize, IPL_DEPTH_8U, 3);
    }
}

void Kinect::disableColor(ProcColor color) {
    if (isEnabled(color)) {
        m_enabledColors &= ~(1 << color);
        cvReleaseImage(&m_calibImages[color]);
        m_calibImages[color] = nullptr;
    }
}

bool Kinect::isEnabled(ProcColor color) const {
    return m_enabledColors & (1 << color);
}

void Kinect::setScreenRect(RECT screenRect) {
    m_screenRect = screenRect;
}

void Kinect::newVideoFrame(nstream<Kinect>& streamObject, void* classObject) {
    Kinect* kntPtr = reinterpret_cast<Kinect*>(classObject);

    kntPtr->m_vidImageMutex.lock();

    // Copy image to internal buffer (3 channels)
    std::memcpy(&kntPtr->m_vidBuffer[0], kntPtr->rgb.buf,
                ImageVars::width * ImageVars::height * 3);

    kntPtr->m_vidDisplayMutex.lock();

    DeleteObject(kntPtr->m_vidImage); // free previous image if there is one

    //                            B ,   G ,   R ,   A
    CvScalar lineColor = cvScalar(0x00, 0xFF, 0x00, 0xFF);

    kntPtr->m_cvVidImage = RGBtoIplImage(&kntPtr->m_vidBuffer[0],
                                            ImageVars::width, ImageVars::height);

    if (kntPtr->m_foundScreen) {
        // Draw lines to show user where the screen is
        cvLine(kntPtr->m_cvVidImage, kntPtr->m_quad->point[0],
               kntPtr->m_quad->point[1], lineColor, 2, 8, 0);
        cvLine(kntPtr->m_cvVidImage, kntPtr->m_quad->point[1],
               kntPtr->m_quad->point[2], lineColor, 2, 8, 0);
        cvLine(kntPtr->m_cvVidImage, kntPtr->m_quad->point[2],
               kntPtr->m_quad->point[3], lineColor, 2, 8, 0);
        cvLine(kntPtr->m_cvVidImage, kntPtr->m_quad->point[3],
               kntPtr->m_quad->point[0], lineColor, 2, 8, 0);

        kntPtr->lookForCursors();
    }

    // Perform conversion from RGBA to BGRA for use as image data in CreateBitmap
    cvCvtColor(kntPtr->m_cvVidImage, kntPtr->m_cvBitmapDest, CV_RGB2BGRA);

    kntPtr->m_vidImage = CreateBitmap(ImageVars::width, ImageVars::height,
                                         1, 32 , kntPtr->m_cvBitmapDest->imageData);

    cvReleaseImage(&kntPtr->m_cvVidImage);

    kntPtr->m_vidDisplayMutex.unlock();

    kntPtr->m_vidImageMutex.unlock();

    // Limit video frame rate
    using namespace std::chrono;
    if (1.f / duration_cast<seconds>(system_clock::now() - kntPtr->m_lastVidFrameTime).count() < kntPtr->m_vidFrameRate) {
        {
            std::lock_guard<std::mutex> lock(kntPtr->m_vidWindowMutex);
            if (kntPtr->m_vidWindow != nullptr) {
                kntPtr->displayVideo(kntPtr->m_vidWindow, 0, 0);
            }
        }

        kntPtr->m_lastVidFrameTime = system_clock::now();
    }
}

void Kinect::newDepthFrame(nstream<Kinect>& streamObject, void* classObject) {
    Kinect* kntPtr = reinterpret_cast<Kinect*>(classObject);

    kntPtr->m_depthImageMutex.lock();

    // Copy image to internal buffer (2 bytes per pixel)
    std::memcpy(&kntPtr->m_depthBuffer[0], kntPtr->depth.buf,
                ImageVars::width * ImageVars::height * 2);

    double depth = 0.0;
    unsigned short depthVal = 0;
    for (unsigned int index = 0; index < ImageVars::width * ImageVars::height; index++) {
        std::memcpy(&depthVal, &kntPtr->m_depthBuffer[2 * index], sizeof(unsigned short));

        depth = Kinect::rawDepthToMeters(depthVal);

        sf::Color color = HSVtoRGB(360 * depth / 5.f, 100, 100);

        // Assign values from 0 to 5 meters with a shade from black to white
        kntPtr->m_cvDepthImage->imageData[4 * index + 0] = color.b;
        kntPtr->m_cvDepthImage->imageData[4 * index + 1] = color.g;
        kntPtr->m_cvDepthImage->imageData[4 * index + 2] = color.r;
    }

    // Make HBITMAP from pixel array
    kntPtr->m_depthDisplayMutex.lock();

    //                            B ,   G ,   R ,   A
    CvScalar lineColor = cvScalar(0x00, 0xFF, 0x00, 0xFF);

    if (kntPtr->m_foundScreen) {
        // Draw lines to show user where the screen is
        cvLine(kntPtr->m_cvDepthImage, kntPtr->m_quad->point[0],
               kntPtr->m_quad->point[1], lineColor, 2, 8, 0);
        cvLine(kntPtr->m_cvDepthImage, kntPtr->m_quad->point[1],
               kntPtr->m_quad->point[2], lineColor, 2, 8, 0);
        cvLine(kntPtr->m_cvDepthImage, kntPtr->m_quad->point[2],
               kntPtr->m_quad->point[3], lineColor, 2, 8, 0);
        cvLine(kntPtr->m_cvDepthImage, kntPtr->m_quad->point[3],
               kntPtr->m_quad->point[0], lineColor, 2, 8, 0);
    }

    DeleteObject(kntPtr->m_depthImage); // free previous image if there is one
    kntPtr->m_depthImage = CreateBitmap(ImageVars::width, ImageVars::height,
                                           1, 32, kntPtr->m_cvDepthImage->imageData);

    kntPtr->m_depthDisplayMutex.unlock();

    kntPtr->m_depthImageMutex.unlock();

    // Limit depth image stream frame rate
    using namespace std::chrono;
    if (1.f / duration_cast<seconds>(system_clock::now() - kntPtr->m_lastDepthFrameTime).count() < kntPtr->m_depthFrameRate) {
        {
            std::lock_guard<std::mutex> lock(kntPtr->m_depthWindowMutex);
            if (kntPtr->m_depthWindow != nullptr) {
                kntPtr->displayDepth(kntPtr->m_depthWindow, 0, 0);
            }
        }

        kntPtr->m_lastDepthFrameTime = system_clock::now();
    }
}

void Kinect::display(HWND window, int x, int y, HBITMAP image, std::mutex& displayMutex, HDC deviceContext) {
    std::lock_guard<std::mutex> lock(displayMutex);

    if (image != nullptr) {
        // Create offscreen DC for image to go on
        HDC imageHdc = CreateCompatibleDC(nullptr);

        // Put the image into the offscreen DC and save the old one
        HBITMAP imageBackup = static_cast<HBITMAP>(SelectObject(imageHdc, image));

        // Stores DC of window to which to draw
        HDC windowHdc = deviceContext;

        // Stores whether or not the window's HDC was provided for us
        bool neededHdc = (deviceContext == nullptr);

        // If we don't have the window's device context yet
        if (neededHdc) {
            windowHdc = GetDC(window);
        }

        // Load image to real BITMAP just to retrieve its dimensions
        BITMAP tempBMP;
        GetObject(image, sizeof(BITMAP), &tempBMP);

        // Copy image from offscreen DC to window's DC
        BitBlt(windowHdc, x, y, tempBMP.bmWidth, tempBMP.bmHeight, imageHdc, 0, 0, SRCCOPY);

        if (neededHdc) {
            // Release window's HDC if we needed to get one earlier
            ReleaseDC(window, windowHdc);
        }

        // Restore old image
        SelectObject(imageHdc, imageBackup);

        // Delete offscreen DC
        DeleteDC(imageHdc);
    }
}

char* Kinect::RGBtoBITMAPdata(const char* imageData, unsigned int width, unsigned int height) {
    char* bitmapData = (char*) std::malloc(width * height * 4);

    /* ===== Convert RGB to BGRA before displaying the image ===== */
    /* Swap R and B because Win32 expects the color components in the
     * opposite order they are currently in
     */

    for (unsigned int startI = 0, endI = 0; endI < width * height * 4; startI += 3, endI += 4) {
        bitmapData[endI+0] = imageData[startI + 2];
        bitmapData[endI+1] = imageData[startI + 1];
        bitmapData[endI+2] = imageData[startI + 0];
    }
    /* =========================================================== */

    return bitmapData;
}

double Kinect::rawDepthToMeters(unsigned short depthValue) {
    if (depthValue < 2047) {
        return 1.f / (static_cast<double>(depthValue) * -0.0030711016 +
                      3.3309495161);
    }

    return 0.0;
}

sf::Color HSVtoRGB( unsigned short hue , unsigned short saturation , unsigned short value ) {
    /* H is [0,360]
     * S_HSV is [0,1]
     * V is [0,1]
     */

    sf::Color color(0, 0, 0);
    float C = value / 100 * saturation / 100;
    float H = hue / 60;
    float X = C * (1 - abs(static_cast<int>(floor(H)) % 2 - 1));

    if (0 <= H && H < 1) {
        color.r = 255 * C;
        color.g = 255 * X;
        color.b = 0;
    }
    else if (1 <= H && H < 2) {
        color.r = 255 * X;
        color.g = 255 * C;
        color.b = 0;
    }
    else if (2 <= H && H < 3) {
        color.r = 0;
        color.g = 255 * C;
        color.b = 255 * X;
    }
    else if (3 <= H && H < 4) {
        color.r = 0;
        color.g = 255 * X;
        color.b = 255 * C;
    }
    else if (4 <= H && H < 5) {
        color.r = 255 * X;
        color.g = 0;
        color.b = 255 * C;
    }
    else if (5 <= H && H < 6) {
        color.r = 255 * C;
        color.g = 0;
        color.b = 255 * X;
    }
    else {
        return color;
    }

    float m = value - C;

    color.r += m;
    color.g += m;
    color.b += m;

    return color;
}

/*
 * Callback called by libfreenect each time the buffer is filled with a
 * new RGB frame
 *
 * dev: filled with a pointer the the freenect device
 * rgb: pointer to the RGB buffer
 * timestamp: POSIX timestamp of the buffer
 *
 * not safe for multiple instances because nstm has to be global
 */
void Kinect::rgb_cb(freenect_device* dev, void* rgbBuf, uint32_t timestamp) {
    Kinect& kntPtr = *static_cast<Kinect*>(freenect_get_user(dev));

    /* Do nothing if the stream isn't up */
    if (kntPtr.rgb.state != NSTREAM_UP) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(kntPtr.rgb.mutex);

        /* Update timestamp: should we be using the provided one? */
        /* gettimeofday(&curtime, &thistimezone); nstm->timestamp = curtime.tv_sec;
         */
        kntPtr.rgb.timestamp = timestamp;

        /* Swap buffers */
        if (kntPtr.rgb.buf == kntPtr.rgb.buf0.get()) {
            kntPtr.rgb.buf = kntPtr.rgb.buf1.get();
            freenect_set_video_buffer(dev, kntPtr.rgb.buf0.get());
        }
        else {
            kntPtr.rgb.buf = kntPtr.rgb.buf0.get();
            freenect_set_video_buffer(dev, kntPtr.rgb.buf1.get());
        }
    }

    /* call the new frame callback */
    if (kntPtr.rgb.newframe != nullptr) {
        kntPtr.rgb.newframe(kntPtr.rgb, kntPtr.rgb.callbackarg);
    }
}

/*
 * Callback called by libfreenect each time the buffer is filled with a
 * new depth frame
 *
 * dev: filled with a pointer to the freenect device
 * rgb: pointer to the depth buffer
 * timestamp: POSIX timestamp of the buffer
 *
 * not safe for multiple instances because nstm has to be global
 */
void Kinect::depth_cb(freenect_device* dev, void* depthBuf, uint32_t timestamp) {
    Kinect& kntPtr = *static_cast<Kinect*>(freenect_get_user(dev));

    /* Do nothing if the stream isn't up */
    if (kntPtr.depth.state != NSTREAM_UP) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(kntPtr.depth.mutex);

        /* update timestamp: should we be using the provided one? */
        kntPtr.depth.timestamp = timestamp;

        /* Swap buffers */
        if (kntPtr.depth.buf == kntPtr.depth.buf0.get()) {
            kntPtr.depth.buf = kntPtr.depth.buf1.get();
            freenect_set_depth_buffer(dev, kntPtr.depth.buf0.get());
        }
        else {
            kntPtr.depth.buf = kntPtr.depth.buf0.get();
            freenect_set_depth_buffer(dev, kntPtr.depth.buf1.get());
        }
    }

    /* call the new frame callback */
    if (kntPtr.depth.newframe != nullptr) {
        kntPtr.depth.newframe(kntPtr.depth, kntPtr.depth.callbackarg);
    }
}

/*
 * User calls this to start an RGB or depth stream. Should be called
 * through the nstream_t struct
 *
 * stream: The nstream_t handle of the stream to start.
 */
int Kinect::startstream(nstream<Kinect>& stream) {
    /* You can't start a stream that's already started */
    if (stream.state != NSTREAM_DOWN)
        return 1;

    threadrunning_mutex.lock();
    if (!threadrunning) {
        /* The main worker thread isn't running yet. Start it. */
        threadrunning = true;
        thread = std::thread(&Kinect::threadmain, this);

        {
            std::unique_lock<std::mutex> lock(threadrunning_mutex);
            threadcond.wait(lock);
        }
        if (!threadrunning) {
            /* the kinect failed to initialize */
            threadrunning_mutex.unlock();
            return 1;
        }

    }
    threadrunning_mutex.unlock();

    stream.state = NSTREAM_UP;

    /* Do the callback */
    if (stream.streamstarting != nullptr) {
        stream.streamstarting(stream, stream.callbackarg);
    }

    return 0;
}

/*
 * User calls this to stop the RGB stream. Note that RGB and depth streams
 * have seperate stop functions, unlike start functions.
 *
 * stream: The nstream_t handle of the stream to stop.
 */
int Kinect::rgb_stopstream() {
    if (rgb.state != NSTREAM_UP)
        return 1;

    if (depth.state != NSTREAM_UP) {
        /* We're servicing no other streams, shut down the thread */
        threadrunning = false;
    }

    rgb.state = NSTREAM_DOWN;

    /* Do the callback */
    if (rgb.streamstopping != nullptr) {
        rgb.streamstopping(rgb, rgb.callbackarg);
    }

    return 0;
}

/*
 * User calls this to stop the depth stream. Note that depth and depth streams
 * have seperate stop functions, unlike start functions.
 *
 * stream: The nstream_t handle of the stream to stop
 */
int Kinect::depth_stopstream() {
    if (depth.state != NSTREAM_UP)
        return 1;

    if (rgb.state != NSTREAM_UP) {
        /* We're servicing no other streams, shut down the thread */
        threadrunning = false;
    }

    depth.state = NSTREAM_DOWN;

    /* Do the callback */
    if (depth.streamstopping != nullptr) {
        depth.streamstopping(depth, depth.callbackarg);
    }

    return 0;
}

/*
 * Main thread function, initializes the kinect, and runs the
 * libfreenect event loop.
 */
void Kinect::threadmain() {
    int error;
    int ndevs;
    freenect_context *f_ctx;
    freenect_device *f_dev;

    /* initialize libfreenect */
    error = freenect_init(&f_ctx, nullptr);
    if (error != 0) {
        fprintf(stderr, "failed to initialize libfreenect\n");
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    ndevs = freenect_num_devices(f_ctx);
    if (ndevs != 1) {
        fprintf(stderr, "more/less than one kinect detected\n");
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    error = freenect_open_device(f_ctx, &f_dev, 0);
    if (error != 0) {
        fprintf(stderr, "failed to open kinect device\n");
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    freenect_set_user(f_dev, this);
    freenect_set_video_callback(f_dev, rgb_cb);
    freenect_set_depth_callback(f_dev, depth_cb);

    error = freenect_set_video_mode(f_dev, freenect_find_video_mode(
                                           FREENECT_RESOLUTION_MEDIUM,
                                           FREENECT_VIDEO_RGB));
    if (error != 0) {
        fprintf(stderr, "failed to set video mode\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    error = freenect_set_depth_mode(f_dev, freenect_find_depth_mode(
                                           FREENECT_RESOLUTION_MEDIUM,
                                           FREENECT_DEPTH_11BIT));
    if (error != 0) {
        fprintf(stderr, "failed to set depth mode\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    /* Set the user buffer to buf0 */
    {
        std::lock_guard<std::mutex> lock(rgb.mutex);
        rgb.buf = rgb.buf0.get();
    }

    error = freenect_set_video_buffer(f_dev, rgb.buf1.get());
    if (error != 0) {
        fprintf(stderr, "failed to set video buffer\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    /* Set the user buffer to buf0 */
    {
        std::lock_guard<std::mutex> lock(depth.mutex);
        depth.buf = depth.buf0.get();
    }

    error = freenect_set_depth_buffer(f_dev, depth.buf1.get());
    if (error != 0) {
        fprintf(stderr, "failed to set depth buffer\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    error = freenect_start_video(f_dev);
    if (error != 0) {
        fprintf(stderr, "failed to start video stream\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    error = freenect_start_depth(f_dev);
    if (error != 0) {
        fprintf(stderr, "failed to start depth stream\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    // We initialized everything successfully
    threadcond.notify_all();

    /* Turn the LED red */
    freenect_set_led(f_dev, LED_RED);

    /* Do the main loop */
    while (freenect_process_events(f_ctx) >= 0 && threadrunning) {

    }

    /* Turn the LED blinking green */
    freenect_set_led(f_dev, LED_BLINK_GREEN);

    threadrunning = false;

    auto oldState = NSTREAM_UP;
    // If this function was able to switch rgb's state from up to down
    if (rgb.state.compare_exchange_strong(oldState, NSTREAM_DOWN)) {
        // Call the callback
        if (rgb.streamstopping != nullptr) {
            rgb.streamstopping(rgb, rgb.callbackarg);
        }
    }
    oldState = NSTREAM_UP;
    // If this function was able to switch depth's state from up to down
    if (depth.state.compare_exchange_strong(oldState, NSTREAM_DOWN)) {
        // Call the callback
        if (depth.streamstopping != nullptr) {
            depth.streamstopping(depth, depth.callbackarg);
        }
    }

    freenect_stop_video(f_dev);
    freenect_stop_depth(f_dev);

    freenect_close_device(f_dev);
    freenect_shutdown(f_ctx);
}
