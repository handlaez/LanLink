#ifndef LNX_FRAME_ENCODER_HPP
#define LNX_FRAME_ENCODER_HPP

#include <cstdint>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
}

#include "common/IFrameEncoder.hpp"

class FrameEncoder final : public IFrameEncoder {
public:
    FrameEncoder() = default;
    ~FrameEncoder() override;

    FrameEncoder(const FrameEncoder&) = delete;
    FrameEncoder& operator=(const FrameEncoder&) = delete;

    bool Initialize(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) override;

    bool SubmitFrame(const VideoFrame& frame) override;
    bool ReceiveFrame(EncodedFrame& outFrame) override;

    void Shutdown() override;

private:
    bool initialized_ = false;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t fps_ = 0;
    uint32_t bitrate_ = 0;

    AVCodecContext* codecContext_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVPacket* packet_ = nullptr;

    int64_t nextPts_ = 0;
};

#endif // LNX_FRAME_ENCODER_HPP
