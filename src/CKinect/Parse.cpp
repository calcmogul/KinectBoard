/* Functions for idenfiying the screen and pointer in
   images captured by the Micrsoft Kinect. */

#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <cmath>

#include "Parse.hpp"

/* Determines the quadrant point is in, if the origin is in the center of the
 * quadrilateral specified by quad. This is used by the sortquad function.
 */
int quad_getquad(Quad& quad, CvPoint point) {
    int mpx, mpy;
    int px, py;

    /* get the point in the middle of the quadrilateral by
       averaging it's four points */
    mpx = quad.point[0].x;
    mpx += quad.point[1].x;
    mpx += quad.point[2].x;
    mpx += quad.point[3].x;

    mpy = quad.point[0].y;
    mpy += quad.point[1].y;
    mpy += quad.point[2].y;
    mpy += quad.point[3].y;

    mpx /= 4;
    mpy /= 4;

    /* locate the quadrant point is in */
    px = point.x;
    py = point.y;

    /* transform the point */
    px -= mpx;
    py -= mpy;

    /* this assumes px != 0 && py != 0 */

    /* first quadrant */
    if (px < 0 && py < 0) {
        return 0;
    }

    // second quadrant
    if (px < 0 && py > 0) {
        return 1;
    }

    // third quadrant
    if (px > 0 && py > 0) {
        return 2;
    }

    // fourth quadrant
    if (px > 0 && py < 0) {
        return 3;
    }

    return 0;
}

// Re-orders the points in a quadrilateral in a counter-clockwise manner
void sortquad(Quad& quad_in) {
    std::list<QuadSort> sortList;

    // Create the array of structs to be sorted
    for (int i = 0; i < 4; i++) {
        QuadSort temp{quad_in.point[i], quad_getquad(quad_in, quad_in.point[i]),
                      0};
        sortList.push_back(temp);
    }

    // Sort the list
    sortList.sort([](const QuadSort& arg0, const QuadSort& arg1) {
        return arg0.quadrant < arg1.quadrant;
    });

    // Rearrange the input array
    int i = 0;
    for (auto& elem : sortList) {
        quad_in.point[i] = elem.point;
        i++;
    }
}

/* Filters an image for a channel, and fills a pointer to an image
   with the monochrome result. channel tells which channel to filter.
   Valid values are:
   FLT_RED
   FLT_GREEN
   FLT_BLUE
*/

int imageFilter(IplImage* image, IplImage** product, int channel) {
        IplImage *hsvimage;
        IplImage *tmp0;
        IplImage *tmp1;

        tmp0 = cvCreateImage(cvGetSize(image), 8, 1);
        tmp1 = cvCreateImage(cvGetSize(image), 8, 1);

        hsvimage = cvCreateImage(cvGetSize(image), 8, 3);
        cvCvtColor(image, hsvimage, CV_RGB2HSV);

        switch(channel){
        case FLT_RED:
            cvInRangeS(hsvimage, cvScalar(0, 128, 128, 255),
                cvScalar(10, 255, 255, 255), tmp1);

            cvInRangeS(hsvimage, cvScalar(150, 128, 128, 255),
                cvScalar(180, 255, 255, 255), tmp0);

            cvOr(tmp0, tmp1, tmp0, nullptr);
            break;
        case FLT_GREEN:
            cvInRangeS(hsvimage, cvScalar(43, 128, 128, 255),
                cvScalar(70, 255, 255, 255), tmp0);
            break;
        case FLT_BLUE:
            cvInRangeS(hsvimage, cvScalar(99, 64, 64, 255),
                cvScalar(133, 255, 255, 255), tmp0);

            break;
        }

        *product = tmp0;
        cvReleaseImage(&tmp1);
        cvReleaseImage(&hsvimage);

        return 0;
}

#if 0
int
imageFilter(IplImage *image, IplImage **product, int channel)
{

    IplImage *red;
    IplImage *green;
    IplImage *blue;
    IplImage *tmp0;
    IplImage *tmp1;

    CvSize size;

    if(image == nullptr || product == nullptr)
        return 1;

    /* make sure they passed a valid value into channel */
    if(!(channel == FLT_RED || channel == FLT_GREEN || channel == FLT_BLUE))
         return 1;

    /* create all the temporary images to use */
    size.width = image->width;
    size.height = image->height;

    red = cvCreateImage(size, 8, 1);
    green = cvCreateImage(size, 8, 1);
    blue = cvCreateImage(size, 8, 1);
    tmp0 = cvCreateImage(size, 8, 1);
    tmp1 = cvCreateImage(size, 8, 1);
    cvSplit(image, blue, green, red, nullptr);

    /* threshold */
    cvThreshold(red, red, 50, 255,
        ((channel == FLT_GREEN) ? CV_THRESH_BINARY:
        CV_THRESH_BINARY_INV));
    cvThreshold(green, green, 50, 255,
        ((channel == FLT_GREEN) ? CV_THRESH_BINARY:
        CV_THRESH_BINARY_INV));
    cvThreshold(blue, blue, 50, 255,
        ((channel == FLT_GREEN) ? CV_THRESH_BINARY:
        CV_THRESH_BINARY_INV));

    /*
    cvAdaptiveThreshold(red, red, 255,
        CV_ADAPTIVE_THRESH_MEAN_C,
        ((channel == FLT_GREEN) ? CV_THRESH_BINARY:
        CV_THRESH_BINARY_INV), 5, 5);
    cvAdaptiveThreshold(green, green, 255,
        CV_ADAPTIVE_THRESH_MEAN_C,
        ((channel == FLT_GREEN) ? CV_THRESH_BINARY:
        CV_THRESH_BINARY_INV), 5, 5);
    cvAdaptiveThreshold(blue, blue, 255,
        CV_ADAPTIVE_THRESH_MEAN_C,
        ((channel == FLT_GREEN) ? CV_THRESH_BINARY:
        CV_THRESH_BINARY_INV), 5, 5);
    */

    /* The three together to yeild a binary image containing only
       the specified colour. tmp0 is the result.  */
    cvAnd(red, green, tmp0, nullptr);
    cvAnd(tmp0, blue, tmp1, nullptr);

    /* we're done with most of this */
    cvReleaseImage(&red);
    cvReleaseImage(&green);
    cvReleaseImage(&blue);

    cvReleaseImage(&tmp0);
    /* cvReleaseImage(&tmp1); */

    *product = tmp1;

    return 0;
}
#endif

void saveRGBimage(IplImage* image, char* path) {
    cvSaveImage(path, image, nullptr);
}

/* Takes calibration images of red, green, and blue boxes, and finds a
 * quadrilateral that represents the screen. If any of the calibration image
 * arguments are nullptr, they will be ignored.
 */
Quad findScreenBox(IplImage* redimage, IplImage* greenimage, IplImage* blueimage) {
    Quad quad;

    IplImage* redfilter;
    IplImage* greenfilter;
    IplImage* bluefilter;

    CvSize size;

    /* we should probably check that all three images coming in are the same
     * size
     */
    size.width = redimage->width;
    size.height = redimage->height;

    IplImage* tmp0 = cvCreateImage(size, 8, 1);

    // filter the images
    if (redimage != nullptr) {
        imageFilter(redimage, &redfilter, FLT_RED);
        // cvDilate(redfilter, redfilter, nullptr, 2);
        cvSaveImage("redCalib-out.png", redfilter, nullptr); // TODO
    }
    if (greenimage != nullptr) {
        imageFilter(greenimage, &greenfilter, FLT_GREEN);
        // cvDilate(greenfilter, greenfilter, nullptr, 2);
        cvSaveImage("greenCalib-out.png", greenfilter, nullptr); // TODO
    }
    if (blueimage != nullptr) {
        imageFilter(blueimage, &bluefilter, FLT_BLUE);
        // cvDilate(bluefilter, bluefilter, nullptr, 2);
        cvSaveImage("blueCalib-out.png", bluefilter, nullptr); // TODO
    }

    // and the three images together
    std::memset(tmp0->imageData, 0xff, size.width * size.height);
    if (redimage != nullptr) {
        cvAnd(tmp0, redfilter, tmp0, nullptr);
    }
    if (greenimage != nullptr) {
        cvAnd(tmp0, greenfilter, tmp0, nullptr);
    }
    if (blueimage != nullptr) {
        cvAnd(tmp0, bluefilter, tmp0, nullptr);
    }

    cvDilate(tmp0, tmp0, nullptr, 2);

    cvSaveImage("calibCombined-out.png", tmp0, nullptr); // TODO

    /* only the calibration quadrilateral should be in tmp0, now we need to find
     * its points
     */

    /* Find the contour. Note that we should check for the presence of other
     * contours that may be the test pattern.
     */
    CvMemStorage* storage = cvCreateMemStorage(0);
    CvContourScanner scanner = cvStartFindContours(tmp0, storage,
                                                   sizeof(CvContour),
                                                   CV_RETR_LIST,
                                                   CV_CHAIN_APPROX_SIMPLE,
                                                   CvPoint(0, 0));

    CvSeq* ctr;
    while ((ctr = cvFindNextContour(scanner)) != nullptr) {
        // approximate the polygon, and find the points
        ctr = cvApproxPoly(ctr, sizeof(CvContour), storage, CV_POLY_APPROX_DP,
                           10, 0);

        if (ctr == nullptr || ctr->total != 4) {
            continue;
        }

        // extract the points
        for (int i = 0; i < 4; i++) {
            quad.point[i] = *CV_GET_SEQ_ELEM(CvPoint, ctr, i);
        }
        quad.validQuad = true;
        break; // if it succeeds, we want to do it once
    }

    // clean up from finding our contours
    cvEndFindContours(&scanner);
    cvClearMemStorage(storage);
    cvReleaseMemStorage(&storage);

    // free the temporary images
    if (redimage != nullptr) {
        cvReleaseImage(&redfilter);
    }
    if (greenimage != nullptr) {
        cvReleaseImage(&greenfilter);
    }
    if (blueimage != nullptr) {
        cvReleaseImage(&bluefilter);
    }

    cvReleaseImage(&tmp0);

    return quad;
}

/* Finds the x coordinate in the line with points p0 and p1 for
   a given y coordinate. */
int interpolateX(CvPoint p0, CvPoint p1, int y) {
    int num = (p1.y - p0.y);
    int denom = (p1.x - p0.x);

    // the undefined case
    if (denom == 0) {
        return p0.x;
    }

    int m = num / denom;

    /* line along the X axis */
    if (m == 0) {
        return p0.x;
    }

    int b = p0.y - (m * p0.x);
    int x = (y - b) / m;

    return x;
}

/* Finds the y coordinate in the line with points p0 and p1 for a given x
 * coordinate
 */
int interpolateY(CvPoint p0, CvPoint p1, int x) {
    int b;

    int num = p1.y - p0.y;
    int denom = p1.x - p0.x;

    // Handles line along the X axis and the undefined case
    if (num == 0 || denom == 0) {
        return p0.y;
    }

    int m = num / denom;

    b = p0.y - m * p0.x;
    return m * x + b;
}

// returns zero if point is inside quad, one if outside
int quadCheckPoint(CvPoint point, Quad& quad) {
    // should be below segment AB
    if (point.y < interpolateY(quad.point[0], quad.point[3], point.x)) {
        return 1;
    }

    // should be left of segment BC
    if (point.x > interpolateX(quad.point[2], quad.point[3], point.y)) {
        return 1;
    }

    // should be above segment CD
    if (point.y > interpolateY(quad.point[1], quad.point[2], point.x)) {
        return 1;
    }

    // should be right of AD
    if (point.x < interpolateX(quad.point[0], quad.point[1], point.y)) {
        return 1;
    }

    return 0;
}

/* Takes a list of points from findImageLocation, scales them from the
 * quadrilateral quad (representing the screen), to the screen resolution
 * specified by screenwidth and screenheight. Remember to plist_free(*plist_out)
 * when you're done with it.
 */

/* * test that points are inside quad
 * * locate point in reference to (0, 0) being the top left corner of the
 *   quadrilateral
 * * scale it to the size of the screen
 */
std::list<CvPoint> findScreenLocation(std::list<CvPoint>& plist_in,
                                      Quad& quad,
                                      int screenwidth,
                                      int screenheight) {
    int x;
    int y;
    int xoffset;
    int yoffset;

    int x_length;
    int y_length;
    int scrx;
    int scry;

    std::list<CvPoint> plist_out;

    // Sort the calibration quadrilateral's points counter-clockwise
    sortquad(quad);

    for (auto& point : plist_in) {
        // is the point within the quadrilateral?
        if (quadCheckPoint(point, quad)) {
            // it's outside the quadrilateral
            continue;
        }

        /* plist->data is the point inside quadrilateral which we need to
         * transform into the screen
         */
        x = point.x;
        y = point.y;

        /* find the distance from the origin to the corresponding points on the
         * side of the quadrilateral
         */
        xoffset = interpolateX(quad.point[0], quad.point[1], y);
        yoffset = interpolateY(quad.point[0], quad.point[3], x);

        // apply the offsets (distance to side of quadrilateral) to the point
        x -= xoffset;
        y -= yoffset;

        // ...using the distance formula
        y_length = hypot(quad.point[1].y - quad.point[0].y,
            quad.point[1].x - quad.point[0].x);
        x_length = hypot(quad.point[1].y - quad.point[2].y,
            quad.point[1].x - quad.point[2].x);

        // proportion the screen dimensions to the quadrilateral dimensions
        // (x):(quad width) == (x):(screen width)
        scrx = (x * screenwidth) / x_length;
        scry = (y * screenheight) / y_length;
        point.x = scrx;
        point.y = scry;
        if (plist_out.empty()) {
            plist_out.emplace_back(scrx, scry);
        }
    }

    return plist_out;
}

/* Creates a list of points in the image which could be the pointer. Finds areas
 * of color specified by channel. Acceptable values are the same as used by
 * imageFilter(). Remember to plist_free(*plist_out) when you're done with it.
 */
std::list<CvPoint> findImageLocation(IplImage* image, int channel) {
    CvPoint point;
    CvMemStorage* storage;
    CvContourScanner scanner;
    CvSeq* ctr;
    CvRect rect;
    IplImage* tmp1;
    std::list<CvPoint> plist;

    if (image == nullptr) {
        return plist;
    }

    // filter the green channel
    if (imageFilter(image, &tmp1, channel) != 0) {
        return plist;
    }

    // We now have an image with only the channel we want
    storage = cvCreateMemStorage(0);
    scanner = cvStartFindContours(tmp1, storage, sizeof(CvContour),
        CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

    while ((ctr = cvFindNextContour(scanner)) != nullptr) {
        // find the center of the bounding rectangle of the contour
        rect = cvBoundingRect(ctr, 0);
        if (rect.width > 4 && rect.height > 4) {
            plist.emplace_back(rect.x + rect.width / 2, rect.y + rect.height / 2);
        }

        // find the first point in the contour
        /*
        point = *CV_GET_SEQ_ELEM(CvPoint, ctr, 0);
        */
    }

    cvEndFindContours(&scanner);
    cvClearMemStorage(storage);
    cvReleaseMemStorage(&storage);

    cvReleaseImage(&tmp1);

    return plist;
}

/* Converts a raw 24bit RGB image into an OpenCV IplImage. Use
 * cvReleaseImage(IplImage**) to free.
 */
IplImage* RGBtoIplImage(uint8_t* rgbimage, int width, int height) {
    IplImage* image;

    if(rgbimage == nullptr)
        return nullptr;

    image = cvCreateImage(CvSize(width, height), 8, 3);
    std::memcpy(image->imageData, rgbimage, width * height * 3);

    return image;
}

#if 0
// Example usage
int main() {
    struct quad_t* quad;
    std::list<CvPoint> plist_raw;
    std::list<CvPoint> plist_proc;

    IplImage* r_image;
    IplImage* g_image;
    IplImage* b_image;
    IplImage* image;

    /* Load the calibration images, and an image which contains
       a pointing instrument. Normally, RGBtoIplImage would be
       used in place of cvLoadImage to translate images incoming
       from the Kinect to the IplImage format used by later
       calls. */
    r_image = cvLoadImage("r_image_rl-1.png", CV_LOAD_IMAGE_COLOR);
    assert(r_image);

    /* g_image = cvLoadImage("g_image_r.png", CV_LOAD_IMAGE_COLOR);
    assert(g_image); */
    g_image = nullptr;

    b_image = cvLoadImage("b_image_rl-1.png", CV_LOAD_IMAGE_COLOR);
    assert(b_image);

    image = cvLoadImage("image.png", CV_LOAD_IMAGE_COLOR);
    assert(image);

    /* Use the calibration images to locate a quadrilateral in the
       image which represents the screen */
    assert(!findScreenBox(r_image, g_image, b_image, &quad));

    /* The calls to findImageLocation and findScreenLocation
       are to be called repeadedly with images from the Kinect */

    /* Create a list of points which represent potential locations
       of the pointer */
    assert(!findImageLocation(image, &plist_raw, FLT_GREEN));

    /* Identinfy the points in plist_raw which are located inside
       the boundary defined by quad, and scale them to a 1366x768
       screen. */
    assert(!findScreenLocation(plist_raw, &plist_proc,
        quad, 1366, 768));

    /* plist_proc is now a list of potential cursor positions
       relative to the size of the screen. in the best case
       scenerio, it will contain only one point. */

    /* Free the quad we allocated, as well as the plists. */
    delete quad;

    /* Free all the images we created. */
    cvReleaseImage(&r_image);
    cvReleaseImage(&g_image);
    cvReleaseImage(&b_image);
    cvReleaseImage(&image);

    return 0;
}
#endif
