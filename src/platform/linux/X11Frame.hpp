#ifndef X11_FRAME_HPP
#define X11_FRAME_HPP

#include <cstdint>

struct X11Frame {
    const uint8_t* data = nullptr;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t stride = 0;
    uint32_t bitsPerPixel = 0;

    uint64_t redMask = 0;
    uint64_t greenMask = 0;
    uint64_t blueMask = 0;
};

#endif