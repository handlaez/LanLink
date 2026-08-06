#include <iostream>

#include "WinDuplicationGrabber.hpp"

int main()
{
    WinDuplicationGrabber grabber;

    if (!grabber.Initialize()) {
        std::cerr << "Failed to initialize grabber\n";
        return 1;
    }

    FrameData frame;

    if (!grabber.CaptureFrame(frame)) {
        std::cerr << "Failed to capture frame\n";
        return 1;
    }

    std::cout << "Captured frame: " << frame.width << "x" << frame.height << ", texture ptr=" << frame.nativeTextureHandle << std::endl;

    grabber.ReleaseFrame();

    std::cout << "Success!" << std::endl;

    return 0;
}