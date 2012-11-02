//=============================================================================
//File Name: Kinect.cpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

#include <cstdio>
#include "Kinect.hpp"

#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc.hpp>

Kinect::Kinect() {
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
	m_cvImage = cvCreateImageHeader( imageSize , 8 , 3 );
	m_cvDestImage = cvCreateImageHeader( imageSize , 8 , 3 );

	std::printf( "Stream started\n" );
}

Kinect::~Kinect() {
	if ( m_kinect != NULL ) {
		knt_destroy( m_kinect );
	}
}

void Kinect::fillImage() {
    // Restart the Kinect stream
    m_kinect->rgb->startstream( m_kinect->rgb );
}

bool Kinect::isConnected() {
	pthread_mutex_lock( &m_kinect->threadrunning_mutex );
	m_connected = m_kinect->threadrunning;
	pthread_mutex_unlock( &m_kinect->threadrunning_mutex );

	return m_connected;
}

void Kinect::processImage( bool waitForImg ) {
    // Split RGB image into it's different channels
    cv::split( m_cvImage , m_imageChannelsIN );

    // Filter out dimmer parts of each image (test image should be brightest)
    for ( unsigned char i = 0 ; i < 3 ; i++ ) {
        cv::threshold( m_imageChannelsIN[i] , m_imageChannelsOUT[i] , 120 , 255 , CV_THRESH_BINARY );
    }

    // Multiply the three channels together to get a combined rectangle
    m_imageChannelsOUT[0].mul( m_imageChannelsOUT[1] );
    m_imageChannelsOUT[0].mul( m_imageChannelsOUT[2] );
}

void Kinect::newFrame( struct nstream_t* streamObject , void* classObject ) {
	std::printf( "Got new frame\n" );
	Kinect* kinectPtr = static_cast<Kinect*>( classObject );

	kinectPtr->m_cvImage->imageData = kinectPtr->m_kinect->rgb->buf;
}
