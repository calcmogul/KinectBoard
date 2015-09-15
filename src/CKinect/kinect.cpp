/*
 * This module provides the interface between libfreenect and
 * nstream. This provides a higher level API for accessing a
 * stream of frames from the Kinect device.
 */

#include "kinect.h"
#include "nstream.h"
#include <libfreenect/libfreenect.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <sched.h>
#include <sys/time.h>

#include <system_error>

knt::~knt() {
    /* make sure the streams are stopped first */
    rgb_stopstream();
    depth_stopstream();

    /* join to the thread: maybe this should be in the stopstream functions?
       the entire idea that stopstream can be called both from inside and
       outside of the thread is a bit jacked up. I'll probably fix this soon */
    thread.join();
}

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
void knt::rgb_cb(freenect_device* dev, void* rgbBuf, uint32_t timestamp) {
    knt& kntPtr = *static_cast<knt*>(freenect_get_user(dev));

    /* Do nothing if the stream isn't up */
    if (kntPtr.rgb.state != NSTREAM_UP) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(kntPtr.rgb.mutex);

        /* Update timestamp: should we be using the provided one? */
        /* gettimeofday(&curtime, &thistimezone); nstm->timestamp = curtime.tv_sec;
         */
        kntPtr.rgb.timestamp = timestamp;

        /* Swap buffers */
        if (kntPtr.rgb.buf == kntPtr.rgb.buf0.get()) {
            kntPtr.rgb.buf = kntPtr.rgb.buf1.get();
            freenect_set_video_buffer(dev, kntPtr.rgb.buf0.get());
        }
        else {
            kntPtr.rgb.buf = kntPtr.rgb.buf0.get();
            freenect_set_video_buffer(dev, kntPtr.rgb.buf1.get());
        }
    }

    /* call the new frame callback */
    if (kntPtr.rgb.newframe != nullptr) {
        kntPtr.rgb.newframe(kntPtr.rgb, kntPtr.rgb.callbackarg);
    }
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
void knt::depth_cb(freenect_device* dev, void* depthBuf, uint32_t timestamp) {
    knt& kntPtr = *static_cast<knt*>(freenect_get_user(dev));

    /* Do nothing if the stream isn't up */
    if (kntPtr.depth.state != NSTREAM_UP) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(kntPtr.depth.mutex);

        /* update timestamp: should we be using the provided one? */
        kntPtr.depth.timestamp = timestamp;

        /* Swap buffers */
        if (kntPtr.depth.buf == kntPtr.depth.buf0.get()) {
            kntPtr.depth.buf = kntPtr.depth.buf1.get();
            freenect_set_depth_buffer(dev, kntPtr.depth.buf0.get());
        }
        else {
            kntPtr.depth.buf = kntPtr.depth.buf0.get();
            freenect_set_depth_buffer(dev, kntPtr.depth.buf1.get());
        }
    }

    /* call the new frame callback */
    if (kntPtr.depth.newframe != nullptr) {
        kntPtr.depth.newframe(kntPtr.depth, kntPtr.depth.callbackarg);
    }
}

/*
 * User calls this to start an RGB or depth stream. Should be called
 * through the nstream_t struct
 *
 * stream: The nstream_t handle of the stream to start.
 */
int knt::startstream(nstream<knt>& stream) {
    /* You can't start a stream that's already started */
    if (stream.state != NSTREAM_DOWN)
        return 1;

    threadrunning_mutex.lock();
    if (!threadrunning) {
        /* The main worker thread isn't running yet. Start it. */
        threadrunning = true;
        thread = std::thread(&knt::threadmain, this);

        {
            std::unique_lock<std::mutex> lock(threadrunning_mutex);
            threadcond.wait(lock);
        }
        if (!threadrunning) {
            /* the kinect failed to initialize */
            threadrunning_mutex.unlock();
            return 1;
        }

    }
    threadrunning_mutex.unlock();

    stream.state = NSTREAM_UP;

    /* Do the callback */
    if (stream.streamstarting != nullptr) {
        stream.streamstarting(stream, stream.callbackarg);
    }

    return 0;
}

/*
 * User calls this to stop the RGB stream. Note that RGB and depth streams
 * have seperate stop functions, unlike start functions.
 *
 * stream: The nstream_t handle of the stream to stop.
 */
int knt::rgb_stopstream() {
    if (rgb.state != NSTREAM_UP)
        return 1;

    if (depth.state != NSTREAM_UP) {
        /* We're servicing no other streams, shut down the thread */
        threadrunning = false;
    }

    rgb.state = NSTREAM_DOWN;

    /* Do the callback */
    if (rgb.streamstopping != nullptr) {
        rgb.streamstopping(rgb, rgb.callbackarg);
    }

    return 0;
}

/*
 * User calls this to stop the depth stream. Note that depth and depth streams
 * have seperate stop functions, unlike start functions.
 *
 * stream: The nstream_t handle of the stream to stop
 */
int knt::depth_stopstream() {
    if (depth.state != NSTREAM_UP)
        return 1;

    if (rgb.state != NSTREAM_UP) {
        /* We're servicing no other streams, shut down the thread */
        threadrunning = false;
    }

    depth.state = NSTREAM_DOWN;

    /* Do the callback */
    if (depth.streamstopping != nullptr) {
        depth.streamstopping(depth, depth.callbackarg);
    }

    return 0;
}

/*
 * Main thread function, initializes the kinect, and runs the
 * libfreenect event loop.
 *
 * in: The optional argument to the thread. Contains the knt_inst_t*.
 */
void knt::threadmain() {
    int error;
    int ndevs;
    freenect_context *f_ctx;
    freenect_device *f_dev;

    /* initialize libfreenect */
    error = freenect_init(&f_ctx, nullptr);
    if (error != 0) {
        fprintf(stderr, "failed to initialize libfreenect\n");
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    ndevs = freenect_num_devices(f_ctx);
    if (ndevs != 1) {
        fprintf(stderr, "more/less than one kinect detected\n");
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    error = freenect_open_device(f_ctx, &f_dev, 0);
    if (error != 0) {
        fprintf(stderr, "failed to open kinect device\n");
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    freenect_set_user(f_dev, this);
    freenect_set_video_callback(f_dev, rgb_cb);
    freenect_set_depth_callback(f_dev, depth_cb);

    error = freenect_set_video_mode(f_dev, freenect_find_video_mode(
                                           FREENECT_RESOLUTION_MEDIUM,
                                           FREENECT_VIDEO_RGB));
    if (error != 0) {
        fprintf(stderr, "failed to set video mode\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    error = freenect_set_depth_mode(f_dev, freenect_find_depth_mode(
                                           FREENECT_RESOLUTION_MEDIUM,
                                           FREENECT_DEPTH_11BIT));
    if (error != 0) {
        fprintf(stderr, "failed to set depth mode\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    /* Set the user buffer to buf0 */
    {
        std::lock_guard<std::mutex> lock(rgb.mutex);
        rgb.buf = rgb.buf0.get();
    }

    error = freenect_set_video_buffer(f_dev, rgb.buf1.get());
    if (error != 0) {
        fprintf(stderr, "failed to set video buffer\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    /* Set the user buffer to buf0 */
    {
        std::lock_guard<std::mutex> lock(depth.mutex);
        depth.buf = depth.buf0.get();
    }

    error = freenect_set_depth_buffer(f_dev, depth.buf1.get());
    if (error != 0) {
        fprintf(stderr, "failed to set depth buffer\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    error = freenect_start_video(f_dev);
    if (error != 0) {
        fprintf(stderr, "failed to start video stream\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    error = freenect_start_depth(f_dev);
    if (error != 0) {
        fprintf(stderr, "failed to start depth stream\n");
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    // We initialized everything successfully
    threadcond.notify_all();

    /* Turn the LED red */
    freenect_set_led(f_dev, LED_RED);

    /* Do the main loop */
    while (freenect_process_events(f_ctx) >= 0 && threadrunning) {

    }

    /* Turn the LED blinking green */
    freenect_set_led(f_dev, LED_BLINK_GREEN);

    /* Clean up */
    rgb_stopstream();
    depth_stopstream();

    freenect_stop_video(f_dev);
    freenect_stop_depth(f_dev);

    freenect_close_device(f_dev);
    freenect_shutdown(f_ctx);
}
