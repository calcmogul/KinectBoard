# KinectBoard

KinectBoard is a program which facilitates interacting with a computer's mouse via the Microsoft Kinect.

## Dependencies

If one is building on Windows, we recommend using the [MSYS2 compiler](https://msys2.github.io/). The following libraries are required as dependencies and should be installed using either your package manager of choice or MSYS2's pacman:

* Qt4 or Qt5 (Qt5 recommended)
* OpenCV
* libfreenect

## Building KinectBoard

To build the project, first run `qmake dir` within a terminal from the desired build directory, where "dir" is the relative location of the KinectBoard.pro file. This will generate three makefiles. If a debug build is desired, run `make -f Makefile.Debug`. The default behavior when simply running `make` is to perform a release build.

## Setting Up

### Kinect

Before running this program, make sure your Kinect motion sensor is plugged into the USB port on your computer and into a power source. Once the Kinect is plugged in, it will be recognized by the computer automatically.

On Windows clients, next open "devmgmt.msc" and select the items which contain "XBox NUI" in their name... (incomplete)
