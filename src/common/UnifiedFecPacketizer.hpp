#ifndef FEC_PACKETIZER_HPP
#define FEC_PACKETIZER_HPP

#include <cstdint>
#include <memory>

#include "UDPFrameHeader.hpp"
#include "UnifiedFecCodec.hpp"

class FecPacketizer {
public:
    explicit FecPacketizer(std::size_t shardSize);

    void Packetize(std::span<const Packet> dataPackets, std::vector<Packet>& outPackets);

private:
    FecCodec& getCodec(std::size_t dataShards);

    static constexpr std::size_t MaxDataShards = 20;

    std::size_t shardSize_;
    std::array<std::unique_ptr<FecCodec>, MaxDataShards + 1> codecs_;

    uint32_t nextSequenceNumber_ = 0;
    uint32_t nextBlockId_ = 0;
};

#endif