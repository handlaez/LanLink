#ifndef I_FRAME_CONVERTER_HPP 
#define I_FRAME_CONVERTER_HPP

#include <cstdint>

#include "common/VideoFrame.hpp"

struct ConversionParams {
    VideoFrame inputNativeResource;   // ID3D11Texture2D or VkImage/VkBuffer
    VideoFrame outputNativeResource;  // Target NV12
};

class IFrameConverter {
public:
    virtual ~IFrameConverter() = default;
    virtual bool Initialize(uint32_t width, uint32_t height) = 0;
    virtual bool Convert(const ConversionParams& params) = 0;
};

#endif