#ifndef I_DEPACKETIZER_HPP
#define I_DEPACKETIZER_HPP

#include <cstdint>
#include <cstddef>
#include <optional>

#include "EncodedFrame.hpp"

class IDepacketizer {
public:
    virtual ~IDepacketizer() = default;

    virtual std::optional<EncodedFrame> processPacket(const uint8_t* packetData, size_t size) = 0;

    virtual void reset() = 0;
};

#endif