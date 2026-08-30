#include <stdexcept>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif


#include "UnifiedFecPacketizer.hpp"

namespace {
    constexpr uint8_t PacketFlagParity = 1 << 0;
} // namespace

FecPacketizer::FecPacketizer(std::size_t shardSize) : shardSize_(shardSize)
{
    if (shardSize_ == 0) {
        throw std::invalid_argument("FEC shard size cannot be zero");
    }
}

void FecPacketizer::Packetize(std::span<const Packet> dataPackets, std::vector<Packet>& outPackets)
{
    outPackets.empty();

    if (dataPackets.empty()) {
        return;
    }

    const std::size_t k = dataPackets.size();

    if (k > MaxDataShards) {
        throw std::invalid_argument("Frame contains too many packets for FEC");
    }

    FecCodec& codec = getCodec(k);

    const std::size_t parityShards = codec.parityShards();
    const std::size_t totalShards = k + parityShards;

    outPackets.reserve(totalShards);

    const uint32_t fecBlockId = nextBlockId_++;

    // Copy the data packets into the output and assign FEC metadata.
    // Note to self: it can be done better

    for (std::size_t i = 0; i < dataPackets.size(); ++i) {
        Packet packet = dataPackets[i];

        if (packet.bytes.size() != MaxDatagramSize) {
            throw std::invalid_argument("FEC input packet has invalid size");
        }

        auto* header = reinterpret_cast<UDPStreamHeader*>(packet.bytes.data());

#ifdef _WIN32
        header->sequenceNumber = htonl(nextSequenceNumber_++);
        header->fecBlockId = htonl(fecBlockId);
#else
        header->sequenceNumber = htonl(nextSequenceNumber_++);
        header->fecBlockId = htonl(fecBlockId);
#endif

        header->fecIndex = static_cast<uint8_t>(i);
        header->flags &= ~PacketFlagParity;

        outPackets.push_back(std::move(packet));
    }

    // Prepare FEC input/output shards.
    std::vector<std::span<const std::byte>> dataShards;
    dataShards.reserve(k);

    for (const auto& packet : outPackets) {
        dataShards.emplace_back(
            reinterpret_cast<const std::byte*>(packet.bytes.data() + sizeof(UDPStreamHeader)), shardSize_);
    }

    std::vector<std::vector<std::byte>> parityBuffers(parityShards, std::vector<std::byte>(shardSize_));

    std::vector<std::span<std::byte>> parityShardsViews;
    parityShardsViews.reserve(parityShards);

    for (auto& buffer : parityBuffers) {
        parityShardsViews.emplace_back(
            buffer.data(),
            buffer.size());
    }

    // Generate Parity
    codec.encode(dataShards, parityShardsViews);

    // Turn parity shards into UDP packets.
    const Packet& firstDataPacket = outPackets.front();

    const auto* firstHeader =
        reinterpret_cast<const UDPStreamHeader*>(firstDataPacket.bytes.data());

    for (std::size_t i = 0; i < parityShards; ++i) {
        Packet packet;
        packet.bytes.resize(MaxDatagramSize, 0);

        auto* header = reinterpret_cast<UDPStreamHeader*>(packet.bytes.data());

        header->timestamp = firstHeader->timestamp;
        header->sequenceNumber = htonl(nextSequenceNumber_++);
        header->packetIndex = 0;
        header->packetCount = firstHeader->packetCount;
        header->fecBlockId = htonl(fecBlockId);
        header->fecIndex = static_cast<uint8_t>(k + i);
        header->payloadSize = htons(static_cast<uint16_t>(shardSize_));
        header->flags = PacketFlagParity;

        std::memcpy(packet.bytes.data() + sizeof(UDPStreamHeader), parityBuffers[i].data(), shardSize_);

        outPackets.push_back(std::move(packet));
    }
}

FecCodec& FecPacketizer::getCodec(std::size_t k)
{
    if (k == 0 || k > MaxDataShards) {
        throw std::out_of_range("Invalid FEC data shard count");
    }

    auto& codec = codecs_[k];

    if (!codec) {
        const auto parity = k / 5 + 1;
        codec = std::make_unique<FecCodec>(shardSize_, k, parity);
    }

    return *codec;
}