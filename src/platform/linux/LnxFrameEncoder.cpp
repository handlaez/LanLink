#include "LnxFrameEncoder.hpp"

#include <algorithm>
#include <cstring>

#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>

#include "Logger.hpp"
#include "platform/linux/YUVFrame.hpp"

namespace {

    std::string ffmpegError(int error)
    {
        char buffer[AV_ERROR_MAX_STRING_SIZE]{};
        av_strerror(error, buffer, sizeof(buffer));
        return std::string(buffer);
    }

} // namespace

LnxFrameEncoder::~LnxFrameEncoder()
{
    Shutdown();
}

void LnxFrameEncoder::Shutdown()
{
    if (codecContext_) {
        avcodec_free_context(&codecContext_);
    }

    if (frame_) {
        av_frame_free(&frame_);
    }

    if (packet_) {
        av_packet_free(&packet_);
    }

    initialized_ = false;
    width_ = 0;
    height_ = 0;
    fps_ = 0;
    bitrate_ = 0;
    nextPts_ = 0;
}

bool LnxFrameEncoder::Initialize(
    uint32_t width,
    uint32_t height,
    uint32_t fps,
    uint32_t bitrate)
{
    Shutdown();

    if (width == 0 || height == 0 || fps == 0 || bitrate == 0) {
        logger().error("Invalid encoder parameters.");
        return false;
    }

    const AVCodec* codec = avcodec_find_encoder_by_name("libx265");

    if (!codec) {
        logger().error("FFmpeg libx265 encoder was not found.");
        return false;
    }

    codecContext_ = avcodec_alloc_context3(codec);

    if (!codecContext_) {
        logger().error("Failed to allocate FFmpeg codec context.");
        return false;
    }

    codecContext_->codec_type = AVMEDIA_TYPE_VIDEO;
    codecContext_->codec_id = AV_CODEC_ID_HEVC;
    codecContext_->width = static_cast<int>(width);
    codecContext_->height = static_cast<int>(height);

    codecContext_->pix_fmt = AV_PIX_FMT_YUV420P;

    codecContext_->time_base = AVRational{
        1,
        static_cast<int>(fps)
    };

    codecContext_->framerate = AVRational{
        static_cast<int>(fps),
        1
    };

    codecContext_->bit_rate = static_cast<int64_t>(bitrate);

    codecContext_->gop_size = static_cast<int>(fps);
    codecContext_->max_b_frames = 0;

    /*
     * ultrafast reduces CPU usage.
     * zerolatency disables frame reordering and lookahead.
     */
    av_opt_set(codecContext_->priv_data, "preset", "ultrafast", 0);
    av_opt_set(codecContext_->priv_data, "tune", "zerolatency", 0);

    /*
     * Repeat VPS/SPS/PPS with keyframes so a receiver can recover
     * without requiring an earlier stream header.
     */
    av_opt_set(codecContext_->priv_data, "repeat-headers", "1", 0);

    const int result = avcodec_open2(codecContext_, codec, nullptr);

    if (result < 0) {
        logger().error(QString("Failed to open H.265 encoder: %1").arg(QString::fromStdString(ffmpegError(result))));
        Shutdown();
        return false;
    }

    frame_ = av_frame_alloc();

    if (!frame_) {
        logger().error("Failed to allocate AVFrame.");
        Shutdown();
        return false;
    }

    frame_->format = codecContext_->pix_fmt;
    frame_->width = codecContext_->width;
    frame_->height = codecContext_->height;

    // The frame uses externally owned buffers supplied by YUVFrame.
    packet_ = av_packet_alloc();

    if (!packet_) {
        logger().error("Failed to allocate AVPacket.");
        Shutdown();
        return false;
    }

    width_ = width;
    height_ = height;
    fps_ = fps;
    bitrate_ = bitrate;
    nextPts_ = 0;
    initialized_ = true;

    return true;
}

bool LnxFrameEncoder::SubmitFrame(const VideoFrame& input)
{
    if (!initialized_ || !input.nativeResource) {
        return false;
    }

    if (input.width != width_ || input.height != height_) {
        logger().error("Input frame dimensions do not match encoder.");
        return false;
    }

    auto* yuvFrame = static_cast<YUVFrame*>(input.nativeResource);

    if (!yuvFrame || !yuvFrame->dataY || !yuvFrame->dataU || !yuvFrame->dataV) {
        logger().error("Invalid YUV frame.");
        return false;
    }

    if (yuvFrame->width != width_ || yuvFrame->height != height_) {
        logger().error("YUV frame dimensions do not match encoder.");
        return false;
    }

    frame_->data[0] = yuvFrame->dataY;
    frame_->data[1] = yuvFrame->dataU;
    frame_->data[2] = yuvFrame->dataV;

    frame_->linesize[0] = static_cast<int>(yuvFrame->strideY);
    frame_->linesize[1] = static_cast<int>(yuvFrame->strideU);
    frame_->linesize[2] = static_cast<int>(yuvFrame->strideV);

    frame_->pts = nextPts_++;

    /*
     * avcodec_send_frame() consumes the frame metadata synchronously.
     * The actual encoded packet is retrieved by ReceiveFrame().
     */
    const int result = avcodec_send_frame(codecContext_, frame_);

    if (result < 0) {
        logger().error(QString("Failed to submit frame: %1").arg(QString::fromStdString(ffmpegError(result))));
        return false;
    }

    return true;
}

bool LnxFrameEncoder::ReceiveFrame(EncodedFrame& output)
{
    if (!initialized_) {
        return false;
    }

    av_packet_unref(packet_);

    const int result = avcodec_receive_packet(codecContext_, packet_);

    if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
        return false;
    }

    if (result < 0) {
        logger().error(QString("Failed to receive encoded packet: %1").arg(QString::fromStdString(ffmpegError(result))));
        return false;
    }

    output.data.assign(packet_->data, packet_->data + packet_->size);
    output.timestamp = static_cast<uint64_t>(packet_->pts);
    output.type = (packet_->flags & AV_PKT_FLAG_KEY) ? EncodedFrameType::Keyframe : EncodedFrameType::Delta;

    return true;
}