#ifndef UNIFIED_FRAME_DECODER_HPP
#define UNIFIED_FRAME_DECODER_HPP

#include "common/IFrameDecoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
}

class FrameDecoder final : public IFrameDecoder {
public:
    FrameDecoder() = default;
    ~FrameDecoder() override;

    bool initialize() override;

    bool sendPacket(const EncodedFrame& encodedFrame) override;

    bool receiveFrame(VideoFrame& outFrame) override;

    void flush() override;

private:
    AVCodecContext* m_context = nullptr;
    AVPacket* m_packet = nullptr;
    AVFrame* m_frame = nullptr;
};

#endif // UNIFIED_FRAME_DECODER_HPP