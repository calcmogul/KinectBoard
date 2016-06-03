//=============================================================================
//File Name: Kinect.inl
//Description: Manages interface with a Microsoft Kinect connected to the USB
//             port of the computer
//Author: Tyler Veness
//=============================================================================

#include <chrono>
#include <iostream>
#include <thread>
#include <cstring>
#include <cstdlib>

#include <QCursor>
#include <QImage>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

template <class T>
Kinect<T>::Kinect() {
    rgb.newFrame = newVideoFrame;
    rgb.callbackarg = this;

    depth.newFrame = newDepthFrame;
    depth.callbackarg = this;

    m_imageSize = {static_cast<int>(ImageVars::width), static_cast<int>(ImageVars::height)};

    m_cvVidImage = cv::Mat(ImageVars::height, ImageVars::width, CV_8UC3);
    m_cvDepthImage = cv::Mat(ImageVars::height, ImageVars::width, CV_8UC(4));
    m_cvBitmapDest = cv::Mat(ImageVars::height, ImageVars::width, CV_8UC(4));

    // 3 bytes per pixel
    m_vidBuffer.resize(ImageVars::width * ImageVars::height * 3);

    // Each value in the depth image is 2 bytes long per pixel
    m_depthBuffer.resize(ImageVars::width * ImageVars::height * 2);

    for (unsigned int index = 0; index < ProcColor::Size; index++) {
        m_calibImages.emplace_back(ImageVars::height, ImageVars::width, CV_8UC3);
    }
}

template <class T>
Kinect<T>::~Kinect() {
    threadrunning = false;
    stop();

    /* join to the thread: maybe this should be in the stopstream functions?
       the entire idea that stopstream can be called both from inside and
       outside of the thread is a bit jacked up. I'll probably fix this soon */
    if (thread.joinable()) {
        thread.join();
    }

    // wait for Kinect thread to close before destroying instance
    while (threadrunning == true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

template <class T>
void Kinect<T>::start() {
    // If rgb image stream is down, start it
    if (rgb.state == NSTREAM_DOWN) {
        startstream(rgb);
    }

    // If depth image stream is down, start it
    if (depth.state == NSTREAM_DOWN) {
        startstream(depth);
    }
}

template <class T>
void Kinect<T>::stop() {
    auto oldVidState = NSTREAM_UP;

    // If this function was able to switch rgb's state from up to down
    if (rgb.state.compare_exchange_strong(oldVidState, NSTREAM_DOWN)) {
        // If other stream is down too, stop the thread
        if (depth.state == NSTREAM_DOWN) {
            threadrunning = false;
        }

        // Call the callback
        if (rgb.streamStopping != nullptr) {
            rgb.streamStopping(rgb, rgb.callbackarg);
        }
    }

    m_foundScreen = false;

    auto oldDepthState = NSTREAM_UP;

    // If this function was able to switch depth's state from up to down
    if (depth.state.compare_exchange_strong(oldDepthState, NSTREAM_DOWN)) {
        // If other stream is down too, stop the thread
        if (rgb.state == NSTREAM_DOWN) {
            threadrunning = false;
        }

        // Call the callback
        if (depth.streamStopping != nullptr) {
            depth.streamStopping(depth, depth.callbackarg);
        }
    }
}

template <class T>
bool Kinect<T>::isStreaming() const {
    return isVideoStreamRunning() || isDepthStreamRunning();
}

template <class T>
void Kinect<T>::saveCurrentImage(const std::string& fileName) {
    std::lock_guard<std::mutex> lock(m_vidImageMutex);

    QImage tmp(&m_vidBuffer[0], 640, 480, QImage::Format_RGB888);
    if (!tmp.save(fileName.c_str())) {
        std::cout << "Kinect: failed to save image to '" << fileName << "'\n";
    }
}

template <class T>
uint8_t* Kinect<T>::getCurrentImage() {
    if (m_displayVid) {
        return m_cvVidImage.ptr<uint8_t>(0);
    }
    else {
        return m_cvDepthImage.ptr<uint8_t>(0);
    }
}

template <class T>
unsigned int Kinect<T>::getCurrentWidth() const {
    return 640;
}

template <class T>
unsigned int Kinect<T>::getCurrentHeight() const {
    return 480;
}

template <class T>
bool Kinect<T>::isVideoStreamRunning() const {
    return rgb.state == NSTREAM_UP;
}

template <class T>
bool Kinect<T>::isDepthStreamRunning() const {
    return depth.state == NSTREAM_UP;
}

template <class T>
void Kinect<T>::registerVideoWindow() {
    m_displayVid = true;
}

template <class T>
void Kinect<T>::registerDepthWindow() {
    m_displayVid = false;
}

template <class T>
bool Kinect<T>::saveVideo(const std::string& fileName) {
    cv::Mat img(ImageVars::height, ImageVars::width, CV_8UC(3), &m_vidBuffer[0]);
    return cv::imwrite(fileName, img);
}

template <class T>
bool Kinect<T>::saveDepth(const std::string& fileName) {
    cv::Mat img(ImageVars::height, ImageVars::width, CV_8UC(3), m_cvDepthImage);
    return cv::imwrite(fileName, img);
}

template <class T>
void Kinect<T>::setCalibImage(ProcColor colorWanted) {
    if (isVideoStreamRunning()) {
        std::lock_guard<std::mutex> lock(m_vidImageMutex);
        std::memcpy(m_calibImages[colorWanted].data, &m_vidBuffer[0],
                    ImageVars::width * ImageVars::height * 3);
    }
}

template <class T>
void Kinect<T>::calibrate() {
    cv::Mat* redCalib = nullptr;
    cv::Mat* greenCalib = nullptr;
    cv::Mat* blueCalib = nullptr;
    char procImages = 0;

    if (isEnabled(Red)) {
        redCalib = &m_calibImages[Red];
        procImages |= 1;
    }

    if (isEnabled(Green)) {
        greenCalib = &m_calibImages[Green];
        procImages |= (1 << 1);
    }

    if (isEnabled(Blue)) {
        blueCalib = &m_calibImages[Blue];
        procImages |= (1 << 2);
    }

    m_plistRaw.clear();
    m_plistProc.clear();

    // If image was disabled, nullptr is passed instead, so it's ignored

    /* Use the calibration images to locate a quadrilateral in the
     * image which represents the screen (returns 1 on failure)
     */
    m_quad = findScreenBox(*redCalib, *greenCalib, *blueCalib, procImages);

    // If no box was found, m_quad will be nullptr
    m_foundScreen = m_quad.validQuad;
}

template <class T>
void Kinect<T>::lookForCursors() {
    // We can't look for cursors if we never found a screen on which to look
    if (m_foundScreen) {
        {
            std::lock_guard<std::mutex> lock(m_vidImageMutex);

            /* Create a list of points which represent potential locations
               of the pointer */
            cv::Mat tempImage(ImageVars::height, ImageVars::width, CV_8UC3, &m_vidBuffer[0]);
            m_plistRaw = findImageLocation(tempImage, FLT_RED);
        }

        /* Identify the points in m_plistRaw which are located inside the
         * boundary defined by m_quad, and scale them to the size of the
         * computer's main screen. These are mouse pointer candidates.
         */
        if (!m_plistRaw.empty() && m_moveMouse) {
            m_plistProc = findScreenLocation(m_plistRaw, m_quad, m_screenRect.right() - m_screenRect.left(), m_screenRect.bottom() - m_screenRect.top());

            if (!m_plistProc.empty()) {
                auto& point = m_plistProc.front();
                QCursor::setPos(m_screenRect.left() + point.x,
                                m_screenRect.top() + point.y);
            }
        }
    }
}

template <class T>
void Kinect<T>::setMouseTracking(bool on) {
    m_moveMouse = on;
}

template <class T>
void Kinect<T>::enableColor(ProcColor color) {
    if (!isEnabled(color)) {
        m_enabledColors |= (1 << color);
        m_calibImages[color] = cvCreateImage(m_imageSize, IPL_DEPTH_8U, 3);
    }
}

template <class T>
void Kinect<T>::disableColor(ProcColor color) {
    if (isEnabled(color)) {
        m_enabledColors &= ~(1 << color);
    }
}

template <class T>
bool Kinect<T>::isEnabled(ProcColor color) const {
    return m_enabledColors & (1 << color);
}

template <class T>
void Kinect<T>::setScreenRect(const QRect& screenRect) {
    m_screenRect = screenRect;
}

template <class T>
void Kinect<T>::newVideoFrame(void* classObject) {
    Kinect* kntPtr = reinterpret_cast<Kinect*>(classObject);

    kntPtr->m_vidImageMutex.lock();

    // Copy image to internal buffer (3 channels)
    std::memcpy(&kntPtr->m_vidBuffer[0], kntPtr->rgb.buf,
                ImageVars::width * ImageVars::height * 3);

    kntPtr->m_vidDisplayMutex.lock();

    //                            B ,   G ,   R ,   A
    cv::Scalar lineColor = cv::Scalar(0x00, 0xFF, 0x00, 0xFF);

    kntPtr->m_cvVidImage = cv::Mat(ImageVars::height, ImageVars::width, CV_8UC3, &kntPtr->m_vidBuffer[0]);

    if (kntPtr->m_foundScreen) {
        // Draw lines to show user where the screen is
        cv::line(kntPtr->m_cvVidImage, kntPtr->m_quad.point[0],
               kntPtr->m_quad.point[1], lineColor, 2);
        cv::line(kntPtr->m_cvVidImage, kntPtr->m_quad.point[1],
               kntPtr->m_quad.point[2], lineColor, 2);
        cv::line(kntPtr->m_cvVidImage, kntPtr->m_quad.point[2],
               kntPtr->m_quad.point[3], lineColor, 2);
        cv::line(kntPtr->m_cvVidImage, kntPtr->m_quad.point[3],
               kntPtr->m_quad.point[0], lineColor, 2);

        kntPtr->lookForCursors();
    }

    // Perform conversion from RGBA to BGRA for use as image data in CreateBitmap
    cv::cvtColor(kntPtr->m_cvVidImage, kntPtr->m_cvBitmapDest, CV_RGB2BGRA);

    kntPtr->m_vidDisplayMutex.unlock();

    kntPtr->m_vidImageMutex.unlock();

    kntPtr->callNewImage(nullptr, 0);
}

template <class T>
void Kinect<T>::newDepthFrame(void* classObject) {
    Kinect* kntPtr = reinterpret_cast<Kinect*>(classObject);

    kntPtr->m_depthImageMutex.lock();

    // Copy image to internal buffer (2 bytes per pixel)
    std::memcpy(&kntPtr->m_depthBuffer[0], kntPtr->depth.buf,
                ImageVars::width * ImageVars::height * 2);

    double depth = 0.0;
    unsigned short depthVal = 0;
    for (unsigned int index = 0; index < ImageVars::width * ImageVars::height; index++) {
        std::memcpy(&depthVal, &kntPtr->m_depthBuffer[2 * index], sizeof(unsigned short));

        depth = Kinect::rawDepthToMeters(depthVal);

        auto color = QColor::fromHsv(360 * depth / 5.f, 255, 255);
        //auto color = QColor::fromHsv(36, 25, 25);

        // Assign values from 0 to 5 meters with a shade from black to white
        kntPtr->m_cvDepthImage.data[4 * index + 0] = color.blue();
        kntPtr->m_cvDepthImage.data[4 * index + 1] = color.green();
        kntPtr->m_cvDepthImage.data[4 * index + 2] = color.red();
    }

    // Make HBITMAP from pixel array
    kntPtr->m_depthDisplayMutex.lock();

    //                            B ,   G ,   R ,   A
    CvScalar lineColor = cvScalar(0x00, 0xFF, 0x00, 0xFF);

    if (kntPtr->m_foundScreen) {
        // Draw lines to show user where the screen is
        cv::line(kntPtr->m_cvDepthImage, kntPtr->m_quad.point[0],
               kntPtr->m_quad.point[1], lineColor, 2);
        cv::line(kntPtr->m_cvDepthImage, kntPtr->m_quad.point[1],
               kntPtr->m_quad.point[2], lineColor, 2);
        cv::line(kntPtr->m_cvDepthImage, kntPtr->m_quad.point[2],
               kntPtr->m_quad.point[3], lineColor, 2);
        cv::line(kntPtr->m_cvDepthImage, kntPtr->m_quad.point[3],
               kntPtr->m_quad.point[0], lineColor, 2);
    }

    kntPtr->m_depthDisplayMutex.unlock();

    kntPtr->m_depthImageMutex.unlock();

    kntPtr->callNewImage(nullptr, 0);
}

template <class T>
double Kinect<T>::rawDepthToMeters(unsigned short depthValue) {
    if (depthValue < 2047) {
        return 1.f / (static_cast<double>(depthValue) * -0.0030711016 +
                      3.3309495161);
    }

    return 0.0;
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
template <class T>
void Kinect<T>::rgb_cb(freenect_device* dev, void* rgbBuf, uint32_t timestamp) {
    (void) rgbBuf;
    Kinect& kntPtr = *static_cast<Kinect*>(freenect_get_user(dev));

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
    if (kntPtr.rgb.newFrame != nullptr) {
        kntPtr.rgb.newFrame(kntPtr.rgb.callbackarg);
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
template <class T>
void Kinect<T>::depth_cb(freenect_device* dev, void* depthBuf, uint32_t timestamp) {
    (void) depthBuf;
    Kinect& kntPtr = *static_cast<Kinect*>(freenect_get_user(dev));

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
    if (kntPtr.depth.newFrame != nullptr) {
        kntPtr.depth.newFrame(kntPtr.depth.callbackarg);
    }
}

/*
 * User calls this to start an RGB or depth stream. Should be called
 * through the NStream struct
 *
 * stream: The NStream handle of the stream to start.
 */
template <class T>
int Kinect<T>::startstream(NStream<Kinect>& stream) {
    /* You can't start a stream that's already started */
    if (stream.state != NSTREAM_DOWN)
        return 1;

    threadrunning_mutex.lock();
    if (!threadrunning) {
        /* The main worker thread isn't running yet. Start it. */
        threadrunning = true;
        thread = std::thread(&Kinect::threadmain, this);

        {
            std::unique_lock<std::mutex> lock(threadcond_mutex);
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
    if (stream.streamStarting != nullptr) {
        stream.streamStarting(stream, stream.callbackarg);
    }

    return 0;
}

/*
 * User calls this to stop the RGB stream. Note that RGB and depth streams
 * have seperate stop functions, unlike start functions.
 *
 * stream: The NStream handle of the stream to stop.
 */
template <class T>
int Kinect<T>::rgb_stopstream() {
    if (rgb.state != NSTREAM_UP)
        return 1;

    if (depth.state != NSTREAM_UP) {
        /* We're servicing no other streams, shut down the thread */
        threadrunning = false;
    }

    rgb.state = NSTREAM_DOWN;

    /* Do the callback */
    if (rgb.streamStopping != nullptr) {
        rgb.streamStopping(rgb, rgb.callbackarg);
    }

    return 0;
}

/*
 * User calls this to stop the depth stream. Note that depth and depth streams
 * have seperate stop functions, unlike start functions.
 *
 * stream: The NStream handle of the stream to stop
 */
template <class T>
int Kinect<T>::depth_stopstream() {
    if (depth.state != NSTREAM_UP)
        return 1;

    if (rgb.state != NSTREAM_UP) {
        /* We're servicing no other streams, shut down the thread */
        threadrunning = false;
    }

    depth.state = NSTREAM_DOWN;

    /* Do the callback */
    if (depth.streamStopping != nullptr) {
        depth.streamStopping(depth, depth.callbackarg);
    }

    return 0;
}

/*
 * Main thread function, initializes the kinect, and runs the
 * libfreenect event loop.
 */
template <class T>
void Kinect<T>::threadmain() {
    int error;
    int ndevs;
    freenect_context *f_ctx;
    freenect_device *f_dev;

    /* initialize libfreenect */
    error = freenect_init(&f_ctx, nullptr);
    if (error != 0) {
        std::cerr << "failed to initialize libfreenect\n";
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    ndevs = freenect_num_devices(f_ctx);
    if (ndevs != 1) {
        std::cerr << "more/less than one kinect detected\n";
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    error = freenect_open_device(f_ctx, &f_dev, 0);
    if (error != 0) {
        std::cerr << "failed to open kinect device\n";
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
        std::cerr << "failed to set video mode\n";
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
        std::cerr << "failed to set depth mode\n";
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
        std::cerr << "failed to set video buffer\n";
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
        std::cerr << "failed to set depth buffer\n";
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    error = freenect_start_video(f_dev);
    if (error != 0) {
        std::cerr << "failed to start video stream\n";
        freenect_close_device(f_dev);
        freenect_shutdown(f_ctx);
        threadrunning = false;
        threadcond.notify_all();
        return;
    }

    error = freenect_start_depth(f_dev);
    if (error != 0) {
        std::cerr << "failed to start depth stream\n";
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

    threadrunning = false;

    auto oldState = NSTREAM_UP;
    // If this function was able to switch rgb's state from up to down
    if (rgb.state.compare_exchange_strong(oldState, NSTREAM_DOWN)) {
        // Call the callback
        if (rgb.streamStopping != nullptr) {
            rgb.streamStopping(rgb, rgb.callbackarg);
        }
    }
    oldState = NSTREAM_UP;
    // If this function was able to switch depth's state from up to down
    if (depth.state.compare_exchange_strong(oldState, NSTREAM_DOWN)) {
        // Call the callback
        if (depth.streamStopping != nullptr) {
            depth.streamStopping(depth, depth.callbackarg);
        }
    }

    freenect_stop_video(f_dev);
    freenect_stop_depth(f_dev);

    freenect_close_device(f_dev);
    freenect_shutdown(f_ctx);
}
