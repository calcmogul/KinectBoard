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

#include <SFML/System/Mutex.hpp>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct _IplImage IplImage;

class Kinect {
public:
    enum ProcColor {
        Red = 0,
        Green = 1,
        Blue = 2
    };

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

    // Returns true if the rgb image stream is running
    bool isVideoStreamRunning();

    // Returns true if the depth image stream is running
    bool isDepthStreamRunning();

    // Displays most recently received RGB image
    void displayVideo( HWND window , int x , int y );

    // Displays most recently processed depth image
    void displayDepth( HWND window , int x , int y );

    /* Processes the image stored in the internal buffer
     * colorWanted determines what color to filter
     */
    void processCalib( ProcColor colorWanted );

    /* Combines the red, green, and blue processed images with bitwise and
     * Stores the result in m_cvImage
     */
    void combineCalibImages();

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

    // Holds components of red calibration image
    IplImage* m_red1;
    IplImage* m_green1;
    IplImage* m_blue1;

    // Holds components of green calibration image
    IplImage* m_red2;
    IplImage* m_green2;
    IplImage* m_blue2;

    // Holds components of blue calibration image
    IplImage* m_red3;
    IplImage* m_green3;
    IplImage* m_blue3;

    // Used as temporary storage when bitwise-and'ing channels together
    IplImage* m_channelAnd1;
    IplImage* m_channelAnd2;

    // Used as temporary storage when bitwise-and'ing entire images together
    IplImage* m_imageAnd1;
    IplImage* m_imageAnd2;

    // Calibration image storage
    IplImage* m_redCalib;
    IplImage* m_greenCalib;
    IplImage* m_blueCalib;

    // Displays the given image in the given window at the given coordinates
    void display( HWND window , int x , int y , HBITMAP image , sf::Mutex& displayMutex );

    static double rawDepthToMeters( unsigned short depthValue );
};

#endif // KINECT_HPP
