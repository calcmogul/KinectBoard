// =============================================================================
// File Name: VideoStream.hpp
// Description: Receives a video stream and displays it in a child window with
//             the specified properties
// Author: FRC Team 3512, Spartatroniks
// =============================================================================

#ifndef VIDEO_STREAM_HPP
#define VIDEO_STREAM_HPP

#include <QOpenGLWidget>

class QPaintEvent;
class QMouseEvent;

#include <string>
#include <atomic>
#include <chrono>
#include <map>
#include <cstdint>
#include <functional>
#include <thread>
#include <mutex>

#include "ClientBase.hpp"

class VideoStream : public QOpenGLWidget {
    Q_OBJECT

public:
    VideoStream(ClientBase<VideoStream>* client,
                QWidget* parentWin,
                int width,
                int height,
                std::function<void(void)> newImageCbk = nullptr,
                std::function<void(void)> startCbk = nullptr,
                std::function<void(void)> stopCbk = nullptr);
    virtual ~VideoStream();

    QSize sizeHint() const;

    // Set max frame rate of images displaying in window
    void setFPS(unsigned int fps);

protected:
    void newImageCallback(uint8_t* buf, int bufsize);
    void startCallback();
    void stopCallback();

    void paintGL();
    void resizeGL(int w, int h); // Arguments are buffer dimensions

private:
    ClientBase<VideoStream>* m_client;

    // Contains "Connecting" message
    QImage m_connectImg;

    // Contains "Disconnected" message
    QImage m_disconnectImg;

    // Contains "Waiting..." message
    QImage m_waitImg;

    // Stores image before displaying it on the screen
    uint8_t* m_img = nullptr;
    unsigned int m_imgWidth = 0;
    unsigned int m_imgHeight = 0;
    unsigned int m_textureWidth = 0;
    unsigned int m_textureHeight = 0;
    std::mutex m_imageMutex;

    /* Set to true when a new image is received from the MJPEG server
     * Set back to false upon the first call to getCurrentImage()
     */
    std::atomic<bool> m_newImageAvailable;

    /* Used to determine when to draw the "Connecting..." message
     * (when the stream first starts)
     */
    std::atomic<bool> m_firstImage{true};

    // Determines when a video frame is old
    std::chrono::time_point<std::chrono::system_clock> m_imageAge;

    // Used to limit display frame rate
    std::chrono::time_point<std::chrono::system_clock> m_displayTime;
    unsigned int m_frameRate = 15;

    // Locks window so only one thread can access or draw to it at a time
    std::mutex m_windowMutex;

    std::function<void(void)> m_newImageCallback;
    std::function<void(void)> m_startCallback;
    std::function<void(void)> m_stopCallback;

    // Makes sure "Waiting..." graphic is drawn after timeout
    std::thread m_updateThread;

    /* Recreates the graphics that display messages in the stream window
     * (Resizes them and recenters the text in the window)
     */
    void recreateGraphics(int width, int height);

    // Function is used by m_updateThread
    void updateFunc();

signals:
    void redraw();
};

#endif // VIDEO_STREAM_HPP

