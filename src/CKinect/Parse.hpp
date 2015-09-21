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

struct ctrlist_t {
    int maxuid;
    struct element_t* element;
};

struct element_t {
    int uid; /* unique identifier for the contour */
    CvSeq *ctr; /* the contour */
    struct element_t *next;
};

struct quad_t {
    CvPoint point[4];
};

struct quadsort_t {
    CvPoint point;
    int quadrant;
    int angle;
};

int quad_getquad(struct quad_t* quad, CvPoint point);
int quad_sortfunc(const void* arg0, const void* arg1);
void sortquad(struct quad_t* quad_in);
int imageFilter(IplImage* image, IplImage** product, int channel);
int findScreenBox(IplImage* redimage,
                  IplImage* greenimage,
                  IplImage* blueimage,
                  struct quad_t **quadout);
int interpolateX(CvPoint p0, CvPoint p1, int y);
int interpolateY(CvPoint p0, CvPoint p1, int x);
int quadCheckPoint(CvPoint point, struct quad_t *quad);
std::list<CvPoint> findScreenLocation(std::list<CvPoint>& plist_in,
                                      struct quad_t *quad,
                                      int screenwidth,
                                      int screenheight);
std::list<CvPoint> findImageLocation(IplImage *image, int channel);
IplImage* RGBtoIplImage(uint8_t *rgbimage, int width, int height);
void saveRGBimage(IplImage *image, char *path);

#endif // PARSE_HPP
