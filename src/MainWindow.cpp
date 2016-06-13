#include "MainWindow.hpp"
#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_ui = std::make_unique<Ui::MainWindow>();
    m_ui->setupUi(this);
    m_ui->actionDisplay_Video->setChecked(true);

    // Stream is initially stopped
    setWindowIcon(QIcon(":/images/OpenKinectOFF.ico"));

    connect(m_ui->actionStart_Tracking, SIGNAL(triggered()),
            this, SLOT(startTracking()));
    connect(m_ui->actionStop_Tracking, SIGNAL(triggered()),
            this, SLOT(stopTracking()));
    connect(m_ui->actionDisplay_Video, SIGNAL(triggered()),
             this, SLOT(displayVideo()));
    connect(m_ui->actionDisplay_Depth, SIGNAL(triggered()),
             this, SLOT(displayDepth()));
    connect(m_ui->actionAbout_KinectBoard, &QAction::triggered,
            [this] {
                QMessageBox::about(this, tr("About KinectBoard"),
                                   tr("<br>KinectBoard, Version 1.0a<br>"
                                   "Copyright &copy;2012-2015<br>"
                                   "Alan Fisher and Tyler Veness<br>"
                                   "All Rights Reserved"));
            });
    connect(m_ui->actionHelp, &QAction::triggered,
            [this] {
                auto dir = QDir::currentPath();
                QUrl url = QUrl::fromLocalFile(dir + "/help/index.html");
                if (!QDesktopServices::openUrl(url)) {
                    QMessageBox::critical(this, tr("Help"), tr("Could not open help file"));
                }
            });

    freenect_init(&m_context, nullptr);

    m_kinect = new Kinect(m_context, 0);
    m_stream = new VideoStream(m_kinect,
                               this,
                               640,
                               480,
                               [] {},
                               [] {},
                               [] {});
    m_ui->verticalLayout->addWidget(m_stream);

    m_kinect->start();
}

MainWindow::~MainWindow() {
    freenect_shutdown(m_context);
}

void MainWindow::startTracking() {
    m_kinect->setMouseTracking(true);

    setWindowIcon(QIcon(":/images/OpenKinectON.ico"));
    QMessageBox::information(this, tr("Mouse Tracking"), "Mouse tracking has been enabled.");
}

void MainWindow::stopTracking() {
    m_kinect->setMouseTracking(false);

    setWindowIcon(QIcon(":/images/OpenKinectOFF.ico"));
    QMessageBox::information(this, tr("Mouse Tracking"), "Mouse tracking has been disabled.");
}

void MainWindow::recalibrate() {
    // If there is no Kinect connected, don't bother trying to retrieve images
    if (m_kinect->isVideoStreamRunning()) {
        TestScreen testWin(gCurrentMonitor.dim);
        testWin.setColor(ProcColor::Red);

        // Give Kinect time to get image w/ test pattern in it
        std::this_thread::sleep_for(std::chrono::milliseconds(750));
        m_kinect->setCalibImage(ProcColor::Red);

        testWin.setColor(ProcColor::Blue);

        // Give Kinect time to get image w/ test pattern in it
        std::this_thread::sleep_for(std::chrono::milliseconds(750));
        m_kinect->setCalibImage(ProcColor::Blue);

        m_kinect->calibrate();
    }
}

void MainWindow::displayVideo() {
    // Check "Display Video" and uncheck "Display Depth"
    if (!m_ui->actionDisplay_Video->isChecked()) {
        m_ui->actionDisplay_Video->setChecked(true);
    }
    m_ui->actionDisplay_Depth->setChecked(false);

    // Stop depth stream and switch to video stream
    m_kinect->registerVideoWindow();
}

void MainWindow::displayDepth() {
    // Check "Display Depth" and uncheck "Display Video"
    if (!m_ui->actionDisplay_Depth->isChecked()) {
        m_ui->actionDisplay_Depth->setChecked(true);
    }
    m_ui->actionDisplay_Video->setChecked(false);

    // Stop video stream and switch to depth stream
    m_kinect->registerDepthWindow();
}
