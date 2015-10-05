/* Functions for idenfiying the screen and pointer in
   images captured by the Micrsoft Kinect. */

#ifndef PARSE_HPP
#define PARSE_HPP

#include <opencv2/imgproc/imgproc.hpp>
#include <list>
#include <cstdint>

#define FLT_RED 0x01
#define FLT_GREEN 0x02
#define FLT_BLUE 0x03

class ContourList {
public:
    int maxuid;
    struct element_t* element;
};

class Element {
public:
    int uid; // unique identifier for the contour
    CvSeq* ctr; // the contour
    struct element_t* next;
};

class Quad {
public:
    cv::Point point[4];
    bool validQuad = false;
};

class QuadSort {
public:
    cv::Point point;
    int quadrant;
    int angle;
};

int quad_getquad(Quad& quad, cv::Point point);
void sortquad(Quad& quad_in);
int imageFilter(cv::Mat& image, IplImage** product, int channel);
Quad findScreenBox(cv::Mat& redimage,
                   cv::Mat& greenimage,
                   cv::Mat& blueimage,
                   // bitfield. '1' include corresponding cv::Mat in processing
                   char procImages);
int interpolateX(cv::Point p0, cv::Point p1, int y);
int interpolateY(cv::Point p0, cv::Point p1, int x);
int quadCheckPoint(cv::Point point, Quad& quad);
std::list<cv::Point> findScreenLocation(std::list<cv::Point>& plist_in,
                                      Quad& quad,
                                      int screenwidth,
                                      int screenheight);
std::list<cv::Point> findImageLocation(cv::Mat& image, int channel);

#endif // PARSE_HPP
