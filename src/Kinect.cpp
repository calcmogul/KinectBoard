//=============================================================================
//File Name: Kinect.cpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

#include <cstdio>
#include "Kinect.hpp"

Kinect::Kinect() {
	rgbImage = fopen( "rgbImage.rgb" , "w+b" );
	connected = false; // may be false if something in initialization fails later

	// Initialize Kinect
	kinect = knt_init();

	// start the rgb image stream
	if ( kinect != NULL ) {
		kinect->rgb->startstream( kinect->rgb );
	}

	hasImage = Empty;
}

Kinect::~Kinect() {
	if ( kinect != NULL ) {
		knt_destroy( kinect );
	}
	fclose( rgbImage );
}

void Kinect::fillImage() {
	if ( kinect != NULL ) {
		if ( kinect->rgb->state == NSTREAM_UP ) {
			if ( hasImage == Empty ) {
				hasImage = Filling;
			}

			fseek( rgbImage , 0 , SEEK_SET );

			pthread_mutex_lock( &kinect->rgb->mutex );
			fwrite( kinect->rgb->buf , kinect->rgb->bufsize , 1 , rgbImage );
			pthread_mutex_unlock( &kinect->rgb->mutex );

			fflush( rgbImage );
		}
		else { // stream finished
			if( hasImage == Filling ) {
				hasImage = Full;
			}

			// Restart the Kinect stream
			kinect->rgb->startstream( kinect->rgb );
			kinect->depth->startstream( kinect->depth );
		}
	}
}

bool Kinect::hasNewImage() {
	return hasImage == Full;
}

bool Kinect::isConnected() {
	return connected;
}
