#ifndef _NSTREAM_H
#define _NSTREAM_H

#include <atomic>
#include <memory>
#include <mutex>
#include <cstdint>

#define NSTREAM_DOWN 0
#define NSTREAM_UP 1
#define NSTREAM_STARTING 2
#define NSTREAM_STOPPING 3

template <class T>
class nstream;

template <class T>
class nstream {
public:
    nstream(int width, int height, int depth, int (T::*startstream)(nstream<T>&),
                     int (T::*stopstream)(), T* ih);

    std::mutex mutex;

    /* the current state of the stream, either NSTREAM_UP or NSTREAM_DOWN */
    std::atomic<int> state{NSTREAM_DOWN};

    /* The height and width in pixels */
    int imgwidth;
    int imgheight;
    int imgdepth;               /* in bytes per pixel */

    /* The size of the buffers */
    unsigned int bufsize;

    /* The two buffers to swap */
    std::unique_ptr<uint8_t[]> buf0;
    std::unique_ptr<uint8_t[]> buf1;

    /* The current swapped-in buffer */
    uint8_t* buf;

    /* Callbacks */
    void *callbackarg;

    /* stream starting */
    void (*streamstarting)(nstream<T>&, void*);
    /* stream stopping */
    void (*streamstopping)(nstream<T>&, void*);
    /* new frame in buffer */
    void (*newframe)(nstream<T>&, void*);

    /* Calls */
    /* start stream */
    int (T::*startstream)(nstream<T>&);
    /* stop stream */
    int (T::*stopstream)();

    /* Handle for internal use (implemented differently in different
       applications */
    T* ih;

    /* Timestamp in seconds since the epoch */
    long timestamp = 0;
};

#include "nstream.inl"

#endif
