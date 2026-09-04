class LinuxFrameConverter : public IFrameConverter {
public:
    bool Initialize(uint32_t width, uint32_t height) override
    {
        return width > 0 && height > 0;
    }

    bool ConvertBgraToRgba(const ConversionParams& params) override {
        params.outputNativeResource = params.inputNativeResource;
        return true;
    }
};