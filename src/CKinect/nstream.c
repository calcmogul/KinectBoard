/*
 * This module provides a high(-ish) level interface for connecting streams
 * of frames between different modules of the program. It is implemented in
 * kinect.c, udpout.c, and many other modules. It provides a framework for
 * streaming based on the structure nstream_t, and implements double
 * buffering, and callbacks for various uses.
 */

#include "nstream.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/*
 * Initialize a new nstream_t struct (usually used on the sender side) and
 * it's elements and buffers.
 *
 * Many of it's arguments are various parameters that coorispond with members
 * of the struct nstream_t.
 *
 * A pointer to the new nstream_t is returned. 
 *
 * width: The width of the image in pixels.
 * height: The height of the image in pixels.
 * depth: The number of bytes per pixel. (ie, 3 for a bit depth of 24)
 * startstream: The function called by the recieving end of the stream
 *              to request that the stream be started.
 * stopstream: The function called by the recieving end of the stream
 *             to request that the stream be stopped.
 * ih: An internal handle to be used by the sending side of the stream
 *     for whatever purpose it would like. Normally, ih would contain
 *     a pointer to an instance of a structure which contains
 *     additional state information about the module implementing the
 *     sending side of nstream.
 */
struct nstream_t *nstream_init(int width, int height, int depth,
                               NSSFPTR(startstream), NSSFPTR(stopstream),
                               void *ih)
{
    int error;
    struct nstream_t *nstm;

    nstm = malloc(sizeof(struct nstream_t));
    memset(nstm, 0x00, sizeof(struct nstream_t));

    /* We do this first because it's the only failure condition */
    error = pthread_mutex_init(&nstm->mutex, NULL);
    if (error != 0) {
        free(nstm);
        return NULL;
    }

    nstm->imgwidth = width;
    nstm->imgheight = height;
    nstm->imgdepth = depth;

    nstm->bufsize = nstm->imgwidth * nstm->imgheight * nstm->imgdepth;

    nstm->buf0 = malloc(nstm->bufsize);
    nstm->buf1 = malloc(nstm->bufsize);

    memset(nstm->buf0, 0x00, nstm->bufsize);
    memset(nstm->buf1, 0x00, nstm->bufsize);

    nstm->buf = nstm->buf0;

    nstm->startstream = startstream;
    nstm->stopstream = stopstream;

    nstm->ih = ih;
    nstm->timestamp = 0;

    return nstm;
}

/*
 * Frees the buffers and nstream_t structure. Usually used by the sending
 * side to clean up it's nstream_t structure after it's finished with it.
 * note that any data malloc'd by the sending module or recieving module
 * stored in callbackarg and ih will not be destroyed.
 *
 * nstm: The pointer to the nstream_t to be free'd.
 */
void nstream_destroy(struct nstream_t *nstm)
{
    pthread_mutex_destroy(&nstm->mutex);
    free(nstm->buf0);
    free(nstm->buf1);
    free(nstm);
}
