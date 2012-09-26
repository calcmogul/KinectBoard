//=============================================================================
//File Name: Kinect.hpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

#ifndef KINECT_HPP
#define KINECT_HPP

#include "CKinect/kinect.h"

struct _iobuf; // prototype of FILE struct from cstdio

class Kinect {
public:
	Kinect();
	virtual ~Kinect();

	/* gets new image from the Kinect to process
	 * @return was new image received
	 */
	void fillImage();

	// returns whether or not a new image was filled and therefore available to process
	bool hasNewImage();

	typedef enum {
		Empty = 0,
		Filling,
		Full
	} ImageStatus;

protected:
	FILE* rgbImage;
	struct knt_inst_t* kinect;
	struct nstream_t* kntrgb;
	struct nstream_t* kntdepth;

private:
	ImageStatus hasImage;
};

#endif // KINECT_HPP
