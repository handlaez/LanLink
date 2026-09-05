#ifndef LNX_FRAME_CONVERTER
#define LNX_FRAME_CONVERTER

#include "common/IFrameConverter.hpp"

class LinuxFrameConverter : public IFrameConverter {
public:
    bool Initialize(uint32_t width, uint32_t height) override;
    bool Convert(ConversionParams& params) override;
};

#endif