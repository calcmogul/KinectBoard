//=============================================================================
//File Name: Kinect.cpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

#include <cstring>
#include <cstdlib>
#include "Kinect.hpp"
#include <SFML/Graphics/Image.hpp>

#include <iostream> // TODO Remove me

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

    m_redPart = cvCreateImage( imageSize , 8 , 1 );
    m_greenPart = cvCreateImage( imageSize , 8 , 1 );
    m_bluePart = cvCreateImage( imageSize , 8 , 1 );

    m_channelAnd = cvCreateImage( imageSize , 8 , 1 );

    m_imageAnd = cvCreateImage( imageSize , 8 , 4 );

    m_enabledColors = 0x00;

    for ( unsigned int index = 0 ; index < ProcColor::Size ; index++ ) {
        m_calibImages.push_back( new IplImage );
        m_calibImages.at( m_calibImages.size() - 1 ) = cvCreateImage( imageSize , 8 , 4 );
    }
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

    if ( m_vidImage != NULL ) {
        DeleteObject( m_vidImage );
    }

    if ( m_depthImage != NULL ) {
        DeleteObject( m_depthImage );
    }

    m_vidImageMutex.lock();
    std::free( m_cvVidImage->imageData );
    m_vidImageMutex.unlock();

    m_depthImageMutex.lock();
    std::free( m_cvDepthImage->imageData );
    m_depthImageMutex.unlock();

    cvReleaseImage( &m_cvVidImage );
    cvReleaseImage( &m_cvDepthImage );

    cvReleaseImage( &m_redPart );
    cvReleaseImage( &m_greenPart );
    cvReleaseImage( &m_bluePart );

    cvReleaseImage( &m_channelAnd );

    cvReleaseImage( &m_imageAnd );

    for ( unsigned int index = m_calibImages.size() ; index > 0 ; index-- ) {
        cvReleaseImage( &m_calibImages.at( index - 1 ) );
    }
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

bool Kinect::saveVideo( const std::string& fileName ) const {
    unsigned char* imageData = (unsigned char*)std::malloc( ImageVars::width * ImageVars::height * 4 );

    // Copy OpenCV BGR image to RGB array
    for ( unsigned int i = 0 ; i < ImageVars::width * ImageVars::height * 4 ; i += 4 ) {
        imageData[i+0] = m_cvVidImage->imageData[i+2];
        imageData[i+1] = m_cvVidImage->imageData[i+1];
        imageData[i+2] = m_cvVidImage->imageData[i+0];
        imageData[i+3] = 255;
    }

    sf::Image imageBuffer;
    imageBuffer.create( ImageVars::width , ImageVars::height , imageData );

    return imageBuffer.saveToFile( fileName );
}

bool Kinect::saveDepth( const std::string& fileName ) const {
    unsigned char* imageData = (unsigned char*)std::malloc( ImageVars::width * ImageVars::height * 4 );

    // Copy OpenCV BGR image to RGB array
    for ( unsigned int i = 0 ; i < ImageVars::width * ImageVars::height * 4 ; i += 4 ) {
        imageData[i+0] = m_cvDepthImage->imageData[i+2];
        imageData[i+1] = m_cvDepthImage->imageData[i+1];
        imageData[i+2] = m_cvDepthImage->imageData[i+0];
        imageData[i+3] = 255;
    }

    sf::Image imageBuffer;
    imageBuffer.create( ImageVars::width , ImageVars::height , imageData );

    return imageBuffer.saveToFile( fileName );
}

void Kinect::processCalibImages( Processing::ProcColor colorWanted ) {
    if ( isVideoStreamRunning() ) {
        // Split image into individual channels
        m_vidImageMutex.lock();
        cvSplit( m_cvVidImage , m_bluePart , m_greenPart , m_redPart , NULL );
        m_vidImageMutex.unlock();

        // Filter out all but color requested
        if ( colorWanted == Red ) {
            cvThreshold( m_redPart , m_redPart , 140 , 255 , CV_THRESH_BINARY );
        }
        else {
            cvThreshold( m_redPart , m_redPart , 40 , 255 , CV_THRESH_BINARY_INV );
        }

        if ( colorWanted == Green ) {
            cvThreshold( m_greenPart , m_greenPart , 140 , 255 , CV_THRESH_BINARY );
        }
        else {
            cvThreshold( m_greenPart , m_greenPart , 40 , 255 , CV_THRESH_BINARY_INV );
        }

        if ( colorWanted == Blue ) {
            cvThreshold( m_bluePart , m_bluePart , 140 , 255 , CV_THRESH_BINARY );
        }
        else {
            cvThreshold( m_bluePart , m_bluePart , 40 , 255 , CV_THRESH_BINARY_INV );
        }

        // Combine all colors to make sure white isn't included in the final picture
        cvAnd( m_redPart , m_greenPart , m_channelAnd , NULL );
        cvAnd( m_channelAnd , m_bluePart , m_channelAnd , NULL );
        cvMerge( m_channelAnd , m_channelAnd , m_channelAnd , NULL , m_calibImages[colorWanted] );

        m_vidImageMutex.lock();
        m_vidDisplayMutex.lock();
        if ( colorWanted == Blue ) {
            //m_vidImage = CreateBitmap( ImageVars::width , ImageVars::height , 1 , 32 , m_calibImages[colorWanted]->imageData );
        }
        m_vidDisplayMutex.unlock();
        m_vidImageMutex.unlock();
    }
}

void Kinect::combineCalibImages() {
    if ( isVideoStreamRunning() ) {
        std::cout << "Combine\n";
        static CvScalar whiteImage = cvScalar( 255 , 255 , 255 , 255 );

        /* Makes all pixels from first calibration image get copied to AND
         * buffer upon first loop iteration
         */
        cvSet( m_imageAnd , whiteImage , NULL );

        for ( unsigned int index = 0 ; index < ProcColor::Size ; index++ ) {
            // If color is enabled
            if ( m_enabledColors | ( 1 << index ) ) {
                cvAnd( m_imageAnd , m_calibImages[index] , m_imageAnd , NULL );
            }
        }

        m_vidImageMutex.lock();
        m_vidDisplayMutex.lock();

        if ( m_vidImage != NULL ) {
            DeleteObject( m_vidImage );
        }
        m_vidImage = CreateBitmap( ImageVars::width , ImageVars::height , 1 , 32 , m_imageAnd->imageData );

        m_vidDisplayMutex.unlock();
        m_vidImageMutex.unlock();
    }
}

void Kinect::enableColor( ProcColor color ) {
    m_enabledColors |= ( 1 << color );
}

void Kinect::disableColor( ProcColor color ) {
    m_enabledColors &= ~( 1 << color );
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

    if ( kinectPtr->m_vidImage != NULL ) {
        DeleteObject( kinectPtr->m_vidImage );
    }
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

    if ( kinectPtr->m_depthImage != NULL ) {
        DeleteObject( kinectPtr->m_depthImage );
    }
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
