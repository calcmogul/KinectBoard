#ifndef _KINECT_H
#define _KINECT_H

#include "nstream.h"
#include <libfreenect/libfreenect.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class knt;

class knt {
public:
    virtual ~knt();

    nstream<knt> rgb{640, 480, 3, &knt::startstream, &knt::rgb_stopstream, this};
    nstream<knt> depth{640, 480, 2, &knt::startstream, &knt::depth_stopstream, this};

    std::thread thread;

    std::atomic<bool> threadrunning{false};
    std::mutex threadrunning_mutex;
    std::condition_variable threadcond;

    static void rgb_cb(freenect_device* dev, void* rgbBuf, uint32_t timestamp);
    static void depth_cb(freenect_device* dev, void* depthBuf, uint32_t timestamp);
    int startstream(nstream<knt>& stream);
    int rgb_stopstream();
    int depth_stopstream();
    void threadmain();
};

#endif
