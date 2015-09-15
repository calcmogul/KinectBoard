/*
 * This module provides a high(-ish) level interface for connecting streams
 * of frames between different modules of the program. It is implemented in
 * kinect.c, udpout.c, and many other modules. It provides a framework for
 * streaming based on the structure nstream_t, and implements double
 * buffering, and callbacks for various uses.
 */

/*
 * Initialize a new nstream_t struct (usually used on the sender side) and
 * it's elements and buffers.
 *
 * Many of it's arguments are various parameters that correspond with members
 * of the struct nstream_t.
 *
 * A pointer to the new nstream_t is returned.
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
 *     sending side of nstream.
 */
template <class T>
nstream<T>::nstream(int width, int height, int depth, int (T::*startstream)(nstream<T>&),
                 int (T::*stopstream)(), T* ih) {
    imgwidth = width;
    imgheight = height;
    imgdepth = depth;

    bufsize = imgwidth * imgheight * imgdepth;

    buf0 = std::make_unique<uint8_t[]>(bufsize);
    buf1 = std::make_unique<uint8_t[]>(bufsize);

    buf = buf0.get();

    this->startstream = startstream;
    this->stopstream = stopstream;

    this->ih = ih;
}
