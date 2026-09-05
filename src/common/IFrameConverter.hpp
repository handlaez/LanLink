#ifndef I_FRAME_CONVERTER_HPP 
#define I_FRAME_CONVERTER_HPP

#include <cstdint>

#include "common/VideoFrame.hpp"

class IFrameConverter {
public:
    virtual ~IFrameConverter() = default;

    virtual bool Initialize(uint32_t width, uint32_t height) = 0;
    // Converts to NV12 frame type.
    virtual bool Convert(const VideoFrame& input, VideoFrame& output) = 0;
};

#endif