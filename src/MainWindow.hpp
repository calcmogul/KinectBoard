#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QPushButton>
#include <list>
#include <memory>

#include "ui_MainWindow.h"

#include "TestScreen.hpp"
#include "VideoStream.hpp"
#include "Kinect.hpp"

struct MonitorIndex {
    // dimensions of monitor
    QRect dim;

    // button used to represent this monitor
    QPushButton* activeButton;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void startTracking();
    void stopTracking();
    void recalibrate();
    void displayVideo();
    void displayDepth();

private:
    std::unique_ptr<Ui::MainWindow> m_ui;
    Kinect<VideoStream>* m_kinect;
    VideoStream* m_stream;

    // Used for choosing on which monitor to draw test image
    std::list<MonitorIndex> gMonitors;
    MonitorIndex gCurrentMonitor = {{0, 0, QApplication::desktop()->screenGeometry().width(),
                                     QApplication::desktop()->screenGeometry().height()}, nullptr};
};

#endif // MAIN_WINDOW_HPP
