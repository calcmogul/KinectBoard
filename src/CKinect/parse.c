/* Functions for idenfiying the screen and pointer in
   images captured by the Micrsoft Kinect. */
  
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "parse.h"

/* Frees the memory of all the elements in a plist_t linked list */
void
plist_free(struct plist_t *plist_in)
{
    struct plist_t *plist;
    struct plist_t *plist_next;

    for(plist = plist_in; plist != NULL; plist = plist_next){
    	plist_next = plist->next;
    	free(plist);
    }

    return;
}

/* Determines the quadrant point is in, if the origin is in the center
   of the quadrilateral specified by quad. This is used by the sortquad
   function. */
int
quad_getquad(struct quad_t *quad, CvPoint point)
{
    int mpx, mpy;
    int px, py;

    if(quad == NULL)
        return 1;

    /* get the point in the middle of the quadrilateral by
       averaging it's four points */
    mpx = quad->point[0].x;
    mpx += quad->point[1].x;
    mpx += quad->point[2].x;
    mpx += quad->point[3].x;

    mpy = quad->point[0].y;
    mpy += quad->point[1].y;
    mpy += quad->point[2].y;
    mpy += quad->point[3].y;

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
    if(px < 0 && py < 0){
    	return 0;
    }

    /* second quadrant */
    if(px < 0 && py > 0){
    	return 1;
    }

    /* third quadrant */
    if(px > 0 && py > 0){
    	return 2;
    }

    /* fourth quadrant */
    if(px > 0 && py < 0){
    	return 3;
    }
    
    return 0;
}

/* The internal sorting function used by qsort(3) as used by
   sortquad() */
int
quad_sortfunc(const void *arg0, const void *arg1)
{
    const struct quadsort_t *quad0 = arg0;
    const struct quadsort_t *quad1 = arg1;

    return quad0->quadrant - quad1->quadrant;
}

/* Re-orders the points in a quadrilateral in a counter-clockwise
   manner. */
void
sortquad(struct quad_t *quad_in)
{
    int i;
    struct quadsort_t sortlist[4];

    /* create the array of structs to be sorted */
    for(i = 0; i < 4; i++){
    	sortlist[i].point = quad_in->point[i];
    	sortlist[i].quadrant = quad_getquad(quad_in,
    		quad_in->point[i]);
    }

    /* sort the list */
    qsort(sortlist, 4, sizeof(struct quadsort_t), quad_sortfunc);

    /* rearrange the input array */
    for(i = 0; i < 4; i++){
    	quad_in->point[i] = sortlist[i].point;
    }

    return;
}

#if 0
/* The internal sorting function used by qsort(3) as used by
   sortquad() */
int
quad_sortfunc(const void *arg0, const void *arg1)
{
    double tmp;

    const struct quadsort_t *quad0 = arg0;
    const struct quadsort_t *quad1 = arg1;

    tmp = quad1->angle - quad0->angle;
    if(tmp < 0) return -1;
    if(tmp > 0) return 1;
    if(tmp == 0) return 0;

    return 0;
}

/* Re-orders the points in a quadrilateral in a counter-clockwise
   manner. */
void
sortquad(struct quad_t *quad_in)
{
    int i;
    struct quadsort_t sortlist[4];

    if(quad_in == NULL)
        return 1;

    /* create the array of structs to be sorted */
    for(i = 0; i < 4; i++){
    	sortlist[i].point = quad_in->point[i];
    	sortlist[i].angle = atan(
    		(double)sortlist[i].point.y/
    		(double)sortlist[i].point.x);
    }

    /* sort the list */
    qsort(sortlist, 4, sizeof(struct quadsort_t), quad_sortfunc);

    /* rearrange the input array */
    for(i = 0; i < 4; i++){
    	quad_in->point[i] = sortlist[i].point;
    }

    return;
}
#endif

#if 0
struct plistsort_t {
    struct plist_t *plist;
    double angle;
};

int
plist_sortfunc(const void *arg0, const void *arg1)
{
    const struct plistsort_t *plist0 = arg0;
    const struct plistsort_t *plist1 = arg1;

    return plist0->angle - plist1->angle;
}

struct plist_t *
plistsort(struct plist_t *plist_in)
{
    int i;
    int listlength;
    struct plistsort_t *sortlist;

    if(plist_in == NULL)
        return 1;

    /* find out how big the list is */
    listlength = 0;
    for(plist = plist_in; plist != NULL; plist = plist->next){
    	listlength++;
    }

    /* Fill an array with information about this list for the
       sorting function to use */
    sortlist = malloc(sizeof(struct plistsort_t)*i);

    for(plist = plist_in; plist != NULL; plist = plist->next){
    	sortlist[i].plist = plist;
    	sortlist[i].angle = atan(plist->data.y/plist->data.x);
    }

    /* Engage! */
    qsort(sortlist, listlength, sizeof(struct plistsort_t),
    	plist_sortfunc);

    /* rearrange the linked list to the sorted order.
       the -1 causes every element to be touched except the last. */
    for(i = 0; i < listlength-1; i++){
    	sortlist[i].plist->next = sortlist[i+1].plist->next;
    }
    /* set the last element to NULL */
    sortlist[i].plist->next = NULL;

    free(sortlist);

    return sortlist[i].plist;
}
#endif 

/* Filters an image for a channel, and fills a pointer to an image
   with the monochrome result. channel tells which channel to filter.
   Valid values are:
   FLT_RED
   FLT_GREEN
   FLT_BLUE
*/
int
imageFilter(IplImage *image, IplImage **product, int channel)
{

    IplImage *red;
    IplImage *green;
    IplImage *blue;
    IplImage *tmp0;
    IplImage *tmp1;

    CvSize size;

    if(image == NULL || product == NULL)
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
    cvSplit(image, blue, green, red, NULL);

    /* threshold */
    cvThreshold(green, green, 150, 255,
    	(channel == FLT_GREEN) ? CV_THRESH_BINARY:
    	CV_THRESH_BINARY_INV);
    cvThreshold(red, red, 50, 255,
    	(channel == FLT_RED) ? CV_THRESH_BINARY:
    	CV_THRESH_BINARY_INV);
    cvThreshold(blue, blue, 50, 255,
    	(channel == FLT_BLUE) ? CV_THRESH_BINARY:
    	CV_THRESH_BINARY_INV);

    /* The three together to yeild a binary image containing only
       the specified colour. tmp0 is the result.  */
    cvAnd(red, green, tmp0, NULL);
    cvAnd(tmp0, blue, tmp1, NULL);

    /* we're done with most of this */
    cvReleaseImage(&red);
    cvReleaseImage(&green);
    cvReleaseImage(&blue);

    cvReleaseImage(&tmp0);
    /* cvReleaseImage(&tmp1); */

    *product = tmp1;

    return 0;
}

/* Takes calibration images of red, green, and blue boxes, and
   finds a quadrilateral that represents the screen. If any of
   the calibration image arguments are NULL, they will be ignored.
   Remember to free(*quadout) when you're done with it. */
int
findScreenBox(
    IplImage *redimage,
    IplImage *greenimage,
    IplImage *blueimage,
    struct quad_t **quadout)
{
    int i;
    struct quad_t *quad;

    IplImage *redfilter;
    IplImage *greenfilter;
    IplImage *bluefilter;
    IplImage *tmp0;

    CvMemStorage *storage;
    CvContourScanner scanner;
    CvSeq *ctr;

    CvSize size;

    if(quadout == NULL)
        return 1;

    /* we should probably check that all three images coming in
       are the same size */
    size.width = redimage->width;
    size.height = redimage->height;

    tmp0 = cvCreateImage(size, 8, 1);
    /* tmp1 = cvCreateImage(size, 8, 1); */

    /* filter the images */
    if(redimage != NULL)
    	imageFilter(redimage, &redfilter, FLT_RED);
    if(redimage != NULL)
    	imageFilter(greenimage, &greenfilter, FLT_GREEN);
    if(redimage != NULL)
    	imageFilter(blueimage, &bluefilter, FLT_BLUE);

    /* and the three images together */
    memset(tmp0->imageData, 0xff, size.width*size.height);
    if(redimage != NULL)
    	cvAnd(tmp0, redfilter, tmp0, NULL);
    if(greenimage != NULL)
    	cvAnd(tmp0, greenfilter, tmp0, NULL);
    if(blueimage != NULL)
    	cvAnd(tmp0, bluefilter, tmp0, NULL);

    /* only the calibration quadrilateral should be in tmp0, now
       we need to find it's points */

    /* Find the contour.
       Note that we should check for the presence of other contours
       that may be the test pattern. */
    storage = cvCreateMemStorage(0);
    scanner = cvStartFindContours(tmp0, storage, sizeof(CvContour),
    	CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));
    ctr = cvFindNextContour(scanner);

    /* ctr had better not be NULL here */
    if(ctr == NULL) {
    	/* clean up from finding our contours */
    	cvEndFindContours(&scanner);
    	cvClearMemStorage(storage);
    	cvReleaseMemStorage(&storage);

    	/* free the temporary images */
    	cvReleaseImage(&redfilter);
    	cvReleaseImage(&greenfilter);
    	cvReleaseImage(&bluefilter);

    	cvReleaseImage(&tmp0);

    	return 1;
    }

    /* approximate the polygon, and find the points */
    ctr = cvApproxPoly(ctr, sizeof(CvContour), storage,
    	CV_POLY_APPROX_DP, 10, 0);

    if(ctr == NULL || ctr->total != 4){
        /* failure occurred */
        /* clean up from finding our contours */
        cvEndFindContours(&scanner);
        cvClearMemStorage(storage);
        cvReleaseMemStorage(&storage);

        /* free the temporary images */
        cvReleaseImage(&redfilter);
        cvReleaseImage(&greenfilter);
        cvReleaseImage(&bluefilter);

        cvReleaseImage(&tmp0);

        return 1;
    }

    /* extract the points */
    quad = malloc(sizeof(struct quad_t));
    for(i = 0; i < 4; i++){
    	quad->point[i] = *CV_GET_SEQ_ELEM(CvPoint, ctr, i);
    }

    /* clean up from finding our contours */
    cvEndFindContours(&scanner);
    cvClearMemStorage(storage);
    cvReleaseMemStorage(&storage);

    /* free the temporary images */
    cvReleaseImage(&redfilter);
    cvReleaseImage(&greenfilter);
    cvReleaseImage(&bluefilter);

    cvReleaseImage(&tmp0);

    /* send off the output */
    *quadout = quad;

    return 0;
}

/* Function to help create linked lists of points (plist_t).
   struct plist_t *plist = NULL;
   struct plist_t *plist_start = NULL;
   for(...){
       plist = plist_mke(point, plist); 
       if(plist_start == NULL) plist_start = plist;
   }
   plist_start is the start of the linked list */
struct plist_t *
plist_mke(CvPoint data, struct plist_t *plist_in)
{
    struct plist_t *plist;

    plist = malloc(sizeof(struct plist_t));
    plist->next = NULL;
    if(plist_in != NULL)
    	plist_in->next = plist;
    plist->data = data;

    return plist;
}

/* Finds the x coordinate in the line with points p0 and p1 for
   a given y coordinate. */
int
interpolateX(CvPoint p0, CvPoint p1, int y)
{
    int m;
    int denom;
    int neum;
    int x;
    int b;

    neum = (p1.y - p0.y);
    denom = (p1.x - p0.x);

    /* the undefined case */
    if(denom == 0) return p0.x;

    m = neum/denom;

    /* line along the X axis */
    if(m == 0) return p0.x;

    b = p0.y - (m*p0.x);
    x = (y-b)/m;

    return x;
}

/* Finds the y coordinate in the line with points p0 and p1 for
   a given x coordinate. */
int
interpolateY(CvPoint p0, CvPoint p1, int x)
{
    int m;
    int denom;
    int neum;
    int y;
    int b;

    neum = (p1.y - p0.y);
    denom = (p1.x - p0.x);

    /* the undefined case */
    if(denom == 0) return p0.y;

    m = neum/denom;

    /* line along the X axis */
    if(m == 0) return p0.y;

    b = p0.y - (m*p0.x);
    y = m*x+b;

    return y;
}

/* returns zero if point is inside quad, one if outside */
int
quadCheckPoint(CvPoint point, struct quad_t *quad)
{

    if(quad == NULL)
        return 1;

    /* should be below segment AB */
    if(point.y < interpolateY(quad->point[0], quad->point[3], point.x)) return 1;

    /* should be left of segment BC */
    if(point.x > interpolateX(quad->point[2], quad->point[3],
    	point.y)) return 1;

    /* should be above segment CD */
    if(point.y > interpolateY(quad->point[1], quad->point[2],
    	point.x)) return 1;

    /* should be right of AD */ 
    if(point.x < interpolateX(quad->point[0], quad->point[1],
    	point.y)) return 1;

    return 0;
}

/* Takes a list of points from findImageLocation, scales them from
   the quadrilateral quad (reperesenting the screen), to the screen
   resolution specified by screenwidth and screenheight.
   Remember to plist_free(*plist_out) when you're done with it. */

/* Oh my...
   * test that points are inside quad
   * locate point in reference to (0, 0) being the top left corner 
     of the quadrilateral
   * scale it to the size of the screen */
int
findScreenLocation(
    struct plist_t *plist_in,
    struct plist_t **plist_out,
    struct quad_t *quad,
    int screenwidth,
    int screenheight)
{

    int x;
    int y;
    int xoffset;
    int yoffset;
    /*
    int x_farside;
    int y_farside;
    */
    int x_length;
    int y_length;
    int scrx;
    int scry;

    struct plist_t *plist;
    struct plist_t *plistmk = NULL;
    struct plist_t *plistmk_start = NULL;
    CvPoint point;

    if(plist_in == NULL || plist_out == NULL || quad == NULL)
        return 1;

    /* Sort the calibration quadrilateral's points
       counter-clockwize */
    sortquad(quad);

    for(plist = plist_in; plist != NULL; plist = plist->next){
    	/* is the point within the quadrilateral? */
    	if(quadCheckPoint(plist->data, quad)){
    		/* it's outside the quadrilateral */
    		continue;
    	}

    	/* plist->data is the point inside quadrilateral which
    	   we need to transform into the screen */
    	x = plist->data.x;
    	y = plist->data.y;

    	/* find the distance from the origin to the
    	   corresponding points on the side of the
    	   quadrilateral */
    	xoffset = interpolateX(quad->point[0],
    		quad->point[1], y);
    	yoffset = interpolateY(quad->point[0],
    		quad->point[3], x);

    	/* apply the offsets (distance to side of quadrilateral)
    	   to the point */
    	x -= xoffset;
    	y -= yoffset;

    	/* calculate width and height of quadrilateral */
    	/*
    	x_farside = interpolateX(quad->point[2],
    		quad->point[3], y);
    	y_farside = interpolateY(quad->point[1],
    		quad->point[2], x);

    	x_length = x_farside-xoffset;
    	y_length = y_farside-yoffset;
    	*/

    	/* ...using the distance formula */
    	y_length = hypot(quad->point[1].y - quad->point[0].y,
    		quad->point[1].x - quad->point[0].x);
    	x_length = hypot(quad->point[1].y - quad->point[2].y,
    		quad->point[1].x - quad->point[2].x);

    	/* proportion the screen dimensions to the quadrilateral
    	   dimensions */
    	/* (x):(quad width) == (x):(screen width) */
    	scrx = (x*screenwidth)/x_length;
    	scry = (y*screenheight)/y_length;
    	point.x = scrx;
    	point.y = scry;
    	plistmk = plist_mke(point, plistmk);
    	if(plistmk_start == NULL) plistmk_start = plistmk;
    }

    /* y = mx+b */
    /* m = (y2-y1)/(x2-x1) */

    *plist_out = plistmk_start;

    return 0;
}

/* Creates a list of points in the image which could be the pointer.
   Finds areas of color specified by channel. Acceptable values are
   the same as used by imageFilter() .
   Remember to plist_free(*plist_out) when you're done with it. */
int
findImageLocation(
    IplImage *image,
    struct plist_t **plist_out,
    int channel)
{
    CvPoint point;
    CvMemStorage *storage;
    CvContourScanner scanner;
    CvSeq *ctr;
    CvRect rect;
    IplImage *tmp1;
    struct plist_t *plist = NULL;
    struct plist_t *plist_start = NULL;

    if(image == NULL || plist_out == NULL)
        return 1;

    /* filter the green channel */
    if(imageFilter(image, &tmp1, channel) != 0) return 1;

    /* We now have an image with only the channel we want */
    storage = cvCreateMemStorage(0);
    scanner = cvStartFindContours(tmp1, storage, sizeof(CvContour),
    	CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

    while((ctr = cvFindNextContour(scanner)) != NULL){
    	/* find the center of the bounding rectangle of the
    	   contour */
    	rect = cvBoundingRect(ctr, 0);
    	if(rect.width > 4 && rect.height > 4){
    		point.x = rect.x+(rect.width/2);
    		point.y = rect.y+(rect.height/2);
    		plist = plist_mke(point, plist);
    	}


    	/* find the first point in the contour */
    	/*
    	point = *CV_GET_SEQ_ELEM(CvPoint, ctr, 0);
    	*/

    	/* we should do this better */
    	if(plist_start == NULL) plist_start = plist;
    }

    cvEndFindContours(&scanner);
    cvClearMemStorage(storage);
    cvReleaseMemStorage(&storage);

    cvReleaseImage(&tmp1);

    *plist_out = plist_start;

    return 0;
}

/* Converts a raw 24bit RGB image into an OpenCV IplImage.
   Use cvReleaseImage(IplImage **) to free. */
IplImage *
RGBtoIplImage(uint8_t *rgbimage, int width, int height)
{
    CvSize size;
    IplImage *image;

    if(rgbimage == NULL)
        return NULL;

    size.width = width;
    size.height = height;
    image = cvCreateImageHeader(size, 8, 3);

    return image;
}

#if 0

/* Example usage */
int
main()
{
    struct quad_t *quad;
    struct plist_t *plist_raw;
    struct plist_t *plist_proc;

    IplImage *r_image;
    IplImage *g_image;
    IplImage *b_image;
    IplImage *image;

    /* Load the calibration images, and an image which contains
       a pointing instrument. Normally, RGBtoIplImage would be
       used in place of cvLoadImage to translate images incoming
       from the Kinect to the IplImage format used by later
       calls. */
    r_image = cvLoadImage("r_image.png", CV_LOAD_IMAGE_COLOR);
    assert(r_image);

    g_image = cvLoadImage("g_image.png", CV_LOAD_IMAGE_COLOR);
    assert(g_image);

    b_image = cvLoadImage("b_image.png", CV_LOAD_IMAGE_COLOR);
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
    free(quad);
    plist_free(plist_raw);
    plist_free(plist_proc);

    /* Free all the images we created. */
    cvReleaseImage(&r_image);
    cvReleaseImage(&g_image);
    cvReleaseImage(&b_image);
    cvReleaseImage(&image);

    return 0;
}
#endif

/* moveMouse(input, plist_proc->data.x, plist_proc->data.y, 0); */

