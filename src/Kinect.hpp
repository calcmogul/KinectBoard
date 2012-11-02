//=============================================================================
//File Name: Kinect.hpp
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

// TODO Reliable way to detect if Kinect is still connected and functioning

#ifndef KINECT_HPP
#define KINECT_HPP

#include "CKinect/kinect.h"

#include <opencv2/core/core.hpp>

class Kinect {
public:
	Kinect();
	virtual ~Kinect();

	// Retrieves data from Kinect for new image
	void fillImage();

	// Returns true if there is a Kinect connected to the USB port and available to use
	bool isConnected();

	// Waits for the next image to be received and processed before continuing
	// "waitForImg" will make this call wait for a new image if true
	void processImage( bool waitForImg );

protected:
	struct knt_inst_t* m_kinect;

	// OpenCV variables
	IplImage* m_cvImage;
	IplImage* m_cvDestImage;

	cv::Mat m_imageChannelsIN[3];
	cv::Mat m_imageChannelsOUT[3];

private:
	bool m_connected;

	// Called when a new image is received (swaps the image buffer)
	static void newFrame( struct nstream_t* streamObject , void* classObject );
};

#endif // KINECT_HPP
