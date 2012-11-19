//=============================================================================
//File Name: Kinect.cpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

#include <cstring>
#include <cstdlib>
#include "Kinect.hpp"

#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>

Kinect::Kinect() : m_vidImage( NULL ) , m_depthImage( NULL ) {
    m_kinect = knt_init();

    if ( m_kinect != NULL ) {
        m_kinect->rgb->newframe = newVideoFrame;
        m_kinect->rgb->callbackarg = this;

        m_kinect->depth->newframe = newDepthFrame;
        m_kinect->depth->callbackarg = this;
    }

    CvSize imageSize = { static_cast<int>(ImageVars::width) , static_cast<int>(ImageVars::height) };
    m_cvVidImage = cvCreateImage( imageSize , 8 , 4 );
    m_cvVidImage->imageData = static_cast<char*>( std::malloc( ImageVars::width * ImageVars::height * 4 ) );

    m_cvDepthImage = cvCreateImage( imageSize , 8 , 4 );
    m_cvDepthImage->imageData = static_cast<char*>( std::malloc( ImageVars::width * ImageVars::height * 4 ) );

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
    stopVideoStream();
    stopDepthStream();

    if ( m_kinect != NULL ) {
        knt_destroy( m_kinect );

        // wait for Kinect thread to close before destroying instance
        while ( m_kinect->threadrunning == 1 ) {
            Sleep( 100 );
        }

        m_kinect = NULL;
    }

    m_vidImageMutex.lock();
    std::free( m_cvVidImage->imageData );
    m_vidImageMutex.unlock();

    m_depthImageMutex.lock();
    std::free( m_cvDepthImage->imageData );
    m_depthImageMutex.unlock();

    cvReleaseImage( &m_cvVidImage );
    cvReleaseImage( &m_cvDepthImage );

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

void Kinect::startVideoStream() {
    // Initialize Kinect if needed
    if ( m_kinect == NULL ) {
        m_kinect = knt_init();

        if ( m_kinect != NULL ) {
            m_kinect->rgb->newframe = newVideoFrame;
            m_kinect->rgb->callbackarg = this;
        }
    }

    // If the Kinect instance is valid
    if ( m_kinect != NULL ) {
        // If rgb image stream is down, start it
        if ( m_kinect->rgb->state == NSTREAM_DOWN ) {
            knt_startstream( m_kinect->rgb );
        }
    }
}

void Kinect::startDepthStream() {
    // Initialize Kinect if needed
    if ( m_kinect == NULL ) {
        m_kinect = knt_init();

        if ( m_kinect != NULL ) {
            m_kinect->depth->newframe = newDepthFrame;
            m_kinect->depth->callbackarg = this;
        }
    }

    // If the Kinect instance is valid
    if ( m_kinect != NULL ) {
        // If depth image stream is down, start it
        if ( m_kinect->depth->state == NSTREAM_DOWN ) {
            knt_startstream( m_kinect->depth );
        }
    }
}

void Kinect::stopVideoStream() {
    // If the Kinect instance is valid, stop the rgb image stream if it's running
    if ( m_kinect != NULL ) {
        knt_rgb_stopstream( m_kinect->rgb );
    }
}

void Kinect::stopDepthStream() {
    // If the Kinect instance is valid, stop the depth image stream if it's running
    if ( m_kinect != NULL ) {
        knt_depth_stopstream( m_kinect->depth );
    }
}

bool Kinect::isVideoStreamRunning() {
    if ( m_kinect != NULL ) {
        return ( m_kinect->rgb->state == NSTREAM_UP );
    }

    return false;
}

bool Kinect::isDepthStreamRunning() {
    if ( m_kinect != NULL ) {
        return ( m_kinect->depth->state == NSTREAM_UP );
    }

    return false;
}

void Kinect::displayVideo( HWND window , int x , int y ) {
    display( window , x , y , m_vidImage , m_vidDisplayMutex );
}

void Kinect::displayDepth( HWND window , int x , int y ) {
    display( window , x , y , m_depthImage , m_depthDisplayMutex );
}

void Kinect::processCalib( ProcColor colorWanted ) {
    if ( isVideoStreamRunning() ) {
        // Split image into individual channels
        m_vidImageMutex.lock();
        cvSplit( m_cvVidImage , m_red1 , m_green1 , m_blue1 , NULL );
        m_vidImageMutex.unlock();

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

void Kinect::combineCalibImages() {
    if ( isVideoStreamRunning() ) {
        cvAnd( m_redCalib , m_greenCalib , m_imageAnd1 , NULL );
        cvAnd( m_imageAnd1 , m_blueCalib , m_imageAnd2 , NULL );

        m_vidImageMutex.lock();
        m_vidDisplayMutex.lock();
        m_vidImage = CreateBitmap( ImageVars::width , ImageVars::height , 1 , 32 , m_imageAnd2->imageData );
        m_vidDisplayMutex.unlock();
        m_vidImageMutex.unlock();
    }
}

void Kinect::newVideoFrame( struct nstream_t* streamObject , void* classObject ) {
    Kinect* kinectPtr = reinterpret_cast<Kinect*>( classObject );

    kinectPtr->m_vidImageMutex.lock();

    char* pxlBuf = kinectPtr->m_kinect->rgb->buf;

    /* ===== Convert RGB to BGRA before displaying the image ===== */
    /* Swap R and B because Win32 expects the color components in the
     * opposite order they are currently in
     */

    for ( unsigned int startI = 0 , endI = 0 ; endI < ImageVars::width * ImageVars::height * 4 ; startI += 3 , endI += 4 ) {
        kinectPtr->m_cvVidImage->imageData[endI] = pxlBuf[startI+2];
        kinectPtr->m_cvVidImage->imageData[endI+1] = pxlBuf[startI+1];
        kinectPtr->m_cvVidImage->imageData[endI+2] = pxlBuf[startI];
    }
    /* =========================================================== */

    // Make HBITMAP from pixel array
    kinectPtr->m_vidDisplayMutex.lock();
    kinectPtr->m_vidImage = CreateBitmap( ImageVars::width , ImageVars::height , 1 , 32 , kinectPtr->m_cvVidImage->imageData );
    kinectPtr->m_vidDisplayMutex.unlock();

    kinectPtr->m_vidImageMutex.unlock();
}

void Kinect::newDepthFrame( struct nstream_t* streamObject , void* classObject ) {
    Kinect* kinectPtr = reinterpret_cast<Kinect*>( classObject );

    kinectPtr->m_depthImageMutex.lock();

    char* pxlBuf = kinectPtr->m_kinect->depth->buf;

    double depth = 0.0;
    unsigned short depthVal = 0;
    for ( unsigned int index = 0 ; index < ImageVars::width * ImageVars::height ; index++ ) {
        depthVal = (unsigned short) *((unsigned short*)(pxlBuf + 2 * index));

        depth = Kinect::rawDepthToMeters( depthVal );

        // Assign values from 0 to 5 meters with a shade from black to white
        kinectPtr->m_cvDepthImage->imageData[4 * index + 0] = 255.f * depth / 5.f;
        kinectPtr->m_cvDepthImage->imageData[4 * index + 1] = 255.f * depth / 5.f;
        kinectPtr->m_cvDepthImage->imageData[4 * index + 2] = 255.f * depth / 5.f;
    }

    // Make HBITMAP from pixel array
    kinectPtr->m_depthDisplayMutex.lock();
    kinectPtr->m_depthImage = CreateBitmap( ImageVars::width , ImageVars::height , 1 , 32 , kinectPtr->m_cvDepthImage->imageData );
    kinectPtr->m_depthDisplayMutex.unlock();

    kinectPtr->m_depthImageMutex.unlock();
}

void Kinect::display( HWND window , int x , int y , HBITMAP image , sf::Mutex& displayMutex ) {
    displayMutex.lock();

    if ( image != NULL ) {
        // Create offscreen DC for image to go on
        HDC imageHdc = CreateCompatibleDC( NULL );

        // Put the image into the offscreen DC and save the old one
        HBITMAP imageBackup = static_cast<HBITMAP>( SelectObject( imageHdc , image ) );

        // Get DC of window to which to draw
        HDC windowHdc = GetDC( window );

        // Load image to real BITMAP just to retrieve its dimensions
        BITMAP tempBMP;
        GetObject( image , sizeof( BITMAP ) , &tempBMP );

        // Copy image from offscreen DC to window's DC
        BitBlt( windowHdc , x , y , tempBMP.bmWidth , tempBMP.bmHeight , imageHdc , 0 , 0 , SRCCOPY );

        // Release window's HDC
        ReleaseDC( window , windowHdc );

        // Restore old image
        SelectObject( imageHdc , imageBackup );

        // Delete offscreen DC
        DeleteDC( imageHdc );
    }

    displayMutex.unlock();
}

double Kinect::rawDepthToMeters( unsigned short depthValue ) {
    if ( depthValue < 2047 ) {
        return 1.f /
                (static_cast<double>(depthValue) * -0.0030711016 + 3.3309495161 );
    }

    return 0.0;
}
