QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = KinectBoard
TEMPLATE = app
CONFIG += c++14

SOURCES +=\
    src/MainWindow.cpp \
    src/Main.cpp \
    src/ImageVars.cpp \
    src/TestScreen.cpp \
    src/CKinect/NStream.inl \
    src/CKinect/Parse.cpp \
    src/VideoStream.cpp

HEADERS  += \
    src/MainWindow.hpp \
    src/ImageVars.hpp \
    src/Kinect.hpp \
    src/Kinect.inl \
    src/Processing.hpp \
    src/TestScreen.hpp \
    src/CKinect/NStream.hpp \
    src/CKinect/Parse.hpp \
    src/VideoStream.hpp \
    src/ClientBase.hpp \
    src/ClientBase.inl

FORMS    += \
    src/MainWindow.ui

RESOURCES += \
    KinectBoard.qrc

DISTFILES += \
    Resources.rc

LIBS += -lfreenect -lopencv_core -lopencv_imgcodecs -lopencv_imgproc
