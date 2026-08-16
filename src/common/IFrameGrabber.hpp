#ifndef I_FRAME_GRABBER_HPP
#define I_FRAME_GRABBER_HPP

#include <cstdint>
#include "common/VideoFrame.hpp"

class IFrameGrabber {
public:
	virtual ~IFrameGrabber() = default;
	virtual bool Initialize() = 0;
	virtual bool CaptureFrame(VideoFrame& outFrame) = 0;
	virtual void ReleaseFrame() = 0;
};

#endif // !I_FRAME_GRABBER_HPP
