#ifndef UDP_FRAME_HEADER_HPP
#define UDP_FRAME_HEADER_HPP

#include <cstdint>
#include <vector>

#pragma pack(push, 1)
struct UDPFrameHeader {
    uint64_t timestamp;
    uint16_t packetIndex;
    uint16_t packetCount;
    uint32_t payloadSize;
};
#pragma pack(pop)

struct Packet {
    std::vector<uint8_t> bytes;
};

#endif 
