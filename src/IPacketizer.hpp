#ifndef I_PACKETIZER_HPP
#define I_PACKETIZER_HPP

#include <cstdint>
#include <span>
#include <vector>

#include "UDPFrameHeader.hpp"

class IPacketizer {
public:
    virtual ~IPacketizer() = default;

    virtual std::vector<Packet> Packetize(std::span<const uint8_t> frameData, uint32_t frameId) = 0;
};

#endif // !I_PACKETIZER_HPP
