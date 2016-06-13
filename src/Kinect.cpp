//=============================================================================
// Description: Manages interface with a Microsoft Kinect connected to the USB
//              port of the computer
//=============================================================================

#include "Kinect.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#include <QCursor>
#include <QImage>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

std::array<double, 2048> Kinect::m_depthLUT;
Freenect::Freenect Kinect::m_freenectInst;

Kinect::Kinect(freenect_context* context, int index) :
        Freenect::FreenectDevice(context, index) {
    for (uint32_t i = 0; i < 2048; i++) {
        m_depthLUT[i] = 1.f / (static_cast<double>(i) * -0.0030711016 +
                               3.3309495161);
    }

    m_imageSize = {static_cast<int>(ImageVars::width), static_cast<int>(ImageVars::height)};

    m_cvVidImage = cv::Mat(ImageVars::height, ImageVars::width, CV_8UC3);
    m_cvDepthImage = cv::Mat(ImageVars::height, ImageVars::width, CV_8UC3);
    m_cvBitmapDest = cv::Mat(ImageVars::height, ImageVars::width, CV_8UC4);

    for (uint32_t index = 0; index < ProcColor::Size; index++) {
        m_calibImages.emplace_back(ImageVars::height, ImageVars::width, CV_8UC3);
    }

    setLed(LED_BLINK_GREEN);
}

Kinect::~Kinect() {
    stop();
}

void Kinect::start() {
    startVideo();
    m_videoRunning = true;

    startDepth();
    m_depthRunning = true;

    setLed(LED_RED);
}

void Kinect::stop() {
    m_videoRunning = false;
    stopVideo();

    m_depthRunning = false;
    stopDepth();

    m_foundScreen = false;

    setLed(LED_BLINK_GREEN);
}

bool Kinect::isStreaming() const {
    return isVideoStreamRunning() || isDepthStreamRunning();
}

void Kinect::saveCurrentImage(const std::string& fileName) {
    std::lock_guard<std::mutex> lock(m_vidImageMutex);

    QImage tmp(&m_vidBuf[0], 640, 480, QImage::Format_RGB888);
    if (!tmp.save(fileName.c_str())) {
        std::cout << "Kinect: failed to save image to '" << fileName << "'\n";
    }
}

uint8_t* Kinect::getCurrentImage() {
    if (m_displayVid) {
        return m_cvVidImage.ptr<uint8_t>(0);
    }
    else {
        return m_cvDepthImage.ptr<uint8_t>(0);
    }
}

uint32_t Kinect::getCurrentWidth() const {
    return m_width;
}

uint32_t Kinect::getCurrentHeight() const {
    return m_height;
}

bool Kinect::isVideoStreamRunning() const {
    return m_videoRunning;
}

bool Kinect::isDepthStreamRunning() const {
    return m_depthRunning;
}

void Kinect::registerVideoWindow() {
    m_displayVid = true;
}

void Kinect::registerDepthWindow() {
    m_displayVid = false;
}

bool Kinect::saveVideo(const std::string& fileName) {
    cv::Mat img(ImageVars::height, ImageVars::width, CV_8UC(3),
                &m_vidBuf[0]);
    return cv::imwrite(fileName, img);
}

bool Kinect::saveDepth(const std::string& fileName) {
    return cv::imwrite(fileName, m_cvDepthImage);
}

void Kinect::setCalibImage(ProcColor colorWanted) {
    if (isVideoStreamRunning()) {
        std::lock_guard<std::mutex> lock(m_vidImageMutex);
        std::memcpy(m_calibImages[colorWanted].data, &m_vidBuf[0],
                    ImageVars::width * ImageVars::height * 3);
    }
}

void Kinect::calibrate() {
    cv::Mat* redCalib = nullptr;
    cv::Mat* greenCalib = nullptr;
    cv::Mat* blueCalib = nullptr;
    char procImages = 0;

    if (isEnabled(Red)) {
        redCalib = &m_calibImages[Red];
        procImages |= 1;
    }

    if (isEnabled(Green)) {
        greenCalib = &m_calibImages[Green];
        procImages |= (1 << 1);
    }

    if (isEnabled(Blue)) {
        blueCalib = &m_calibImages[Blue];
        procImages |= (1 << 2);
    }

    m_plistRaw.clear();
    m_plistProc.clear();

    // If image was disabled, nullptr is passed instead, so it's ignored

    /* Use the calibration images to locate a quadrilateral in the
     * image which represents the screen (returns 1 on failure)
     */
    m_quad = findScreenBox(*redCalib, *greenCalib, *blueCalib, procImages);

    // If no box was found, m_quad will be nullptr
    m_foundScreen = m_quad.validQuad;
}

void Kinect::lookForCursors() {
    // We can't look for cursors if we never found a screen on which to look
    if (m_foundScreen) {
        {
            std::lock_guard<std::mutex> lock(m_vidImageMutex);

            /* Create a list of points which represent potential locations
               of the pointer */
            cv::Mat tempImage(ImageVars::height, ImageVars::width, CV_8UC3,
                              &m_vidBuf[0]);
            m_plistRaw = findImageLocation(tempImage, FLT_RED);
        }

        /* Identify the points in m_plistRaw which are located inside the
         * boundary defined by m_quad, and scale them to the size of the
         * computer's main screen. These are mouse pointer candidates.
         */
        if (!m_plistRaw.empty() && m_moveMouse) {
            m_plistProc = findScreenLocation(m_plistRaw, m_quad,
                                             m_screenRect.right() -
                                                 m_screenRect.left(),
                                             m_screenRect.bottom() -
                                                 m_screenRect.top());

            if (!m_plistProc.empty()) {
                auto& point = m_plistProc.front();
                QCursor::setPos(m_screenRect.left() + point.x,
                                m_screenRect.top() + point.y);
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
        m_calibImages[color] = cv::Mat(m_imageSize, CV_8UC3);
    }
}

void Kinect::disableColor(ProcColor color) {
    if (isEnabled(color)) {
        m_enabledColors &= ~(1 << color);
    }
}

bool Kinect::isEnabled(ProcColor color) const {
    return m_enabledColors & (1 << color);
}

void Kinect::setScreenRect(const QRect& screenRect) {
    m_screenRect = screenRect;
}

void Kinect::newVideoFrame() {
    m_vidImageMutex.lock();

    m_vidDisplayMutex.lock();

    auto lineColor = cv::Scalar(0x00, 0xFF, 0x00, 0xFF);

    m_cvVidImage = cv::Mat(ImageVars::height, ImageVars::width, CV_8UC3,
                           &m_vidBuf[0]);

    if (m_foundScreen) {
        // Draw lines to show user where the screen is
        cv::line(m_cvVidImage, m_quad.point[0], m_quad.point[1], lineColor, 2);
        cv::line(m_cvVidImage, m_quad.point[1], m_quad.point[2], lineColor, 2);
        cv::line(m_cvVidImage, m_quad.point[2], m_quad.point[3], lineColor, 2);
        cv::line(m_cvVidImage, m_quad.point[3], m_quad.point[0], lineColor, 2);

        lookForCursors();
    }

    // Perform conversion from RGBA to BGRA for use as image data in CreateBitmap
    cv::cvtColor(m_cvVidImage, m_cvBitmapDest, CV_RGB2BGRA);

    m_vidDisplayMutex.unlock();

    m_vidImageMutex.unlock();

    callNewImage(nullptr, 0);
}

void Kinect::newDepthFrame() {
    m_depthImageMutex.lock();

    double depth = 0.0;
    uint16_t depthVal = 0;
    for (uint32_t index = 0; index < ImageVars::width * ImageVars::height; index++) {
        std::memcpy(&depthVal, &m_depthBuf[2 * index], sizeof(uint16_t));

        QColor color;
        if (depthVal < 2047) {
            depth = m_depthLUT[depthVal];
            color = QColor::fromHsv(14.5745 * depth, 255, 255);
        }
        else {
            depth = 0.0;
            color = QColor::fromHsv(0, 255, 0);
        }

        // Assign values from 0 to 5 meters with a shade from black to white
        m_cvDepthImage.data[3 * index + 0] = color.blue();
        m_cvDepthImage.data[3 * index + 1] = color.green();
        m_cvDepthImage.data[3 * index + 2] = color.red();
    }

    // Make HBITMAP from pixel array
    m_depthDisplayMutex.lock();

    auto lineColor = cv::Scalar(0x00, 0xFF, 0x00);

    if (m_foundScreen) {
        // Draw lines to show user where the screen is
        cv::line(m_cvDepthImage, m_quad.point[0], m_quad.point[1], lineColor, 2);
        cv::line(m_cvDepthImage, m_quad.point[1], m_quad.point[2], lineColor, 2);
        cv::line(m_cvDepthImage, m_quad.point[2], m_quad.point[3], lineColor, 2);
        cv::line(m_cvDepthImage, m_quad.point[3], m_quad.point[0], lineColor, 2);
    }

    m_depthDisplayMutex.unlock();

    m_depthImageMutex.unlock();

    callNewImage(nullptr, 0);
}

double Kinect::rawDepthToMeters(uint16_t depthValue) {
    if (depthValue < 2047) {
        return m_depthLUT[depthValue];
    }
    else {
        return 0.0;
    }
}

/*
 * Callback called by libfreenect each time the buffer is filled with a
 * new RGB frame
 *
 * vidBuf: pointer to the RGB buffer
 * timestamp: POSIX timestamp of the buffer
 *
 * not safe for multiple instances because nstm has to be global
 */
void Kinect::VideoCallback(void* vidBuf, uint32_t timestamp) {
    (void) timestamp;

    // Copy data into external buffer
    std::copy(static_cast<uint8_t*>(vidBuf),
              static_cast<uint8_t*>(vidBuf) + getVideoBufferSize(),
              m_vidBuf.begin());

    // Call the new frame callback
    newVideoFrame();
}

/*
 * Callback called by libfreenect each time the buffer is filled with a
 * new depth frame
 *
 * depthBuf: pointer to the depth buffer
 * timestamp: POSIX timestamp of the buffer
 *
 * not safe for multiple instances because nstm has to be global
 */
void Kinect::DepthCallback(void* depthBuf, uint32_t timestamp) {
    (void) timestamp;

    // Copy data into external buffer
    std::copy(static_cast<uint8_t*>(depthBuf),
              static_cast<uint8_t*>(depthBuf) + getDepthBufferSize(),
              m_depthBuf.begin());

    // Call the new frame callback
    newDepthFrame();
}
