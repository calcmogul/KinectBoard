//=============================================================================
//File Name: Kinect.cpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

#include <cstdio>
#include "Kinect.hpp"

Kinect::Kinect() {
	m_rgbImage = fopen( "rgbImage.rgb" , "w+b" );
	m_connected = false; // may be false if something in initialization fails later

	// Initialize Kinect
	m_kinect = knt_init();

	// start the rgb image stream
	if ( m_kinect != NULL ) {
		m_kinect->rgb->startstream( m_kinect->rgb );
	}

	m_hasImage = Empty;
}

Kinect::~Kinect() {
	if ( m_kinect != NULL ) {
		knt_destroy( m_kinect );
	}
	fclose( m_rgbImage );
}

void Kinect::fillImage() {
	if ( m_kinect != NULL ) {
		if ( m_kinect->rgb->state == NSTREAM_UP ) {
			if ( m_hasImage == Empty ) {
				m_hasImage = Filling;
			}

			fseek( m_rgbImage , 0 , SEEK_SET );

			pthread_mutex_lock( &m_kinect->rgb->mutex );
			fwrite( m_kinect->rgb->buf , m_kinect->rgb->bufsize , 1 , m_rgbImage );
			pthread_mutex_unlock( &m_kinect->rgb->mutex );

			fflush( m_rgbImage );
		}
		else { // stream finished
			if( m_hasImage == Filling ) {
				m_hasImage = Full;
			}

			// Restart the Kinect stream
			m_kinect->rgb->startstream( m_kinect->rgb );
			m_kinect->depth->startstream( m_kinect->depth );
		}
	}
}

bool Kinect::hasNewImage() {
	return m_hasImage == Full;
}

bool Kinect::isConnected() {
	pthread_mutex_lock( &m_kinect->threadrunning_mutex );
	m_connected = m_kinect->threadrunning;
	pthread_mutex_unlock( &m_kinect->threadrunning_mutex );

	return m_connected;
}
