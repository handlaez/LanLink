#include "LnxFrameConverter.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace {

    uint8_t ClampToByte(int value)
    {
        return static_cast<uint8_t>(std::clamp(value, 0, 255));
    }

    void RgbToYuv(uint8_t red, uint8_t green, uint8_t blue, uint8_t& y, uint8_t& u, uint8_t& v)
    {
        // BT.601 limited-range conversion.
        const int yValue = ((66 * red + 129 * green + 25 * blue + 128) >> 8) + 16;
        const int uValue = ((-38 * red - 74 * green + 112 * blue + 128) >> 8) + 128;
        const int vValue = ((112 * red - 94 * green - 18 * blue + 128) >> 8) + 128;

        y = ClampToByte(yValue);
        u = ClampToByte(uValue);
        v = ClampToByte(vValue);
    }

} // namespace

bool FrameConverter::Initialize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0) {
        return false;
    }

    width_ = width;
    height_ = height;

    const std::size_t lumaSize = static_cast<std::size_t>(width_) * height_;

    const std::size_t chromaWidth = (width_ + 1) / 2;
    const std::size_t chromaHeight = (height_ + 1) / 2;
    const std::size_t chromaSize = chromaWidth * chromaHeight;

    try {
        bufferY_.resize(lumaSize);
        bufferU_.resize(chromaSize);
        bufferV_.resize(chromaSize);
    }
    catch (...) {
        bufferY_.clear();
        bufferU_.clear();
        bufferV_.clear();

        outputFrame_ = {};
        width_ = 0;
        height_ = 0;
        initialized_ = false;

        return false;
    }

    outputFrame_.width = width_;
    outputFrame_.height = height_;

    outputFrame_.strideY = width_;
    outputFrame_.strideU = static_cast<uint32_t>(chromaWidth);
    outputFrame_.strideV = static_cast<uint32_t>(chromaWidth);

    outputFrame_.dataY = bufferY_.data();
    outputFrame_.dataU = bufferU_.data();
    outputFrame_.dataV = bufferV_.data();

    initialized_ = true;
    return true;
}

bool FrameConverter::IsValidInput(const X11Frame& input) const
{
    if (!input.data) {
        return false;
    }

    if (input.width != width_ || input.height != height_) {
        return false;
    }

    if (input.bitsPerPixel != 24 && input.bitsPerPixel != 32) {
        return false;
    }

    const uint32_t bytesPerPixel = input.bitsPerPixel / 8;

    if (input.stride < input.width * bytesPerPixel) {
        return false;
    }

    return true;
}

void FrameConverter::ConvertPixel(const X11Frame& input, uint32_t x, uint32_t y, uint8_t& red, uint8_t& green, uint8_t& blue) const
{
    const uint8_t* row = input.data + static_cast<std::size_t>(y) * input.stride;

    if (input.bitsPerPixel == 32) {
        const uint8_t* pixel = row + static_cast<std::size_t>(x) * 4;

        /*
         * Typical X11 TrueColor layout on Raspberry Pi:
         *
         * byte 0: blue
         * byte 1: green
         * byte 2: red
         * byte 3: unused/alpha
         */
        blue = pixel[0];
        green = pixel[1];
        red = pixel[2];
        return;
    }

    const uint8_t* pixel = row + static_cast<std::size_t>(x) * 3;

    blue = pixel[0];
    green = pixel[1];
    red = pixel[2];
}

void FrameConverter::ConvertToYuv420p(const X11Frame& input)
{
    const uint32_t chromaWidth = (width_ + 1) / 2;
    const uint32_t chromaHeight = (height_ + 1) / 2;

    for (uint32_t cy = 0; cy < chromaHeight; ++cy) {
        for (uint32_t cx = 0; cx < chromaWidth; ++cx) {
            const uint32_t x0 = cx * 2;
            const uint32_t y0 = cy * 2;

            int totalU = 0;
            int totalV = 0;
            int sampleCount = 0;

            for (uint32_t dy = 0; dy < 2; ++dy) {
                for (uint32_t dx = 0; dx < 2; ++dx) {
                    const uint32_t x = x0 + dx;
                    const uint32_t y = y0 + dy;

                    if (x >= width_ || y >= height_) {
                        continue;
                    }

                    uint8_t red = 0;
                    uint8_t green = 0;
                    uint8_t blue = 0;

                    ConvertPixel(input, x, y, red, green, blue);

                    uint8_t luma = 0;
                    uint8_t u = 0;
                    uint8_t v = 0;

                    RgbToYuv(red, green, blue, luma, u, v);

                    const std::size_t yIndex = static_cast<std::size_t>(y) * outputFrame_.strideY + x;

                    bufferY_[yIndex] = luma;

                    totalU += u;
                    totalV += v;
                    ++sampleCount;
                }
            }
            const std::size_t chromaIndex = static_cast<std::size_t>(cy) * outputFrame_.strideU + cx;

            bufferU_[chromaIndex] = static_cast<uint8_t>(totalU / sampleCount);
            bufferV_[chromaIndex] = static_cast<uint8_t>(totalV / sampleCount);
        }
    }
}

bool FrameConverter::Convert(const VideoFrame& input, VideoFrame& output)
{
    if (!initialized_ || !input.nativeResource) {
        return false;
    }

    auto* capturedFrame = static_cast<X11Frame*>(input.nativeResource);

    if (!IsValidInput(*capturedFrame)) {
        return false;
    }

    ConvertToYuv420p(*capturedFrame);

    output.nativeResource = &outputFrame_;
    output.width = outputFrame_.width;
    output.height = outputFrame_.height;
    output.timestamp = input.timestamp;

    return true;
}
