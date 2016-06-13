//=============================================================================
// Description: Manages interface with a Microsoft Kinect connected to the USB
//              port of the computer
//=============================================================================

/*
 * startStream() must be called to start the image stream upon construction of
 * the object
 */

#ifndef KINECT_HPP
#define KINECT_HPP

#include "ImageVars.hpp"
#include "Processing.hpp"
#include "ClientBase.hpp"
#include "CKinect/Parse.hpp"
#include "CKinect/NStream.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <cstdint>

#include <QColor>
#include <QRect>

#include <libfreenect/libfreenect.hpp>
#include <opencv2/core/core.hpp>

#include "VideoStream.hpp"

class Kinect : public ClientBase<VideoStream>, public Freenect::FreenectDevice {
public:
    Kinect(freenect_context* context, int index);
    virtual ~Kinect();

    // Starts video and depth streams from Kinect
    void start();

    // Stops video and depth streams from Kinect
    void stop();

    // Returns true if streaming is on
    bool isStreaming() const;

    // Saves most recently received image to a file
    void saveCurrentImage(const std::string& fileName);

    /* Copies the most recently received image into a secondary internal buffer
     * and returns it to the user. After a call to this function, the new size
     * should be retrieved since it may have changed. Do NOT access the buffer
     * pointer returned while this function is executing.
     */
    uint8_t* getCurrentImage();

    // Returns size of image currently in secondary buffer
    uint32_t getCurrentWidth() const;
    uint32_t getCurrentHeight() const;

    // Returns true if the RGB image stream is running
    bool isVideoStreamRunning() const;

    // Returns true if the depth image stream is running
    bool isDepthStreamRunning() const;

    // Set window to which to send Kinect video stream messages
    void registerVideoWindow();

    // Set window to which to send Kinect depth stream messages
    void registerDepthWindow();

    // Saves most recently received RGB image to file
    bool saveVideo(const std::string& fileName);

    // Save most recently received depth image to file
    bool saveDepth(const std::string& fileName);

    // Stores current image as calibration image containing the given color
    void setCalibImage(ProcColor colorWanted);

    /* Processes calibration images stored in internal buffer to find location
     * of screen
     */
    void calibrate();

    /* Find points within screen boundary that could be mouse cursors and sets
     * system mouse to match its location
     */
    void lookForCursors();

    // Turns mouse tracking on/off so user can regain control
    void setMouseTracking(bool on);

    // Adds color to calibration steps
    void enableColor(ProcColor color);

    // Removes color from calibration steps
    void disableColor(ProcColor color);

    // Returns true if there is a calibration image of the given color enabled
    bool isEnabled(ProcColor color) const;

    /* Give class the region of the screen being tracked so the mouse is moved
     * on the correct monitor
     */
    void setScreenRect(const QRect& screenRect);

protected:
    std::mutex m_vidImageMutex;
    std::mutex m_vidDisplayMutex;

    std::mutex m_depthImageMutex;
    std::mutex m_depthDisplayMutex;

    std::mutex m_vidWindowMutex;
    std::mutex m_depthWindowMutex;

    cv::Size m_imageSize;

    // Called when a new video image is received (swaps the image buffer)
    void newVideoFrame();

    // Called when a new depth image is received (swaps the image buffer)
    void newDepthFrame();

private:
    QRect m_screenRect;

    // If true, display m_vidBuffer. Otherwise, display m_depthBuffer
    std::atomic<bool> m_displayVid{true};

    // OpenCV variables
    cv::Mat m_cvVidImage;
    cv::Mat m_cvDepthImage;
    cv::Mat m_cvBitmapDest;

    // Calibration image storage (IplImage* is whole image)
    std::vector<cv::Mat> m_calibImages;

    // Stores which colored images to include in calibration
    char m_enabledColors = 0x00;

    // Used for mouse tracking
    bool m_moveMouse = true;
    bool m_foundScreen = false;
    Quad m_quad;
    std::list<cv::Point> m_plistRaw;
    std::list<cv::Point> m_plistProc;

    static double rawDepthToMeters(uint16_t depthValue);

    static constexpr uint32_t m_width = 640;
    static constexpr uint32_t m_height = 480;

    std::array<uint8_t, m_width * m_height * 3> m_vidBuf;
    bool m_videoRunning = false;

    std::array<uint8_t, m_width * m_height * 2> m_depthBuf;
    bool m_depthRunning = false;
    static std::array<double, 2048> m_depthLUT;

    static Freenect::Freenect m_freenectInst;

    void VideoCallback(void* vidBuf, uint32_t timestamp) override;
    void DepthCallback(void* depthBuf, uint32_t timestamp) override;
};

#endif // KINECT_HPP
