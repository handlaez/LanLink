#ifndef LNX_FRAME_HPP
#define LNX_FRAME_HPP

#include <cstdint>

struct LinuxCapturedFrame {
    const uint8_t* data = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t stride = 0;
    uint32_t bytesPerPixel = 4;
};

#endif