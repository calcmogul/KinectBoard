//=============================================================================
//File Name: Kinect.cpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

#include <cstdio>
#include <cstring>
#include "Kinect.hpp"

#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>

#include <SFML/Graphics/Image.hpp>

Kinect::Kinect() : m_displayImage( NULL ) {
    m_connected = false; // may be false if something in initialization fails later

    // Initialize Kinect
    m_kinect = knt_init();

    // start the rgb image stream
    if ( m_kinect != NULL ) {
        m_kinect->rgb->newframe = newFrame;

        m_kinect->rgb->startstream( m_kinect->rgb );
    }

    std::printf( "Kinect initialized\n" );

    CvSize imageSize = { 320 , 240 };
    m_cvImage = cvCreateImage( imageSize , 8 , 4 );

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

    std::printf( "Stream started\n" );
}

Kinect::~Kinect() {
    if ( m_kinect != NULL ) {
        knt_destroy( m_kinect );
    }

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
    // Restart the Kinect stream
    m_kinect->rgb->startstream( m_kinect->rgb );
}

bool Kinect::isConnected() {
    pthread_mutex_lock( &m_kinect->threadrunning_mutex );
    m_connected = m_kinect->threadrunning;
    pthread_mutex_unlock( &m_kinect->threadrunning_mutex );

    return m_connected;
}

void Kinect::processImage( ProcColor colorWanted ) {
    // Split RGB image into its different channels
    sf::Image testImage;
    testImage.loadFromFile( "Test.png" );

    char* pxlBuf = static_cast<char*>( std::malloc( testImage.getSize().x * testImage.getSize().y * 4 ) );
    std::memcpy( pxlBuf , testImage.getPixelsPtr() , testImage.getSize().x * testImage.getSize().y * 4 );

    m_imageMutex.lock();
    m_cvImage->imageData = pxlBuf; // TODO Remove me after testing is done
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

void Kinect::combineImages() {
    cvAnd( m_redCalib , m_greenCalib , m_imageAnd1 , NULL );
    cvAnd( m_imageAnd1 , m_blueCalib , m_imageAnd2 , NULL );

    m_imageMutex.lock();
    m_displayImage = CreateBitmap( 320 , 240 , 1 , 32 , m_imageAnd2->imageData );
    m_imageMutex.unlock();
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
    std::printf( "Got new frame\n" );
    Kinect* kinectPtr = static_cast<Kinect*>( classObject );

    kinectPtr->m_imageMutex.lock();
    kinectPtr->m_cvImage->imageData = kinectPtr->m_kinect->rgb->buf;

    /* ===== Convert RGB to BGRA before displaying the image ===== */
    /* Swap R and B because Win32 expects the color components in the
     * opposite order they are currently in
     */
    char* pxlBuf = static_cast<char*>( malloc( 320 * 240 * 4 ) );

    for ( unsigned int i = 0 ; i < 320 * 240 * 4 ; i += 4 ) {
        pxlBuf[i] = kinectPtr->m_cvImage->imageData[i+2];
        pxlBuf[i+1] = kinectPtr->m_cvImage->imageData[i+1];
        pxlBuf[i+2] = kinectPtr->m_cvImage->imageData[i];
    }
    /* ============================================================ */

    // Make HBITMAP from pixel array
    kinectPtr->m_displayImage = CreateBitmap( 320 , 240 , 1 , 32 , pxlBuf );
    kinectPtr->m_imageMutex.unlock();

    free( pxlBuf );
}
