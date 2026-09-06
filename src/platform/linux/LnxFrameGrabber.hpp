#ifndef LNX_FRAME_GRABBER_HPP
#define LNX_FRAME_GRABBER_HPP

#include <cstdint>
#include <X11/Xlib.h>

#include "common/IFrameGrabber.hpp"
#include "common/VideoFrame.hpp"
#include "LinuxFrame.hpp"

class LinuxFrameGrabber : IFrameGrabber{
public:
    LinuxFrameGrabber() = default;
    ~LinuxFrameGrabber();

    LinuxFrameGrabber(const LinuxFrameGrabber&) = delete;
    LinuxFrameGrabber& operator=(const LinuxFrameGrabber&) = delete;

    bool Initialize() override;
    bool CaptureFrame(VideoFrame& frame) override;
    void ReleaseFrame() override;
    bool GetFrameDimensions(uint32_t& outWidth, uint32_t& outHeight) override;

    uint32_t width() const noexcept;
    uint32_t height() const noexcept;

private:
    Display* display_ = nullptr;
    Window rootWindow_ = 0;

    XImage* image_ = nullptr;
    LinuxCapturedFrame capturedFrame_{};

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool frameAcquired_ = false;
};

#endif