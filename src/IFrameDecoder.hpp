#ifndef I_FRAME_DECODER_HPP
#define I_FRAME_DECODER_HPP

#include "EncodedFrame.hpp"
#include "VideoFrame.hpp"

class IFrameDecoder {
public:
    virtual ~IFrameDecoder() = default;

    virtual bool initialize() = 0;

    virtual bool sendPacket(const EncodedFrame& encodedFrame) = 0;

    virtual bool receiveFrame(VideoFrame& outFrame) = 0;

    virtual void flush() = 0;
};

#endif