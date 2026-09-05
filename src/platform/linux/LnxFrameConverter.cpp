#include "LnxFrameConverter.hpp"

bool LinuxFrameConverter::Initialize(uint32_t width, uint32_t height)
{
    return width > 0 && height > 0;
}

bool LinuxFrameConverter::Convert(const ConversionParams& params)
{
    params.outputNativeResource = params.inputNativeResource;
    return true;
}