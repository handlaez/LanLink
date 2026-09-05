#ifndef LNX_FRAME_CONVERTER
#define LNX_FRAME_CONVERTER

class LinuxFrameConverter : public IFrameConverter {
public:
    bool Initialize(uint32_t width, uint32_t height) override;
    bool Convert(const ConversionParams& params) override;
};

#endif