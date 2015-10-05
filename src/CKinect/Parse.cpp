/* Functions for idenfiying the screen and pointer in
   images captured by the Micrsoft Kinect. */

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <cmath>
#include <cstring>

#include "Parse.hpp"

/* Determines the quadrant point is in, if the origin is in the center of the
 * quadrilateral specified by quad. This is used by the sortquad function.
 */
int quad_getquad(Quad& quad, cv::Point point) {
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

int imageFilter(cv::Mat& image, cv::Mat& product, int channel) {
        cv::Mat tmp0(image.size(), CV_8UC1);
        cv::Mat tmp1(image.size(), CV_8UC1);

        cv::Mat hsvimage(image.size(), CV_8UC3);
        cv::cvtColor(image, hsvimage, CV_RGB2HSV);

        switch (channel) {
        case FLT_RED:
            cv::inRange(hsvimage, cv::Scalar(0, 128, 128, 255),
                        cv::Scalar(10, 255, 255, 255), tmp1);

            cv::inRange(hsvimage, cv::Scalar(150, 128, 128, 255),
                        cv::Scalar(180, 255, 255, 255), tmp0);

            cv::bitwise_or(tmp0, tmp1, tmp0);
            break;
        case FLT_GREEN:
            cv::inRange(hsvimage, cv::Scalar(43, 128, 128, 255),
                        cv::Scalar(70, 255, 255, 255), tmp0);
            break;
        case FLT_BLUE:
            cv::inRange(hsvimage, cv::Scalar(99, 64, 64, 255),
                        cv::Scalar(133, 255, 255, 255), tmp0);

            break;
        }

        product = tmp0;

        return 0;
}

/* Takes calibration images of red, green, and blue boxes, and finds a
 * quadrilateral that represents the screen. If any of the calibration image
 * arguments are nullptr, they will be ignored.
 */
Quad findScreenBox(cv::Mat& redimage, cv::Mat& greenimage, cv::Mat& blueimage,
                   char procImages) {
    Quad quad;

    cv::Mat redfilter;
    cv::Mat greenfilter;
    cv::Mat bluefilter;

    CvSize size(redimage.size());

    cv::Mat tmp0(redimage.size(), CV_8UC1);

    // filter the images
    if (procImages & 1) {
        imageFilter(redimage, redfilter, FLT_RED);
#if 0
        // Perform dilation
        int dilationSize = 2;
        cv::Mat element = getStructuringElement(cv::MORPH_RECT,
                                                cv::Size(2 * dilationSize + 1,
                                                         2 * dilationSize + 1),
                                                cv::Point(dilationSize,
                                                          dilationSize));
        cv::dilate(redfilter, redfilter, element);
#endif
        cv::imwrite("redCalib-out.png", redfilter); // TODO
    }
    if (procImages & (1 << 1)) {
        imageFilter(greenimage, greenfilter, FLT_GREEN);
#if 0
        // Perform dilation
        int dilationSize = 2;
        cv::Mat element = getStructuringElement(cv::MORPH_RECT,
                                                cv::Size(2 * dilationSize + 1,
                                                         2 * dilationSize + 1),
                                                cv::Point(dilationSize,
                                                          dilationSize));
        cv::dilate(greenfilter, greenfilter, element);
#endif
        cv::imwrite("greenCalib-out.png", greenfilter); // TODO
    }
    if (procImages & (1 << 2)) {
        imageFilter(blueimage, bluefilter, FLT_BLUE);
#if 0
        // Perform dilation
        int dilationSize = 2;
        cv::Mat element = getStructuringElement(cv::MORPH_RECT,
                                                cv::Size(2 * dilationSize + 1,
                                                         2 * dilationSize + 1),
                                                cv::Point(dilationSize,
                                                          dilationSize));
        cv::dilate(bluefilter, bluefilter, element);
#endif
        cv::imwrite("blueCalib-out.png", bluefilter); // TODO
    }

    // and the three images together
    std::memset(tmp0.data, 0xff, size.width * size.height);
    if (procImages & 1) {
        cv::bitwise_and(tmp0, redfilter, tmp0);
    }
    if (procImages & (1 << 1)) {
        cv::bitwise_and(tmp0, greenfilter, tmp0);
    }
    if (procImages & (1 << 2)) {
        cv::bitwise_and(tmp0, bluefilter, tmp0);
    }

    // Perform dilation
    int dilationSize = 2;
    cv::Mat element = getStructuringElement(cv::MORPH_RECT,
                                            cv::Size(2 * dilationSize + 1,
                                                     2 * dilationSize + 1),
                                            cv::Point(dilationSize,
                                                      dilationSize));
    cv::dilate(tmp0, tmp0, element);

    cv::imwrite("calibCombined-out.png", tmp0); // TODO

    /* only the calibration quadrilateral should be in tmp0, now we need to find
     * its points
     */

    /* Find the contour. Note that we should check for the presence of other
     * contours that may be the test pattern.
     */
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    std::vector<cv::Point> contour;

    cv::findContours(tmp0,
                     contours,
                     hierarchy,
                     CV_RETR_LIST,
                     CV_CHAIN_APPROX_SIMPLE);

    for (unsigned int i = 0; i < contours.size(); i++) {
        contour.clear();
        cv::approxPolyDP(contours[i], contour, 10, true);

        if (contour.size() == 4) {
            // Extract the points
            for (int i = 0; i < 4; i++) {
                quad.point[i] = contour[i];
            }
            quad.validQuad = true;
            break; // If it succeeds, we only want to do it once
        }
    }

    return quad;
}

/* Finds the x coordinate in the line with points p0 and p1 for
   a given y coordinate. */
int interpolateX(cv::Point p0, cv::Point p1, int y) {
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
int interpolateY(cv::Point p0, cv::Point p1, int x) {
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
int quadCheckPoint(cv::Point point, Quad& quad) {
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
std::list<cv::Point> findScreenLocation(std::list<cv::Point>& plist_in,
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

    std::list<cv::Point> plist_out;

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
std::list<cv::Point> findImageLocation(cv::Mat& image, int channel) {
    cv::Mat tmp0;
    std::list<cv::Point> plist;

    // filter the channel
    if (imageFilter(image, tmp0, channel) != 0) {
        return plist;
    }

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    std::vector<cv::Point> contour;

    // We now have an image with only the channel we want
    cv::findContours(tmp0,
                     contours,
                     hierarchy,
                     CV_RETR_LIST,
                     CV_CHAIN_APPROX_SIMPLE);

    cv::Rect rect;
    for (unsigned int i = 0; i < contours.size(); i++) {
        contour.clear();
        cv::approxPolyDP(contours[i], contour, 10, true);

        // find the center of the bounding rectangle of the contour
        rect = cv::boundingRect(contour);
        if (rect.width > 4 && rect.height > 4) {
            plist.emplace_back(rect.x + rect.width / 2, rect.y + rect.height / 2);
        }
    }

    return plist;
}

#if 0
// Example usage
int main() {
    struct quad_t* quad;
    std::list<cv::Point> plist_raw;
    std::list<cv::Point> plist_proc;

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
