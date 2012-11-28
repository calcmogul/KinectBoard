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

#include "CKinect/kinect.h"
#include "ImageVars.hpp"
#include "ProcColor.hpp"
#include <vector>
#include <string>
#include <opencv2/core/types_c.h>

#include <SFML/System/Mutex.hpp>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct _IplImage IplImage;

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

    // Displays most recently received RGB image
    void displayVideo( HWND window , int x , int y );

    // Displays most recently processed depth image
    void displayDepth( HWND window , int x , int y );

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

    void lookForCursors();

    // Adds color to calibration steps
    void enableColor( ProcColor color );

    // Removes color from calibration steps
    void disableColor( ProcColor color );

    // Returns true if there is a calibration image of the given color enabled
    bool isEnabled( ProcColor color );

protected:
    sf::Mutex m_vidImageMutex;
    sf::Mutex m_vidDisplayMutex;

    sf::Mutex m_depthImageMutex;
    sf::Mutex m_depthDisplayMutex;

    CvSize m_imageSize;

    // Called when a new video image is received (swaps the image buffer)
    static void newVideoFrame( struct nstream_t* streamObject , void* classObject );

    // Called when a new depth image is received (swaps the image buffer)
    static void newDepthFrame( struct nstream_t* streamObject , void* classObject );

private:
    struct knt_inst_t* m_kinect;

    HBITMAP m_vidImage;
    HBITMAP m_depthImage;

    char* m_vidBuffer;
    char* m_depthBuffer;

    // OpenCV variables
    IplImage* m_cvVidImage;
    IplImage* m_cvDepthImage;

    // Calibration image storage (IplImage* is whole image)
    std::vector<IplImage*> m_calibImages;

    struct quad_t* m_quad;
    struct plist_t* m_plistRaw;
    struct plist_t* m_plistProc;

    // Stores which colored images to include in calibration
    char m_enabledColors;

    // Displays the given image in the given window at the given coordinates
    void display( HWND window , int x , int y , HBITMAP image , sf::Mutex& displayMutex );

    static double rawDepthToMeters( unsigned short depthValue );
};

#endif // KINECT_HPP
