#ifndef I_FRAME_CONVERTER_HPP 
#define I_FRAME_CONVERTER_HPP

#include <cstdint>

struct ConversionParams {
    uint32_t width;
    uint32_t height;
    void* inputNativeResource;   // ID3D11Texture2D* or VkImage/VkBuffer
    void* outputNativeResource;  // Target NV12
};

class IFrameConverter {
public:
    virtual ~IFrameConverter() = default;
    virtual bool Initialize(uint32_t width, uint32_t height) = 0;
    virtual bool ConvertBgraToNv12(const ConversionParams& params) = 0;
};

#endif