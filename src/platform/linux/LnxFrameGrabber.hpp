#ifndef LNX_FRAME_GRABBER_HPP
#define LNX_FRAME_GRABBER_HPP

#include <cstdint>
#include <X11/Xlib.h>
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

#include "common/IFrameGrabber.hpp"
#include "common/VideoFrame.hpp"
#include "platform/linux/X11Frame.hpp"
#include "platform/linux/YUVFrame.hpp"

class FrameGrabber : IFrameGrabber{
public:
    FrameGrabber() = default;
    ~FrameGrabber();

    FrameGrabber(const FrameGrabber&) = delete;
    FrameGrabber& operator=(const FrameGrabber&) = delete;

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
    X11Frame capturedFrame_{};

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool frameAcquired_ = false;
    bool initialized_ = false;
};

#endif
