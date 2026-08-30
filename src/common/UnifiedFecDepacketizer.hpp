#ifndef UNIFIED_FEC_DEPACKETIZER_HPP
#define UNIFIED_FEC_DEPACKETIZER_HPP

#include "common/UDPFrameHeader.hpp"
#include "common/UnifiedFecCodec.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>

class FecDepacketizer {
public:
    static constexpr std::size_t MaxDataShards = 20;
    static constexpr std::size_t MaxTotalShards = MaxDataShards + (MaxDataShards / 5 + 1);

    void processPacket( const std::uint8_t* data, std::size_t size, std::vector<Packet>& outPackets);

private:
    struct FecBlock {
        bool active = false;

        std::uint32_t blockId = 0;

        std::size_t dataShards = 0;
        std::size_t parityShards = 0;

        std::array<Packet, MaxTotalShards> packets;
        std::array<bool, MaxTotalShards> received{};
    };

    FecBlock block_;

    std::array<std::unique_ptr<FecCodec>, MaxDataShards + 1> codecs_;

    FecCodec& getCodec(std::size_t k);
};

#endif