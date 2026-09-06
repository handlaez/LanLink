#include <cstring>
#include <X11/Xutil.h>
#undef Bool
#undef CursorShape
#undef Expose
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#undef FontChange
#undef None
#undef Status
#undef Unsorted

#include "platform/linux/LnxFrameGrabber.hpp"

FrameGrabber::~FrameGrabber()
{
    ReleaseFrame();

    if (display_) {
        XCloseDisplay(display_);
        display_ = nullptr;
    }
}

bool FrameGrabber::GetFrameDimensions(uint32_t& outWidth, uint32_t& outHeight) 
{
    if (!initialized_) {
        return false;
    }

    outWidth = width_;
    outHeight = height_;

    return true;
}

bool FrameGrabber::Initialize()
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

	initialized_ = true;
    width_ = static_cast<uint32_t>(attributes.width);
    height_ = static_cast<uint32_t>(attributes.height);

    return width_ > 0 && height_ > 0;
}

bool FrameGrabber::CaptureFrame(VideoFrame& frame)
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
    capturedFrame_.bitsPerPixel = static_cast<uint32_t>(image_->bits_per_pixel / 8);

    if (capturedFrame_.bitsPerPixel != 4) {
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

void FrameGrabber::ReleaseFrame()
{
    if (image_) {
        XDestroyImage(image_);
        image_ = nullptr;
    }

    capturedFrame_ = {};
    frameAcquired_ = false;
}

uint32_t FrameGrabber::width() const noexcept
{
    return width_;
}

uint32_t FrameGrabber::height() const noexcept
{
    return height_;
}
