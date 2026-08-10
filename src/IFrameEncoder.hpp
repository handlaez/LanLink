#ifndef I_FRAME_ENCODER_HPP
#define I_FRAME_ENCODER_HPP

#include <cstdint>
#include <vector>

#include "VideoFrame.hpp"
#include "EncodedFrame.hpp"

class IFrameEncoder {
public:
	virtual ~IFrameEncoder() = default;
	virtual bool Initialize(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) = 0;
	virtual bool SubmitFrame(const VideoFrame& frame) = 0;
	virtual bool ReceiveFrame(EncodedFrame& outFrame) = 0;
	virtual void Shutdown() = 0;
};

#endif // !I_FRAME_ENCODER_HPP
