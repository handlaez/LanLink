#ifndef LNX_FRAME_DECODER_HPP
#define LNX_FRAME_DECODER_HPP

#include "IFrameDecoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
}

class LnxFrameDecoder final : public IFrameDecoder {
public:
    LnxFrameDecoder() = default;
    ~LnxFrameDecoder() override;

    bool initialize() override;

    bool sendPacket(const EncodedFrame& encodedFrame) override;

    bool receiveFrame(VideoFrame& outFrame) override;

    void flush() override;

private:
    AVCodecContext* m_context = nullptr;
    AVPacket* m_packet = nullptr;
    AVFrame* m_frame = nullptr;
};

#endif // LNX_FRAME_DECODER_HPP