#ifndef I_FRAME_ENCODER_HPP
#define I_FRAME_ENCODER_HPP

#include <cstdint>
#include "IFrameGrabber.hpp"

class IFrameEncoder {
public:
	virtual ~IFrameEncoder() = default;
	virtual bool Initialize(uint32_t width, uint32_t height, uint32_t fps) = 0;
	virtual bool EncodeFrame(const FrameData& frame, uint8_t** outBitstream, uint32_t* outSize) = 0;
};

#endif // !I_FRAME_ENCODER_HPP
