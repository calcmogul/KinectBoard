//=============================================================================
//File Name: Kinect.cpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

#include <cstring>
#include "Kinect.hpp"

#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>

#include <SFML/Graphics/Image.hpp>
#include <iostream> // TODO Remove me

Kinect::Kinect() : m_kinect( NULL ) , m_displayImage( NULL ) {
    CvSize imageSize = { ImageVars::Width , ImageVars::Height };
    m_cvImage = cvCreateImage( imageSize , 8 , 4 );
    m_cvImage->imageData = static_cast<char*>( std::malloc( ImageVars::Width * ImageVars::Height * 4 ) );

    m_red1 = cvCreateImage( imageSize , 8 , 1 );
    m_green1 = cvCreateImage( imageSize , 8 , 1 );
    m_blue1 = cvCreateImage( imageSize , 8 , 1 );

    m_red2 = cvCreateImage( imageSize , 8 , 1 );
    m_green2 = cvCreateImage( imageSize , 8 , 1 );
    m_blue2 = cvCreateImage( imageSize , 8 , 1 );

    m_red3 = cvCreateImage( imageSize , 8 , 1 );
    m_green3 = cvCreateImage( imageSize , 8 , 1 );
    m_blue3 = cvCreateImage( imageSize , 8 , 1 );

    m_channelAnd1 = cvCreateImage( imageSize , 8 , 1 );
    m_channelAnd2 = cvCreateImage( imageSize , 8 , 1 );

    m_imageAnd1 = cvCreateImage( imageSize , 8 , 3 );
    m_imageAnd2 = cvCreateImage( imageSize , 8 , 3 );

    m_redCalib = cvCreateImage( imageSize , 8 , 3 );
    m_greenCalib = cvCreateImage( imageSize , 8 , 3 );
    m_blueCalib = cvCreateImage( imageSize , 8 , 3 );
}

Kinect::~Kinect() {
    stopStream();

    m_imageMutex.lock();
    std::free( m_cvImage->imageData );
    m_imageMutex.unlock();

    cvReleaseImage( &m_cvImage );

    cvReleaseImage( &m_red1 );
    cvReleaseImage( &m_green1 );
    cvReleaseImage( &m_blue1 );

    cvReleaseImage( &m_red2 );
    cvReleaseImage( &m_green2 );
    cvReleaseImage( &m_blue2 );

    cvReleaseImage( &m_red3 );
    cvReleaseImage( &m_green3 );
    cvReleaseImage( &m_blue3 );

    cvReleaseImage( &m_channelAnd1 );
    cvReleaseImage( &m_channelAnd2 );

    cvReleaseImage( &m_imageAnd1 );
    cvReleaseImage( &m_imageAnd2 );

    cvReleaseImage( &m_redCalib );
    cvReleaseImage( &m_greenCalib );
    cvReleaseImage( &m_blueCalib );
}

void Kinect::startStream() {
    // Initialize Kinect
    if ( m_kinect == NULL ) {
        m_kinect = knt_init();
    }

    // If the Kinect instance is valid, start the rgb image stream
    if ( m_kinect != NULL ) {
        m_kinect->rgb->newframe = newFrame;

        m_kinect->rgb->callbackarg = this;
        m_kinect->rgb->startstream( m_kinect->rgb );
    }
}

void Kinect::stopStream() {
    // Stop the Kinect stream if it's running
    if ( m_kinect != NULL ) {
        knt_destroy( m_kinect );

        // wait for Kinect thread to close before destroying instance
        while ( m_kinect->threadrunning == 1 ) {
            Sleep( 100 );
        }

        m_kinect = NULL;
    }
}

bool Kinect::isConnected() {
    bool isConnected = false;

    // Returns true if m_kinect is initialized
    if ( m_kinect != NULL ) {
        pthread_mutex_lock( &m_kinect->threadrunning_mutex );

        isConnected = ( m_kinect->threadrunning == 1 );

        pthread_mutex_unlock( &m_kinect->threadrunning_mutex );
    }

    return isConnected;
}

void Kinect::processImage( ProcColor colorWanted ) {
    if ( isConnected() ) {
        // Split image into individual channels
        m_imageMutex.lock();
        cvSplit( m_cvImage , m_red1 , m_green1 , m_blue1 , NULL );
        m_imageMutex.unlock();

        // Filter out all but color requested
        if ( colorWanted == Red ) {
            cvThreshold( m_red1 , m_red1 , 200 , 255 , CV_THRESH_BINARY );
        }
        else {
            cvThreshold( m_red1 , m_red1 , 55 , 255 , CV_THRESH_BINARY_INV );
        }

        if ( colorWanted == Green ) {
            cvThreshold( m_green1 , m_green1 , 200 , 255 , CV_THRESH_BINARY );
        }
        else {
            cvThreshold( m_green1 , m_green1 , 55 , 255 , CV_THRESH_BINARY_INV );
        }

        if ( colorWanted == Blue ) {
            cvThreshold( m_blue1 , m_blue1 , 200 , 255 , CV_THRESH_BINARY );
        }
        else {
            cvThreshold( m_blue1 , m_blue1 , 55 , 255 , CV_THRESH_BINARY_INV );
        }

        // Combine all colors to make sure white isn't included in the final picture
        cvAnd( m_red1 , m_green1 , m_channelAnd1 , NULL );
        cvAnd( m_channelAnd1 , m_blue1 , m_channelAnd2 , NULL );

        if ( colorWanted == Red ) {
            cvMerge( m_channelAnd2 , m_channelAnd2 , m_channelAnd2 , NULL , m_redCalib );
        }
        else if ( colorWanted == Green ) {
            cvMerge( m_channelAnd2 , m_channelAnd2 , m_channelAnd2 , NULL , m_greenCalib );
        }
        else if ( colorWanted == Blue ) {
            cvMerge( m_channelAnd2 , m_channelAnd2 , m_channelAnd2 , NULL , m_blueCalib );
        }
    }
}

void Kinect::combineProcessedImages() {
    if ( isConnected() ) {
        cvAnd( m_redCalib , m_greenCalib , m_imageAnd1 , NULL );
        cvAnd( m_imageAnd1 , m_blueCalib , m_imageAnd2 , NULL );

        m_imageMutex.lock();
        m_displayMutex.lock();
        m_displayImage = CreateBitmap( ImageVars::Width , ImageVars::Height , 1 , 32 , m_imageAnd2->imageData );
        m_displayMutex.unlock();
        m_imageMutex.unlock();
    }
}

void Kinect::display( HWND window , int x , int y ) {
    m_displayMutex.lock();

    if ( m_displayImage != NULL ) {
        // Create offscreen DC for image to go on
        HDC imageHdc = CreateCompatibleDC( NULL );

        // Put the image into the offscreen DC and save the old one
        HBITMAP imageBackup = static_cast<HBITMAP>( SelectObject( imageHdc , m_displayImage ) );

        // Get DC of window to which to draw
        HDC windowHdc = GetDC( window );

        // Load image to real BITMAP just to retrieve its dimensions
        BITMAP tempBMP;
        GetObject( m_displayImage , sizeof( BITMAP ) , &tempBMP );

        // Copy image from offscreen DC to window's DC
        BitBlt( windowHdc , x , y , tempBMP.bmWidth , tempBMP.bmHeight , imageHdc , 0 , 0 , SRCCOPY );

        // Release window's HDC
        ReleaseDC( window , windowHdc );

        // Restore old image
        SelectObject( imageHdc , imageBackup );

        // Delete offscreen DC
        DeleteDC( imageHdc );
    }

    m_displayMutex.unlock();
}

void Kinect::newFrame( struct nstream_t* streamObject , void* classObject ) {
    Kinect* kinectPtr = reinterpret_cast<Kinect*>( classObject );

    kinectPtr->m_imageMutex.lock();

    char* pxlBuf = kinectPtr->m_kinect->rgb->buf;

    /* ===== Convert RGB to BGRA before displaying the image ===== */
    /* Swap R and B because Win32 expects the color components in the
     * opposite order they are currently in
     */

    for ( unsigned int startI = 0 , endI = 0 ; endI < ImageVars::Width * ImageVars::Height * 4 ; startI += 3 , endI += 4 ) {
        kinectPtr->m_cvImage->imageData[endI] = pxlBuf[startI+2];
        kinectPtr->m_cvImage->imageData[endI+1] = pxlBuf[startI+1];
        kinectPtr->m_cvImage->imageData[endI+2] = pxlBuf[startI];
    }
    /* =========================================================== */

    // Make HBITMAP from pixel array
    kinectPtr->m_displayMutex.lock();
    kinectPtr->m_displayImage = CreateBitmap( ImageVars::Width , ImageVars::Height , 1 , 32 , kinectPtr->m_cvImage->imageData );
    kinectPtr->m_displayMutex.unlock();

    kinectPtr->m_imageMutex.unlock();
}
