#ifndef LNX_FRAME_CONVERTER_HPP
#define LNX_FRAME_CONVERTER_HPP

#include <cstdint>
#include <vector>

#include "common/IFrameConverter.hpp"
#include "platform/linux/X11Frame.hpp"
#include "platform/linux/YUVFrame.hpp"

class FrameConverter final : public IFrameConverter {
public:
    FrameConverter() = default;
    ~FrameConverter() override = default;

    FrameConverter(const FrameConverter&) = delete;
    FrameConverter& operator=(const FrameConverter&) = delete;

    bool Initialize(uint32_t width, uint32_t height) override;
    bool Convert(const VideoFrame& input, VideoFrame& output) override;

private:
    bool IsValidInput(const X11Frame& input) const;
    void ConvertPixel(const X11Frame& input, uint32_t x, uint32_t y, uint8_t& red, uint8_t& green, uint8_t& blue) const;
    void ConvertToYuv420p(const X11Frame& input);

    uint32_t width_ = 0;
    uint32_t height_ = 0;

    std::vector<uint8_t> bufferY_;
    std::vector<uint8_t> bufferU_;
    std::vector<uint8_t> bufferV_;

    YUVFrame outputFrame_{};

    bool initialized_ = false;
};

#endif
