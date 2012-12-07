//=============================================================================
//File Name: Kinect.cpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "CKinect/parse.h"
#include "Kinect.hpp"
#include "WinAPIWrapper.h"
#include "SFML/Graphics/Image.hpp"

Kinect::Kinect() :
        m_vidImage( NULL ) ,
        m_depthImage( NULL ) ,
        m_vidWindow( NULL ) ,
        m_depthWindow( NULL ) ,
        m_foundScreen( false ) ,
        m_quad( NULL ) ,
        m_plistRaw( NULL ) ,
        m_plistProc( NULL ) {
    m_kinect = knt_init();

    if ( m_kinect != NULL ) {
        m_kinect->rgb->newframe = newVideoFrame;
        m_kinect->rgb->callbackarg = this;

        m_kinect->depth->newframe = newDepthFrame;
        m_kinect->depth->callbackarg = this;
    }

    m_imageSize = { static_cast<int>(ImageVars::width) , static_cast<int>(ImageVars::height) };

    m_cvVidImage = cvCreateImage( m_imageSize , IPL_DEPTH_8U , 3 );
    m_cvDepthImage = cvCreateImage( m_imageSize , IPL_DEPTH_8U , 4 );
    m_cvBitmapDest = cvCreateImage( m_imageSize , IPL_DEPTH_8U , 4 );

    // 3 bytes per pixel
    m_vidBuffer = (char*)std::malloc( ImageVars::width * ImageVars::height * 3 );

    // Each value in the depth image is 2 bytes long per pixel
    m_depthBuffer = (char*)std::malloc( ImageVars::width * ImageVars::height * 2 );

    m_enabledColors = 0x00;

    for ( unsigned int index = 0 ; index < ProcColor::Size ; index++ ) {
        m_calibImages.push_back( new IplImage );
        m_calibImages.at( m_calibImages.size() - 1 ) = NULL;
    }

    m_input = { 0 };

    // Set frame rates to maximum the Kinect supports
    m_vidFrameRate = 30;
    m_depthFrameRate = 30;
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

    DeleteObject( m_vidImage );
    DeleteObject( m_depthImage );

    std::free( m_vidBuffer );
    std::free( m_depthBuffer );

    cvReleaseImage( &m_cvVidImage );
    cvReleaseImage( &m_cvDepthImage );
    cvReleaseImage( &m_cvBitmapDest );

    for ( unsigned int index = m_calibImages.size() ; index > 0 ; index-- ) {
        cvReleaseImage( &m_calibImages.at( index - 1 ) );
        delete m_calibImages.at( index - 1 );
    }
    m_calibImages.clear();

    std::free( m_quad );
    plist_free( m_plistRaw );
    plist_free( m_plistProc );
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
            if ( knt_startstream( m_kinect->rgb ) == 0 ) {
                m_vidWindowMutex.lock();
                PostMessage( m_vidWindow , WM_KINECT_VIDEOSTART , 0 , 0 );
                m_vidWindowMutex.unlock();
            }
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
            if ( knt_startstream( m_kinect->depth ) == 0 ) {
                m_depthWindowMutex.lock();
                PostMessage( m_depthWindow , WM_KINECT_DEPTHSTART , 0 , 0 );
                m_depthWindowMutex.unlock();
            }
        }
    }
}

void Kinect::stopVideoStream() {
    // If the Kinect instance is valid, stop the rgb image stream if it's running
    if ( m_kinect != NULL ) {
        knt_rgb_stopstream( m_kinect->rgb );
        m_foundScreen = false;

        m_vidWindowMutex.lock();
        PostMessage( m_vidWindow , WM_KINECT_VIDEOSTOP , 0 , 0 );
        m_vidWindowMutex.unlock();
    }
}

void Kinect::stopDepthStream() {
    // If the Kinect instance is valid, stop the depth image stream if it's running
    if ( m_kinect != NULL ) {
        knt_depth_stopstream( m_kinect->depth );

        m_depthWindowMutex.lock();
        PostMessage( m_depthWindow , WM_KINECT_DEPTHSTOP , 0 , 0 );
        m_depthWindowMutex.unlock();
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

void Kinect::setVideoStreamFPS( unsigned int fps ) {
    m_vidFrameRate = fps;
}

void Kinect::setDepthStreamFPS( unsigned int fps ) {
    m_depthFrameRate = fps;
}

void Kinect::registerVideoWindow( HWND window ) {
    m_vidWindowMutex.lock();
    m_vidWindow = window;
    m_vidWindowMutex.unlock();
}

void Kinect::registerDepthWindow( HWND window ) {
    m_depthWindowMutex.lock();
    m_depthWindow = window;
    m_depthWindowMutex.unlock();
}

void Kinect::displayVideo( HWND window , int x , int y , HDC deviceContext ) {
    display( window , x , y , m_vidImage , m_vidDisplayMutex , deviceContext );
}

void Kinect::displayDepth( HWND window , int x , int y , HDC deviceContext ) {
    display( window , x , y , m_depthImage , m_depthDisplayMutex , deviceContext );
}

bool Kinect::saveVideo( const std::string& fileName ) const {
    unsigned char imageData[ImageVars::width * ImageVars::height * 4];

    // Copy OpenCV BGR image to RGB array
    for ( unsigned int startI = 0 , endI = 0 ; endI < ImageVars::width * ImageVars::height * 4 ; startI += 3 , endI += 4 ) {
        imageData[endI+0] = m_vidBuffer[startI+0];
        imageData[endI+1] = m_vidBuffer[startI+1];
        imageData[endI+2] = m_vidBuffer[startI+2];
        imageData[endI+3] = 255;
    }

    sf::Image imageBuffer;
    imageBuffer.create( ImageVars::width , ImageVars::height , imageData );

    return imageBuffer.saveToFile( fileName );
}

bool Kinect::saveDepth( const std::string& fileName ) const {
    unsigned char imageData[ImageVars::width * ImageVars::height * 4];

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

void Kinect::setCalibImage( Processing::ProcColor colorWanted ) {
    if ( isVideoStreamRunning() ) {
        char* fileName = (char*)std::malloc(16);

        m_vidImageMutex.lock();
        std::memcpy( m_calibImages[colorWanted]->imageData , m_vidBuffer , ImageVars::width * ImageVars::height * 3 );
        m_vidImageMutex.unlock();

        std::sprintf( fileName , "calib-%d.png" , colorWanted );
        saveVideo( fileName );
        std::free( fileName );
    }
}

void Kinect::calibrate() {
    IplImage* redCalib = NULL;
    IplImage* greenCalib = NULL;
    IplImage* blueCalib = NULL;

    if ( isEnabled( Red ) ) {
        redCalib = m_calibImages[Red];
    }

    if ( isEnabled( Green ) ) {
        greenCalib = m_calibImages[Green];
    }

    if ( isEnabled( Blue ) ) {
        blueCalib = m_calibImages[Blue];
    }

    std::free( m_quad );
    plist_free( m_plistRaw );
    plist_free( m_plistProc );

    // If image was disabled, NULL is passed instead, so it's ignored

    /* Use the calibration images to locate a quadrilateral in the
     * image which represents the screen (returns 1 on failure)
     */
    saveRGBimage(redCalib, (char *)"redCalib-rgb.data");
    saveRGBimage(blueCalib, (char *)"blueCalib-rgb.data");
    findScreenBox( blueCalib , greenCalib , redCalib , &m_quad );

    // If no box was found, m_quad will be NULL
    m_foundScreen = (m_quad != NULL);
}

void Kinect::lookForCursors() {
    // We can't look for cursors if we never found a screen on which to look
    if ( m_foundScreen ) {
        m_vidImageMutex.lock();

        /* Create a list of points which represent potential locations
           of the pointer */
        IplImage* tempImage = RGBtoIplImage( reinterpret_cast<unsigned char*>(m_vidBuffer) , ImageVars::width , ImageVars::height );
        findImageLocation( tempImage , &m_plistRaw , FLT_GREEN );
        cvReleaseImage( &tempImage );

        m_vidImageMutex.unlock();

        /* Identify the points in m_plistRaw which are located inside
           the boundary defined by m_quad, and scale them to the size
           of the computer's main screen. */
        if ( m_plistRaw != NULL ) {
            findScreenLocation( m_plistRaw , &m_plistProc , m_quad , GetSystemMetrics( SM_CXSCREEN ) , GetSystemMetrics( SM_CYSCREEN ) );

            if ( m_plistProc != NULL ) {
                moveMouse( &m_input , m_plistProc->data.x , m_plistProc->data.y , MOUSEEVENTF_ABSOLUTE );
            }
        }
    }
}

void Kinect::enableColor( ProcColor color ) {
    if ( !isEnabled( color ) ) {
        m_enabledColors |= ( 1 << color );
        m_calibImages[color] = cvCreateImage( m_imageSize , IPL_DEPTH_8U , 3 );
    }
}

void Kinect::disableColor( ProcColor color ) {
    if ( isEnabled( color ) ) {
        m_enabledColors &= ~( 1 << color );
        cvReleaseImage( &m_calibImages[color] );
        m_calibImages[color] = NULL;
    }
}

bool Kinect::isEnabled( ProcColor color ) {
    return m_enabledColors & ( 1 << color );
}

void Kinect::newVideoFrame( struct nstream_t* streamObject , void* classObject ) {
    Kinect* kinectPtr = reinterpret_cast<Kinect*>( classObject );

    kinectPtr->m_vidImageMutex.lock();

    // Copy image to internal buffer (3 channels)
    std::memcpy( kinectPtr->m_vidBuffer , kinectPtr->m_kinect->rgb->buf , ImageVars::width * ImageVars::height * 3 );

    kinectPtr->m_vidDisplayMutex.lock();

    DeleteObject( kinectPtr->m_vidImage ); // free previous image if there is one

    // B , G , R , A
    CvScalar lineColor = cvScalar( 0x00 , 0xFF , 0x00 , 0xFF );

    kinectPtr->m_cvVidImage = RGBtoIplImage( reinterpret_cast<unsigned char*>( kinectPtr->m_vidBuffer ) , ImageVars::width , ImageVars::height );

    if ( kinectPtr->m_foundScreen ) {
        // Draw lines to show user where the screen is
        cvLine( kinectPtr->m_cvVidImage , kinectPtr->m_quad->point[0] , kinectPtr->m_quad->point[1] , lineColor , 2 , 8 , 0 );
        cvLine( kinectPtr->m_cvVidImage , kinectPtr->m_quad->point[1] , kinectPtr->m_quad->point[2] , lineColor , 2 , 8 , 0 );
        cvLine( kinectPtr->m_cvVidImage , kinectPtr->m_quad->point[2] , kinectPtr->m_quad->point[3] , lineColor , 2 , 8 , 0 );
        cvLine( kinectPtr->m_cvVidImage , kinectPtr->m_quad->point[3] , kinectPtr->m_quad->point[0] , lineColor , 2 , 8 , 0 );

        //kinectPtr->lookForCursors(); // FIXME
    }

    // Perform conversion from RGBA to BGRA for use as image data in CreateBitmap
    cvCvtColor( kinectPtr->m_cvVidImage , kinectPtr->m_cvBitmapDest , CV_RGB2BGRA );

    kinectPtr->m_vidImage = CreateBitmap( ImageVars::width , ImageVars::height , 1 , 32 , kinectPtr->m_cvBitmapDest->imageData );

    cvReleaseImage( &kinectPtr->m_cvVidImage );

    kinectPtr->m_vidDisplayMutex.unlock();

    kinectPtr->m_vidImageMutex.unlock();

    // Limit video frame rate
    if ( 1.f / kinectPtr->m_vidFrameTime.getElapsedTime().asSeconds() < kinectPtr->m_vidFrameRate ) {
        kinectPtr->m_vidWindowMutex.lock();
        kinectPtr->displayVideo( kinectPtr->m_vidWindow , 0 , 0 );
        kinectPtr->m_vidWindowMutex.unlock();

        kinectPtr->m_vidFrameTime.restart();
    }
}

void Kinect::newDepthFrame( struct nstream_t* streamObject , void* classObject ) {
    Kinect* kinectPtr = reinterpret_cast<Kinect*>( classObject );

    kinectPtr->m_depthImageMutex.lock();

    // Copy image to internal buffer (2 bytes per pixel)
    std::memcpy( kinectPtr->m_depthBuffer , kinectPtr->m_kinect->depth->buf , ImageVars::width * ImageVars::height * 2 );

    double depth = 0.0;
    unsigned short depthVal = 0;
    for ( unsigned int index = 0 ; index < ImageVars::width * ImageVars::height ; index++ ) {
        depthVal = (unsigned short) *((unsigned short*)(kinectPtr->m_depthBuffer + 2 * index));

        depth = Kinect::rawDepthToMeters( depthVal );

        // Assign values from 0 to 5 meters with a shade from black to white
        kinectPtr->m_cvDepthImage->imageData[4 * index + 0] = 255.f * depth / 5.f;
        kinectPtr->m_cvDepthImage->imageData[4 * index + 1] = 255.f * depth / 5.f;
        kinectPtr->m_cvDepthImage->imageData[4 * index + 2] = 255.f * depth / 5.f;
    }

    // Make HBITMAP from pixel array
    kinectPtr->m_depthDisplayMutex.lock();

    DeleteObject( kinectPtr->m_depthImage ); // free previous image if there is one
    kinectPtr->m_depthImage = CreateBitmap( ImageVars::width , ImageVars::height , 1 , 32 , kinectPtr->m_cvDepthImage->imageData );

    kinectPtr->m_depthDisplayMutex.unlock();

    kinectPtr->m_depthImageMutex.unlock();

    // Limit depth image stream frame rate
    if ( 1.f / kinectPtr->m_depthFrameTime.getElapsedTime().asSeconds() < kinectPtr->m_depthFrameRate ) {
        kinectPtr->m_depthWindowMutex.lock();
        kinectPtr->displayDepth( kinectPtr->m_depthWindow , 0 , 0 );
        kinectPtr->m_depthWindowMutex.unlock();

        kinectPtr->m_depthFrameTime.restart();
    }
}

void Kinect::display( HWND window , int x , int y , HBITMAP image , sf::Mutex& displayMutex , HDC deviceContext ) {
    displayMutex.lock();

    if ( image != NULL ) {
        // Create offscreen DC for image to go on
        HDC imageHdc = CreateCompatibleDC( NULL );

        // Put the image into the offscreen DC and save the old one
        HBITMAP imageBackup = static_cast<HBITMAP>( SelectObject( imageHdc , image ) );

        // Stores DC of window to which to draw
        HDC windowHdc = deviceContext;

        // Stores whether or not the window's HDC was provided for us
        bool neededHdc = ( deviceContext == NULL );

        // If we don't have the window's device context yet
        if ( neededHdc ) {
            windowHdc = GetDC( window );
        }

        // Load image to real BITMAP just to retrieve its dimensions
        BITMAP tempBMP;
        GetObject( image , sizeof( BITMAP ) , &tempBMP );

        // Copy image from offscreen DC to window's DC
        BitBlt( windowHdc , x , y , tempBMP.bmWidth , tempBMP.bmHeight , imageHdc , 0 , 0 , SRCCOPY );

        if ( neededHdc ) {
            // Release window's HDC if we needed to get one earlier
            ReleaseDC( window , windowHdc );
        }

        // Restore old image
        SelectObject( imageHdc , imageBackup );

        // Delete offscreen DC
        DeleteDC( imageHdc );
    }

    displayMutex.unlock();
}

char* Kinect::RGBtoBITMAPdata( const char* imageData , unsigned int width , unsigned int height ) {
    char* bitmapData = (char*)std::malloc( width * height * 4 );

    /* ===== Convert RGB to BGRA before displaying the image ===== */
    /* Swap R and B because Win32 expects the color components in the
     * opposite order they are currently in
     */

    for ( unsigned int startI = 0 , endI = 0 ; endI < width * height * 4 ; startI += 3 , endI += 4 ) {
        bitmapData[endI+0] = imageData[startI+2];
        bitmapData[endI+1] = imageData[startI+1];
        bitmapData[endI+2] = imageData[startI+0];
    }
    /* =========================================================== */

    return bitmapData;
}

double Kinect::rawDepthToMeters( unsigned short depthValue ) {
    if ( depthValue < 2047 ) {
        return 1.f /
                (static_cast<double>(depthValue) * -0.0030711016 + 3.3309495161 );
    }

    return 0.0;
}
