#ifndef UDP_FRAME_HEADER_HPP
#define UDP_FRAME_HEADER_HPP

#include <cstdint>
#include <array>
#include <span>
#include <cstddef>
#include <vector>

inline constexpr std::size_t MaxDatagramSize = 1200;

struct Packet {
    std::vector<uint8_t> bytes;
};

#pragma pack(push, 1)
struct UDPStreamHeader {
    uint64_t timestamp;

    uint32_t sequenceNumber; 
    uint16_t packetIndex;    
    uint16_t packetCount;    

    // FEC (forward error correction)
    uint32_t fecBlockId;     // which group does this belong to
    uint8_t  fecIndex;       // position in the group (0-9=Data, 10-11=Parity)

    // Payload
    uint16_t payloadSize;
    uint8_t  flags;          // bitmask
};
#pragma pack(pop)

struct PacketSlot {
    bool hasData = false;

    std::array<std::byte, MaxDatagramSize> buffer;

    // cast the front of the buffer to our header
    [[nodiscard]] const UDPStreamHeader* getHeader() const {
        return reinterpret_cast<const UDPStreamHeader*>(buffer.data());
    }

    // return a C++20 span viewing ONLY the payload data (no copy)
    [[nodiscard]] std::span<const std::byte> getPayload() const {
        const auto* header = getHeader();
        return std::span(buffer.data() + sizeof(UDPStreamHeader), header->payloadSize);
    }
};

#endif 