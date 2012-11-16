//=============================================================================
//File Name: Kinect.hpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

// TODO Reliable way to detect if Kinect is still connected and functioning

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

    // Starts image stream from Kinect
    void startStream();

    // Stops image stream from Kinect
    void stopStream();

    /* Returns true if there is a Kinect connected to the USB port and
     * available to use
     */
    bool isConnected();

    /* Processes the image stored in the internal buffer
     * colorWanted determines what color to filter
     */
    void processImage( ProcColor colorWanted );

    /* Combines the red, green, and blue processed images with bitwise and
     * Stores the result in m_cvImage
     */
    void combineProcessedImages();

    /* Displays the most recently received image in the given window at the
     * given coordinates
     */
    void display( HWND window , int x , int y );

protected:
    struct knt_inst_t* m_kinect;

    HBITMAP m_displayImage;

    // OpenCV variables
    IplImage* m_cvImage;

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

private:
    bool m_connected;
    sf::Mutex m_imageMutex;
    sf::Mutex m_displayMutex;

    // Called when a new image is received (swaps the image buffer)
    static void newFrame( struct nstream_t* streamObject , void* classObject );
};

#endif // KINECT_HPP
