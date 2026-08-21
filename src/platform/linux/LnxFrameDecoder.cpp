#include "LnxFrameDecoder.hpp"
#include "Logger.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <cstring>

LnxFrameDecoder::~LnxFrameDecoder()
{
    flush();

    if (m_frame) {
        av_frame_free(&m_frame);
    }

    if (m_packet) {
        av_packet_free(&m_packet);
    }

    if (m_context) {
        avcodec_free_context(&m_context);
    }
}

bool LnxFrameDecoder::initialize()
{
    if (m_context || m_packet || m_frame) {
        return false;
    }

    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);

    if (!codec) {
        logger().error("HEVC decoder not found");
        return false;
    }

    m_context = avcodec_alloc_context3(codec);

    if (!m_context) {
        logger().error("Failed to allocate decoder context");
        return false;
    }

    if (avcodec_open2(m_context, codec, nullptr) < 0) {
        logger().error("Failed to open HEVC decoder");
        avcodec_free_context(&m_context);
        return false;
    }

    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();

    if (!m_packet || !m_frame) {
        logger().error("Failed to allocate FFmpeg packet/frame");

        if (m_frame) {
            av_frame_free(&m_frame);
        }

        if (m_packet) {
            av_packet_free(&m_packet);
        }

        avcodec_free_context(&m_context);

        return false;
    }

    return true;
}

bool LnxFrameDecoder::sendPacket(
    const EncodedFrame& encodedFrame)
{
    if (!m_context || !m_packet) {
        return false;
    }

    av_packet_unref(m_packet);

    if (encodedFrame.data.empty()) {
        return false;
    }

    if (av_new_packet(
        m_packet,
        static_cast<int>(encodedFrame.data.size())) < 0) {
        return false;
    }

    std::memcpy(
        m_packet->data,
        encodedFrame.data.data(),
        encodedFrame.data.size());

    // Preserve the timestamp through FFmpeg.
    m_packet->pts = static_cast<int64_t>(encodedFrame.timestamp);

    return avcodec_send_packet(m_context, m_packet) >= 0;
}

bool LnxFrameDecoder::receiveFrame(VideoFrame& outFrame)
{
    if (!m_context || !m_frame) {
        return false;
    }

    const int result =
        avcodec_receive_frame(m_context, m_frame);

    if (result != 0) {
        return false;
    }

    outFrame.nativeResource = m_frame;
    outFrame.width = static_cast<uint32_t>(m_frame->width);
    outFrame.height = static_cast<uint32_t>(m_frame->height);

    if (m_frame->best_effort_timestamp != AV_NOPTS_VALUE) {
        outFrame.timestamp =
            static_cast<uint64_t>(m_frame->best_effort_timestamp);
    }

    return true;
}

void LnxFrameDecoder::flush()
{
    if (!m_context) {
        return;
    }

    avcodec_send_packet(m_context, nullptr);

    while (avcodec_receive_frame(m_context, m_frame) == 0) {
    }
}