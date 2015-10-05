#ifndef NSTREAM_HPP
#define NSTREAM_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <cstdint>

#define NSTREAM_DOWN 0
#define NSTREAM_UP 1
#define NSTREAM_STARTING 2
#define NSTREAM_STOPPING 3

template <class T>
class NStream;

template <class T>
class NStream {
public:
    NStream(int width, int height, int depth, int (T::*startstream)(NStream<T>&),
                     int (T::*stopstream)(), T* ih);

    std::mutex mutex;

    // The current state of the stream, either NSTREAM_UP or NSTREAM_DOWN
    std::atomic<int> state{NSTREAM_DOWN};

    // The height and width in pixels
    int imgWidth;
    int imgHeight;
    int imgDepth; // In bytes per pixel

    // The size of the buffers
    unsigned int bufSize;

    // The two buffers to swap
    std::unique_ptr<uint8_t[]> buf0;
    std::unique_ptr<uint8_t[]> buf1;

    // The current swapped-in buffer
    uint8_t* buf = nullptr;

    void* callbackarg = nullptr;

    void (*streamStarting)(NStream<T>&, void*) = nullptr;
    void (*streamStopping)(NStream<T>&, void*) = nullptr;
    // New frame in buffer
    void (*newFrame)(void*) = nullptr;

    // Callbacks
    int (T::*startStream)(NStream<T>&);
    int (T::*stopStream)();

    /* Handle for internal use (implemented differently in different
     * applications
     */
    T* ih;

    // Timestamp in seconds since the epoch
    long timestamp = 0;
};

#include "NStream.inl"

#endif // NSTREAM_HPP
