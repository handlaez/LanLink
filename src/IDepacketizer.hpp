#ifndef I_DEPACKETIZER_HPP
#define I_DEPACKETIZER_HPP

#include "EncodedFrame.hpp"
#include <cstdint>
#include <cstddef>

class IDepacketizer {
public:
    virtual ~IDepacketizer() = default;

    virtual EncodedFrame processPacket(const uint8_t* packetData, size_t size) = 0;

    virtual void reset() = 0;
};

#endif