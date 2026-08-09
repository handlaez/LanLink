#ifndef I_PACKETIZER_HPP
#define I_PACKETIZER_HPP

#include <cstdint>
#include <span>
#include <vector>

#pragma pack(push, 1)
struct UDPFrameHeader {
    uint32_t frameId;
    uint16_t packetIndex;
    uint16_t packetCount;
    uint32_t payloadSize;
};
#pragma pack(pop)

struct Packet {
    std::vector<uint8_t> bytes;
};

class IPacketizer {
public:
    virtual ~IPacketizer() = default;

    virtual std::vector<Packet> Packetize(std::span<const uint8_t> frameData, uint32_t frameId) = 0;
};

#endif // !I_PACKETIZER_HPP
