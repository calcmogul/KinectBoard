/* Functions for idenfiying the screen and pointer in
   images captured by the Micrsoft Kinect. */

#ifndef PARSE_HPP
#define PARSE_HPP

#include <opencv2/imgproc/imgproc_c.h>
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
    CvPoint point[4];
    bool validQuad = false;
};

class QuadSort {
public:
    CvPoint point;
    int quadrant;
    int angle;
};

int quad_getquad(Quad& quad, CvPoint point);
void sortquad(Quad& quad_in);
int imageFilter(IplImage* image, IplImage** product, int channel);
Quad findScreenBox(IplImage* redimage,
                   IplImage* greenimage,
                   IplImage* blueimage);
int interpolateX(CvPoint p0, CvPoint p1, int y);
int interpolateY(CvPoint p0, CvPoint p1, int x);
int quadCheckPoint(CvPoint point, Quad& quad);
std::list<CvPoint> findScreenLocation(std::list<CvPoint>& plist_in,
                                      Quad& quad,
                                      int screenwidth,
                                      int screenheight);
std::list<CvPoint> findImageLocation(IplImage* image, int channel);
IplImage* RGBtoIplImage(uint8_t* rgbimage, int width, int height);
void saveRGBimage(IplImage* image, char* path);

#endif // PARSE_HPP
