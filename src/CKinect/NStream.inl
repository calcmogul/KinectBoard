/*
 * This module provides a high(-ish) level interface for connecting streams
 * of frames between different modules of the program. It is implemented in
 * kinect.c, udpout.c, and many other modules. It provides a framework for
 * streaming based on the structure NStream, and implements double
 * buffering, and callbacks for various uses.
 */

/*
 * Initialize a new NStream struct (usually used on the sender side) and
 * it's elements and buffers.
 *
 * Many of it's arguments are various parameters that correspond with members
 * of the struct NStream.
 *
 * A pointer to the new NStream is returned.
 *
 * width: The width of the image in pixels.
 * height: The height of the image in pixels.
 * depth: The number of bytes per pixel. (ie, 3 for a bit depth of 24)
 * startstream: The function called by the receiving end of the stream
 *              to request that the stream be started.
 * stopstream: The function called by the receiving end of the stream
 *             to request that the stream be stopped.
 * ih: An internal handle to be used by the sending side of the stream
 *     for whatever purpose it would like. Normally, ih would contain
 *     a pointer to an instance of a structure which contains
 *     additional state information about the module implementing the
 *     sending side of NStream.
 */
template <class T>
NStream<T>::NStream(int width, int height, int depth,
                    int (T::*startStream)(NStream<T>&),
                    int (T::*stopStream)(), T* ih) {
    imgWidth = width;
    imgHeight = height;
    imgDepth = depth;

    bufSize = imgWidth * imgHeight * imgDepth;

    buf0 = std::make_unique<uint8_t[]>(bufSize);
    buf1 = std::make_unique<uint8_t[]>(bufSize);

    buf = buf0.get();

    this->startStream = startStream;
    this->stopStream = stopStream;
    this->ih = ih;
}
