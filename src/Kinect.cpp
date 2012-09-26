//=============================================================================
//File Name: Kinect.cpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

#include <cstdio>
#include <iostream>
#include "Kinect.hpp"

Kinect::Kinect() {
	rgbImage = fopen( "rgbImage.rgb" , "w+b" );

	// Initialize Kinect
	kinect = knt_init();
	if ( kinect == NULL )
		exit( 1 );

	kntrgb = kinect->rgb;
	kntdepth = kinect->depth;

	/* start the rgb image stream going */
	if ( kntrgb->startstream( kntrgb ) != 0 )
		exit( 1 );

	hasImage = Empty;
}

Kinect::~Kinect() {
	knt_destroy( kinect );
	fclose( rgbImage );
}

void Kinect::fillImage() {
	if ( kntrgb->state == NSTREAM_UP ) {
		if ( hasImage == Empty )
			hasImage = Filling;

		fseek( rgbImage , 0 , SEEK_SET );

		pthread_mutex_lock( &kntrgb->mutex );
		fwrite( kntrgb->buf , kntrgb->bufsize , 1 , rgbImage );
		pthread_mutex_unlock( &kntrgb->mutex );

		fflush( rgbImage );
	}
	else { // stream finished
		if( hasImage == Filling )
			hasImage = Full;

		// Restart the Kinect stream
		kntrgb->startstream( kntrgb );
		kntdepth->startstream( kntdepth );
	}
}

bool Kinect::hasNewImage() {
	return hasImage == Full;
}
