#ifndef I_FRAME_GRABBER_HPP
#define I_FRAME_GRABBER_HPP

#include <cstdint>

struct FrameData {
	void* nativeTextureHandle = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
};

class IFrameGrabber {
public:
	virtual ~IFrameGrabber() = default;
	virtual bool Initialize() = 0;
	virtual bool CaptureFrame(FrameData& outFrame) = 0;
	virtual void ReleaseFrame() = 0;
};

#endif // !I_FRAME_GRABBER_HPP
