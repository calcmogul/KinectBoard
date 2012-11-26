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

    /* Processes the image stored in the internal buffer
     * colorWanted determines what color to filter
     */
    void processCalibImages( Processing::ProcColor colorWanted );

    /* Combines the red, green, and blue processed images with bitwise and
     * Stores the result in m_cvImage
     */
    void combineCalibImages();

    // Adds color to calibration steps
    void enableColor( ProcColor color );

    // Removes color from calibration steps
    void disableColor( ProcColor color );

protected:
    sf::Mutex m_vidImageMutex;
    sf::Mutex m_vidDisplayMutex;

    sf::Mutex m_depthImageMutex;
    sf::Mutex m_depthDisplayMutex;

    // Called when a new video image is received (swaps the image buffer)
    static void newVideoFrame( struct nstream_t* streamObject , void* classObject );

    // Called when a new depth image is received (swaps the image buffer)
    static void newDepthFrame( struct nstream_t* streamObject , void* classObject );

private:
    struct knt_inst_t* m_kinect;

    HBITMAP m_vidImage;
    HBITMAP m_depthImage;

    // OpenCV variables
    IplImage* m_cvVidImage;
    IplImage* m_cvDepthImage;

    // Calibration image storage (IplImage* is whole image)
    std::vector<IplImage*> m_calibImages;

    // Holds components of calibration image currently being worked on
    IplImage* m_redPart;
    IplImage* m_greenPart;
    IplImage* m_bluePart;

    // Used as temporary storage when bitwise-and'ing channels together
    IplImage* m_channelAnd;

    // Used as temporary storage when bitwise-and'ing entire images together
    IplImage* m_imageAnd;

    // Stores which colored images to include in calibration
    char m_enabledColors;

    // Displays the given image in the given window at the given coordinates
    void display( HWND window , int x , int y , HBITMAP image , sf::Mutex& displayMutex );

    static double rawDepthToMeters( unsigned short depthValue );
};

#endif // KINECT_HPP
