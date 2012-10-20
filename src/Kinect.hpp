//=============================================================================
//File Name: Kinect.hpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

// TODO Reliable way to detect if Kinect is still connected and functioning

#ifndef KINECT_HPP
#define KINECT_HPP

extern "C" {
	#include "CKinect/kinect.h"
}

struct _iobuf; // prototype of FILE struct from cstdio

class Kinect {
public:
	Kinect();
	virtual ~Kinect();

	// retrieves data from Kinect for new image
	void fillImage();

	// returns true if a new image was filled and is therefore available to process
	bool hasNewImage();

	// returns true if there is a Kinect connected to the USB port and available to use
	bool isConnected();

	typedef enum {
		Empty = 0,
		Filling,
		Full
	} ImageStatus;

protected:
	FILE* m_rgbImage;
	struct knt_inst_t* m_kinect;

private:
	ImageStatus m_hasImage;
	bool m_connected;
};

#endif // KINECT_HPP
