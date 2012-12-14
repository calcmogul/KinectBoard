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

#include "CKinect/kinect.h"
#include "ImageVars.hpp"
#include "Processing.hpp"
#include <vector>
#include <string>
#include <opencv2/core/types_c.h>

#include "SFML/System/Clock.hpp"
#include "SFML/System/Mutex.hpp"

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
    void setVideoStreamFPS( unsigned int fps );

    // Set max frame rate of depth image stream
    void setDepthStreamFPS( unsigned int fps );

    // Set window to which to send Kinect video stream messages
    void registerVideoWindow( HWND window );

    // Set window to which to send Kinect depth stream messages
    void registerDepthWindow( HWND window );

    /* Displays most recently received RGB image
     * If it's called in response to the WM_PAINT message, pass in the window's
     * device context received from BeginPaint()
     */
    void displayVideo( HWND window , int x , int y , HDC deviceContext = NULL );

    /* Displays most recently processed depth image
    * If it's called in response to the WM_PAINT message, pass in the window's
    * device context received from BeginPaint()
    */
    void displayDepth( HWND window , int x , int y , HDC deviceContext = NULL );

    // Saves most recently received RGB image to file
    bool saveVideo( const std::string& fileName ) const;

    // Save most recently received depth image to file
    bool saveDepth( const std::string& fileName ) const;

    // Stores current image as calibration image containing the given color
    void setCalibImage( ProcColor colorWanted );

    /* Processes calibration images stored in internal buffer to find location
     * of screen
     */
    void calibrate();

    /* Find points within screen boundary that could be mouse cursors and sets
     * system mouse to match its location
     */
    void lookForCursors();

    // Turns mouse tracking on/off so user can regain control
    void setMouseTracking( bool on );

    // Adds color to calibration steps
    void enableColor( ProcColor color );

    // Removes color from calibration steps
    void disableColor( ProcColor color );

    // Returns true if there is a calibration image of the given color enabled
    bool isEnabled( ProcColor color );

    /* Give class the region of the screen being tracked so the mouse is moved
     * on the correct monitor
     */
    void setScreenRect( RECT screenRect );

protected:
    sf::Mutex m_vidImageMutex;
    sf::Mutex m_vidDisplayMutex;

    sf::Mutex m_depthImageMutex;
    sf::Mutex m_depthDisplayMutex;

    sf::Mutex m_vidWindowMutex;
    sf::Mutex m_depthWindowMutex;

    CvSize m_imageSize;

    // Called when a new video image is received (swaps the image buffer)
    static void newVideoFrame( struct nstream_t* streamObject , void* classObject );

    // Called when a new depth image is received (swaps the image buffer)
    static void newDepthFrame( struct nstream_t* streamObject , void* classObject );

private:
    RECT m_screenRect;

    struct knt_inst_t* m_kinect;

    HBITMAP m_vidImage;
    HBITMAP m_depthImage;

    HWND m_vidWindow;
    HWND m_depthWindow;

    char* m_vidBuffer;
    char* m_depthBuffer;

    // OpenCV variables
    IplImage* m_cvVidImage;
    IplImage* m_cvDepthImage;
    IplImage* m_cvBitmapDest;

    // Calibration image storage (IplImage* is whole image)
    std::vector<IplImage*> m_calibImages;

    // Stores which colored images to include in calibration
    char m_enabledColors;

    // Used for mouse tracking
    bool m_moveMouse;
    bool m_foundScreen;
    struct quad_t* m_quad;
    struct plist_t* m_plistRaw;
    struct plist_t* m_plistProc;

    // Used for moving mouse cursor and clicking mouse buttons
    INPUT m_input;

    // Used to control maximum frame rate of each image stream
    sf::Clock m_vidFrameTime;
    sf::Clock m_depthFrameTime;
    unsigned int m_vidFrameRate;
    unsigned int m_depthFrameRate;

    // Displays the given image in the given window at the given coordinates
    void display( HWND window , int x , int y , HBITMAP image , sf::Mutex& displayMutex , HDC deviceContext );

    static char* RGBtoBITMAPdata( const char* imageData , unsigned int width , unsigned int height );

    static double rawDepthToMeters( unsigned short depthValue );
};

#endif // KINECT_HPP
