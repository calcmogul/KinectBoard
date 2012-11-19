/*
 * This module provides the interface between libfreenect and
 * nstream. This provides a higher level API for accessing a
 * stream of frames from the Kinect device.
 */

#include "kinect.h"
#include "nstream.h"
#include <libfreenect.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <sched.h>
#include <sys/time.h>

/* global stuff for the callbacks */
struct nstream_t *knt_rgb_nstm;
struct nstream_t *knt_depth_nstm;

/*
 * Callback called by libfreenect each time the buffer is filled with a
 * new RGB frame
 *
 * dev: filled with a pointer the the freenect device
 * rgb: pointer to the RGB buffer
 * timestamp: POSIX timestamp of the buffer
 *
 * not safe for multiple instances because nstm has to be global
 */
void knt_rgb_cb(freenect_device * dev, void *rgb, uint32_t timestamp)
{
    struct nstream_t *nstm = knt_rgb_nstm;

    /* got RGB callback */
    /* printf("got RGB callback\n"); */

    /* Do nothing if the stream isn't up */
    if (nstm->state != NSTREAM_UP)
        return;

    /* lock mutex */
    pthread_mutex_lock(&nstm->mutex);

    /* Update timestamp: should we be using the provided one? */
    /* gettimeofday(&curtime, &thistimezone); nstm->timestamp = curtime.tv_sec; 
     */
    nstm->timestamp = timestamp;

    /* Swap buffers */
    if (nstm->buf == nstm->buf0) {
        nstm->buf = nstm->buf1;
        freenect_set_video_buffer(dev, nstm->buf0);
    } else {
        nstm->buf = nstm->buf0;
        freenect_set_video_buffer(dev, nstm->buf1);
    }

    /* unlock mutex */
    pthread_mutex_unlock(&nstm->mutex);

    /* call the new frame callback */
    if (nstm->newframe != NULL) {
        nstm->newframe(nstm, nstm->callbackarg);
    }

    return;
}

/*
 * Callback called by libfreenect each time the buffer is filled with a
 * new depth frame
 *
 * dev: filled with a pointer to the freenect device
 * rgb: pointer to the depth buffer
 * timestamp: POSIX timestamp of the buffer
 *
 * not safe for multiple instances because nstm has to be global
 */
void knt_depth_cb(freenect_device * dev, void *rgb, uint32_t timestamp)
{
    struct nstream_t *nstm = knt_depth_nstm;

    /* got depth callback */

    /* Do nothing if the stream isn't up */
    if (nstm->state != NSTREAM_UP)
        return;

    /* lock mutex */
    pthread_mutex_lock(&nstm->mutex);

    /* update timestamp: should we be using the provided one? */
    nstm->timestamp = timestamp;

    /* Swap buffers */
    if (nstm->buf == nstm->buf0) {
        nstm->buf = nstm->buf1;
        freenect_set_depth_buffer(dev, nstm->buf0);
    } else {
        nstm->buf = nstm->buf0;
        freenect_set_depth_buffer(dev, nstm->buf1);
    }

    /* unlock mutex */
    pthread_mutex_unlock(&nstm->mutex);

    /* call the new frame callback */
    if (nstm->newframe != NULL) {
        nstm->newframe(nstm, nstm->callbackarg);
    }

    return;
}

/*
 * User calls this to start an RGB or depth stream. Should be called
 * through the nstream_t struct
 *
 * stream: The nstream_t handle of the stream to start.
 */
int knt_startstream(struct nstream_t *stream)
{
    struct knt_inst_t *inst = stream->ih;

    /* You can't start a stream that's already started */
    if (stream->state != NSTREAM_DOWN)
        return 1;

    pthread_mutex_lock(&inst->threadrunning_mutex);
    if (inst->threadrunning == 0) {
        /* The main worker thread isn't running yet. Start it. */
        inst->threadrunning = 1;
        pthread_create(&inst->thread, NULL, knt_threadmain, inst);

        pthread_cond_wait(&inst->threadcond, &inst->threadrunning_mutex);
        if (inst->threadrunning == 0) {
            /* the kinect failed to initialize */
            pthread_mutex_unlock(&inst->threadrunning_mutex);
            return 1;
        }

    }
    pthread_mutex_unlock(&inst->threadrunning_mutex);

    stream->state = NSTREAM_UP;

    /* Do the callback */
    if (stream->streamstarting != NULL) {
        stream->streamstarting(stream, stream->callbackarg);
    }

    return 0;
}

/*
 * User calls this to stop the RGB stream. Note that RGB and depth streams
 * have seperate stop functions, unlike start functions.
 *
 * stream: The nstream_t handle of the stream to stop.
 */
int knt_rgb_stopstream(struct nstream_t *stream)
{
    struct knt_inst_t *inst = stream->ih;

    if (stream->state != NSTREAM_UP)
        return 1;

    if (inst->depth->state != NSTREAM_UP) {
        /* We're servicing no other streams, shut down the thread */
        inst->threadrunning = 0;
    }

    stream->state = NSTREAM_DOWN;

    /* Do the callback */
    if (stream->streamstopping != NULL) {
        stream->streamstopping(stream, stream->callbackarg);
    }

    return 0;
}

/*
 * User calls this to stop the depth stream. Note that depth and depth streams
 * have seperate stop functions, unlike start functions.
 *
 * stream: The nstream_t handle of the stream to stop
 */
int knt_depth_stopstream(struct nstream_t *stream)
{
    struct knt_inst_t *inst = stream->ih;

    if (stream->state != NSTREAM_UP)
        return 1;

    if (inst->rgb->state != NSTREAM_UP) {
        /* We're servicing no other streams, shut down the thread */
        inst->threadrunning = 0;
    }

    stream->state = NSTREAM_DOWN;

    /* Do the callback */
    if (stream->streamstopping != NULL) {
        stream->streamstopping(stream, stream->callbackarg);
    }

    return 0;
}

/*
 * Called by knt_threadmain, in the case of an error, to abort
 * starting the thread.
 *
 * inst: The kinect instance pointer
 */
void knt_threadmain_abort(struct knt_inst_t *inst)
{
    inst->threadrunning = 0;
    pthread_cond_broadcast(&inst->threadcond);

    return;
}

/*
 * Main thread function, initializes the kinect, and runs the
 * libfreenect event loop.
 *
 * in: The optional argument to the thread. Contains the knt_inst_t*.
 */
void *knt_threadmain(void *in)
{
    struct knt_inst_t *inst = in;
    int error;
    int ndevs;
    freenect_context *f_ctx;
    freenect_device *f_dev;

    /* initialize libfreenect */
    error = freenect_init(&f_ctx, NULL);
    if (error != 0) {
        fprintf(stderr, "failed to initialize libfreenect\n");
        knt_threadmain_abort(inst);
        pthread_detach(inst->thread);
        pthread_exit(NULL);
    }

    ndevs = freenect_num_devices(f_ctx);
    if (ndevs != 1) {
        fprintf(stderr, "more/less than one kinect detected\n");
        freenect_shutdown(f_ctx);
        knt_threadmain_abort(inst);
        pthread_detach(inst->thread);
        pthread_exit(NULL);
    }

    error = freenect_open_device(f_ctx, &f_dev, 0);
    if (error != 0) {
        fprintf(stderr, "failed to open kinect device\n");
        freenect_shutdown(f_ctx);
        knt_threadmain_abort(inst);
        pthread_detach(inst->thread);
        pthread_exit(NULL);
    }

    freenect_set_video_callback(f_dev, knt_rgb_cb);
    freenect_set_depth_callback(f_dev, knt_depth_cb);

    error =
        freenect_set_video_mode(f_dev,
                                freenect_find_video_mode
                                (FREENECT_RESOLUTION_MEDIUM,
                                 FREENECT_VIDEO_RGB));
    if (error != 0) {
        fprintf(stderr, "failed to set video mode\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        knt_threadmain_abort(inst);
        pthread_detach(inst->thread);
        pthread_exit(NULL);
    }

    error =
        freenect_set_depth_mode(f_dev,
                                freenect_find_depth_mode
                                (FREENECT_RESOLUTION_MEDIUM,
                                 FREENECT_DEPTH_11BIT));
    if (error != 0) {
        fprintf(stderr, "failed to set depth mode\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        knt_threadmain_abort(inst);
        pthread_detach(inst->thread);
        pthread_exit(NULL);
    }

    /* Set the user buffer to buf0 */
    pthread_mutex_lock(&inst->rgb->mutex);
    inst->rgb->buf = inst->rgb->buf0;
    pthread_mutex_unlock(&inst->rgb->mutex);

    error = freenect_set_video_buffer(f_dev, inst->rgb->buf1);
    if (error != 0) {
        fprintf(stderr, "failed to set video buffer\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        knt_threadmain_abort(inst);
        pthread_detach(inst->thread);
        pthread_exit(NULL);
    }

    /* Set the user buffer to buf0 */
    pthread_mutex_lock(&inst->depth->mutex);
    inst->depth->buf = inst->depth->buf0;
    pthread_mutex_unlock(&inst->depth->mutex);

    error = freenect_set_depth_buffer(f_dev, inst->depth->buf1);
    if (error != 0) {
        fprintf(stderr, "failed to set depth buffer\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        knt_threadmain_abort(inst);
        pthread_detach(inst->thread);
        pthread_exit(NULL);
    }

    error = freenect_start_video(f_dev);
    if (error != 0) {
        fprintf(stderr, "failed to start video stream\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        knt_threadmain_abort(inst);
        pthread_detach(inst->thread);
        pthread_exit(NULL);
    }

    error = freenect_start_depth(f_dev);
    if (error != 0) {
        fprintf(stderr, "failed to start depth stream\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        knt_threadmain_abort(inst);
        pthread_detach(inst->thread);
        pthread_exit(NULL);
    }

    /* We initialized everything successfully */
    pthread_cond_broadcast(&inst->threadcond);

    /* Turn the LED red */
    freenect_set_led(f_dev, LED_RED);

    /* Do the main loop */
    while ( freenect_process_events(f_ctx) >= 0 && inst->threadrunning == 1 ) {

    }

    /* Turn the LED blinking green */
    freenect_set_led(f_dev, LED_BLINK_GREEN);

    /* Clean up */
    knt_rgb_stopstream(inst->rgb);
    knt_depth_stopstream(inst->depth);

    freenect_stop_video(f_dev);
    freenect_stop_depth(f_dev);

    freenect_close_device(f_dev);
    freenect_shutdown(f_ctx);

    pthread_detach(inst->thread);
    pthread_exit(NULL);
    return NULL;
}

/* Initialize the kinect instance structs.
 *
 * Returns the knt_inst_t * with the two nstream_t structs initialized.
 */
struct knt_inst_t *knt_init()
{
    struct knt_inst_t *inst;
    int error;

    inst = malloc(sizeof(struct knt_inst_t));
    memset(inst, 0x00, sizeof(struct knt_inst_t));

    /* initialize the thread running mutex */
    error = pthread_mutex_init(&inst->threadrunning_mutex, NULL);
    if (error != 0) {
        free(inst);
        return NULL;
    }

    /* initialize the pthread_cond object */
    error = pthread_cond_init(&inst->threadcond, NULL);
    if (error != 0) {
        free(inst);
        pthread_mutex_destroy(&inst->threadrunning_mutex);
        return NULL;
    }

    inst->rgb =
        nstream_init(640, 480, 3, knt_startstream, knt_rgb_stopstream, inst);
    if (inst->rgb == NULL) {
        free(inst);
        return NULL;
    }
    knt_rgb_nstm = inst->rgb;

    inst->depth =
        nstream_init(640, 480, 2, knt_startstream, knt_depth_stopstream, inst);
    if (inst->depth == NULL) {
        free(inst);
        nstream_destroy(inst->rgb);
        return NULL;
    }
    knt_depth_nstm = inst->depth;

    return inst;
}

/*
 * Destroys the knt_inst_t struct (and it's elements), created by
 * knt_init.
 *
 * inst: The pointer to the knt_inst_t struct to destroy.
 */
void knt_destroy(struct knt_inst_t *inst)
{
    /* make sure the streams are stopped first */
    inst->rgb->stopstream(inst->rgb);
    inst->depth->stopstream(inst->depth);

    /* join to the thread: maybe this should be in the stopstream functions?
       the entire idea that stopstream can be called both from inside and
       outside of the thread is a bit jacked up. I'll probably fix this soon */
    pthread_join(inst->thread, NULL);

    /* free everything */
    pthread_mutex_destroy(&inst->threadrunning_mutex);
    pthread_cond_destroy(&inst->threadcond);
    nstream_destroy(inst->rgb);
    nstream_destroy(inst->depth);
    free(inst);

    return;
}
