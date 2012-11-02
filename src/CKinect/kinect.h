#ifndef _KINECT_H
#define _KINECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nstream.h"
#include <libfreenect.h>
#include <pthread.h>

struct knt_inst_t {
    struct nstream_t *rgb;
    struct nstream_t *depth;

    pthread_t thread;

    int threadrunning;
    pthread_mutex_t threadrunning_mutex;
    pthread_cond_t threadcond;
};

void knt_rgb_cb(freenect_device * dev, void *rgb, uint32_t timestamp);
void knt_depth_cb(freenect_device * dev, void *rgb, uint32_t timestamp);
int knt_startstream(struct nstream_t *stream);
int knt_rgb_stopstream(struct nstream_t *stream);
int knt_depth_stopstream(struct nstream_t *stream);
void knt_threadmain_abort(struct knt_inst_t *inst);
void *knt_threadmain(void *in);
struct knt_inst_t *knt_init();
void knt_destroy(struct knt_inst_t *inst);

#ifdef __cplusplus
}
#endif

#endif
