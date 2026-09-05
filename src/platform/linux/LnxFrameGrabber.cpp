#include "LnxFrameGrabber.hpp"

#include <cstring>
#include <X11/Xutil.h>

LinuxFrameGrabber::~LinuxFrameGrabber()
{
    ReleaseFrame();

    if (display_) {
        XCloseDisplay(display_);
        display_ = nullptr;
    }
}

bool LinuxFrameGrabber::Initialize()
{
    if (display_) {
        return true;
    }

    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        return false;
    }

    rootWindow_ = DefaultRootWindow(display_);

    XWindowAttributes attributes{};
    if (!XGetWindowAttributes(display_, rootWindow_, &attributes)) {
        XCloseDisplay(display_);
        display_ = nullptr;
        rootWindow_ = 0;
        return false;
    }

    width_ = static_cast<uint32_t>(attributes.width);
    height_ = static_cast<uint32_t>(attributes.height);

    return width_ > 0 && height_ > 0;
}

bool LinuxFrameGrabber::CaptureFrame(VideoFrame& frame)
{
    if (!display_ || !rootWindow_ || frameAcquired_) {
        return false;
    }

    image_ = XGetImage(display_, rootWindow_, 0, 0, width_, height_, AllPlanes, ZPixmap);

    if (!image_ || !image_->data) {
        if (image_) {
            XDestroyImage(image_);
            image_ = nullptr;
        }

        return false;
    }

    capturedFrame_.data = reinterpret_cast<const uint8_t*>(image_->data);

    capturedFrame_.width = width_;
    capturedFrame_.height = height_;
    capturedFrame_.stride = static_cast<uint32_t>(image_->bytes_per_line);
    capturedFrame_.bytesPerPixel = static_cast<uint32_t>(image_->bits_per_pixel / 8);

    if (capturedFrame_.bytesPerPixel != 4) {
        ReleaseFrame();
        return false;
    }

    frame.nativeResource = &capturedFrame_;
    frame.width = width_;
    frame.height = height_;
    frame.timestamp = 0;

    frameAcquired_ = true;
    return true;
}

void LinuxFrameGrabber::ReleaseFrame()
{
    if (image_) {
        XDestroyImage(image_);
        image_ = nullptr;
    }

    capturedFrame_ = {};
    frameAcquired_ = false;
}

uint32_t LinuxFrameGrabber::width() const noexcept
{
    return width_;
}

uint32_t LinuxFrameGrabber::height() const noexcept
{
    return height_;
}