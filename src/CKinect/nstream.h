#ifndef _NSTREAM_H
#define _NSTREAM_H

#include <pthread.h>

#define NSTREAM_DOWN 0
#define NSTREAM_UP 1
#define NSTREAM_STARTING 2
#define NSTREAM_STOPPING 3

#define NSSFPTR(n) int (*n)(struct nstream_t *)

struct nstream_t;
struct nstream_t {
    /* mutex */
    pthread_mutex_t mutex;

    /* the current state of the stream, either NSTREAM_UP or NSTREAM_DOWN */
    int state;

    /* The height and width in pixels */
    int imgwidth;
    int imgheight;
    int imgdepth;               /* in bytes per pixel */

    /* The size of the buffers */
    unsigned int bufsize;

    /* The two buffers to swap */
    char *buf0;
    char *buf1;

    /* The current swapped-in buffer */
    char *buf;

    /* Callbacks */
    void *callbackarg;

    /* stream starting */
    void (*streamstarting) (struct nstream_t *, void *);
    /* stream stopping */
    void (*streamstopping) (struct nstream_t *, void *);
    /* new frame in buffer */
    void (*newframe) (struct nstream_t *, void *);

    /* Calls */
    /* start stream */
    int (*startstream) (struct nstream_t *);
    /* stop stream */
    int (*stopstream) (struct nstream_t *);

    /* Handle for internal use (implemented differently in different
       applications */
    void *ih;

    /* Timestamp in seconds since the epoch */
    long timestamp;
};

struct nstream_t *nstream_init(int width, int height, int depth,
                               NSSFPTR(startstream), NSSFPTR(stopstream),
                               void *ih);

void nstream_destroy(struct nstream_t *nstm);

#endif
