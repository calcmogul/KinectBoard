//=============================================================================
//File Name: Kinect.hpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

/*
 * startStream() must be called to start the image stream upon construction of
 * the object
 */

#ifndef KINECT_HPP
#define KINECT_HPP

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ImageVars.hpp"
#include "Processing.hpp"
#include "CKinect/Parse.hpp"
#include "CKinect/NStream.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <cstdint>

#include <libfreenect/libfreenect.h>
#include <opencv2/core/core_c.h>

#define WM_KINECT_VIDEOSTART  (WM_APP + 0x0001)
#define WM_KINECT_VIDEOSTOP   (WM_APP + 0x0002)
#define WM_KINECT_DEPTHSTART  (WM_APP + 0x0003)
#define WM_KINECT_DEPTHSTOP   (WM_APP + 0x0004)

class Kinect : public Processing {
public:
    Kinect();
    virtual ~Kinect();

    // Starts video stream from Kinect
    void startVideoStream();

    // Starts depth stream from Kinect
    void startDepthStream();

    // Stops video stream from Kinect
    void stopVideoStream();

    // Stops depth stream from Kinect
    void stopDepthStream();

    // Returns true if the RGB image stream is running
    bool isVideoStreamRunning();

    // Returns true if the depth image stream is running
    bool isDepthStreamRunning();

    // Set max frame rate of video stream
    void setVideoStreamFPS(unsigned int fps);

    // Set max frame rate of depth image stream
    void setDepthStreamFPS(unsigned int fps);

    // Set window to which to send Kinect video stream messages
    void registerVideoWindow(HWND window);

    // Set window to which to send Kinect depth stream messages
    void registerDepthWindow(HWND window);

    // Stop Kinect video stream from displaying on the registered window
    void unregisterVideoWindow();

    // Stop Kinect depth stream from displaying on the registered window
    void unregisterDepthWindow();

    /* Returns currently registered video window
     * (may return NULL if no window is registered)
     */
    const HWND getRegisteredVideoWindow();

    /* Returns currently registered depth window
     * (may return NULL if no window is registered)
     */
    const HWND getRegisteredDepthWindow();

    /* Displays most recently received RGB image
     * If it's called in response to the WM_PAINT message, pass in the window's
     * device context received from BeginPaint()
     */
    void displayVideo(HWND window, int x, int y, HDC deviceContext = nullptr);

    /* Displays most recently processed depth image
    * If it's called in response to the WM_PAINT message, pass in the window's
    * device context received from BeginPaint()
    */
    void displayDepth(HWND window, int x, int y, HDC deviceContext = nullptr);

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
    void setScreenRect(RECT screenRect);

protected:
    std::mutex m_vidImageMutex;
    std::mutex m_vidDisplayMutex;

    std::mutex m_depthImageMutex;
    std::mutex m_depthDisplayMutex;

    std::mutex m_vidWindowMutex;
    std::mutex m_depthWindowMutex;

    CvSize m_imageSize;

    // Called when a new video image is received (swaps the image buffer)
    static void newVideoFrame(NStream<Kinect>& streamObject, void* classObject);

    // Called when a new depth image is received (swaps the image buffer)
    static void newDepthFrame(NStream<Kinect>& streamObject, void* classObject);

private:
    RECT m_screenRect;

    HBITMAP m_vidImage = nullptr;
    HBITMAP m_depthImage = nullptr;

    HWND m_vidWindow = nullptr;
    HWND m_depthWindow = nullptr;

    std::vector<uint8_t> m_vidBuffer;
    std::vector<uint8_t> m_depthBuffer;

    // OpenCV variables
    IplImage* m_cvVidImage;
    IplImage* m_cvDepthImage;
    IplImage* m_cvBitmapDest;

    // Calibration image storage (IplImage* is whole image)
    std::vector<IplImage*> m_calibImages;

    // Stores which colored images to include in calibration
    char m_enabledColors = 0x00;

    // Used for mouse tracking
    bool m_moveMouse = true;
    bool m_foundScreen = false;
    Quad m_quad;
    std::list<CvPoint> m_plistRaw;
    std::list<CvPoint> m_plistProc;

    // Used for moving mouse cursor and clicking mouse buttons
    INPUT m_input = {0};

    // Used to control maximum frame rate of each image stream
    std::chrono::time_point<std::chrono::system_clock> m_lastVidFrameTime;
    std::chrono::time_point<std::chrono::system_clock> m_lastDepthFrameTime;

    // Set frame rates to maximum the Kinect supports
    unsigned int m_vidFrameRate = 30;
    unsigned int m_depthFrameRate = 30;

    // Displays the given image in the given window at the given coordinates
    void display(HWND window, int x, int y, HBITMAP image, std::mutex& displayMutex, HDC deviceContext);

    static char* RGBtoBITMAPdata(const char* imageData, unsigned int width,
                                 unsigned int height);

    static double rawDepthToMeters(unsigned short depthValue);


    NStream<Kinect> rgb{640, 480, 3, &Kinect::startstream, &Kinect::rgb_stopstream, this};
    NStream<Kinect> depth{640, 480, 2, &Kinect::startstream, &Kinect::depth_stopstream, this};

    std::thread thread;

    std::atomic<bool> threadrunning{false};
    std::mutex threadrunning_mutex;
    std::condition_variable threadcond;

    static void rgb_cb(freenect_device* dev, void* rgbBuf, uint32_t timestamp);
    static void depth_cb(freenect_device* dev, void* depthBuf, uint32_t timestamp);
    int startstream(NStream<Kinect>& stream);
    int rgb_stopstream();
    int depth_stopstream();
    void threadmain();
};

#endif // KINECT_HPP
