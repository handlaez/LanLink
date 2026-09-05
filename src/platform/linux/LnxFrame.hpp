#ifndef LNX_FRAME_HPP
#define LNX_FRAME_HPP

#include <cstdint>

struct LinuxFrame {
    uint32_t strideY = 0;
    uint32_t strideU = 0;
    uint32_t strideV = 0;

    uint32_t width = 0;
    uint32_t height = 0;

    uint8_t* dataY = nullptr;
    uint8_t* dataU = nullptr;
    uint8_t* dataV = nullptr;
};

#endif