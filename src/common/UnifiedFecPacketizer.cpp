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
    constexpr std::size_t HeaderSize = sizeof(UDPStreamHeader);
    constexpr std::size_t ShardSize = MaxDatagramSize - HeaderSize;

    constexpr std::uint8_t PacketFlagParity = 1 << 0;
}

FecPacketizer::FecPacketizer(std::size_t shardSize) : shardSize_(shardSize)
{
    if (shardSize_ == 0) {
        throw std::invalid_argument("FEC shard size cannot be zero");
    }
}

void FecPacketizer::Packetize(std::span<const Packet> dataPackets, std::vector<Packet>& outPackets)
{
    outPackets.clear();

    if (dataPackets.empty()) {
        return;
    }

    if (dataPackets.size() > 0xFFFF) {
        throw std::invalid_argument("Too many packets in frame");
    }

    // maximum output size assuming every block gets parity.
    std::size_t requiredCapacity = dataPackets.size();

    for (std::size_t offset = 0; offset < dataPackets.size(); offset += MaxDataShards)
    {
        const std::size_t k = std::min(MaxDataShards, dataPackets.size() - offset);
        requiredCapacity += k / 5 + 1;
    }

    outPackets.reserve(requiredCapacity);

    const std::uint16_t packetCount = static_cast<std::uint16_t>(dataPackets.size());

    std::size_t packetOffset = 0;

    while (packetOffset < dataPackets.size()) {
        const std::size_t k = std::min(MaxDataShards, dataPackets.size() - packetOffset);
        const std::size_t parityCount = k / 5 + 1;

        FecCodec& codec = getCodec(k);
        const std::uint32_t blockId = nextBlockId_++;

    // Copy the data packets into the output and assign FEC metadata.
    // Note to self: it can be done better

        for (std::size_t i = 0; i < k; ++i) {
            Packet packet = dataPackets[packetOffset + i];

            if (packet.bytes.size() != MaxDatagramSize) {
                throw std::invalid_argument(
                    "FEC input packet has invalid size");
            }

            auto* header = reinterpret_cast<UDPStreamHeader*>(packet.bytes.data());
            header->sequenceNumber = htonl(nextSequenceNumber_++);
            header->packetCount = htons(packetCount);
            header->fecBlockId = htonl(blockId);
            header->fecDataShards = static_cast<std::uint8_t>(k);
            header->fecPacketOffset = htons(static_cast<std::uint16_t>(packetOffset));
            header->fecIndex = static_cast<std::uint8_t>(i);
            header->flags &= ~PacketFlagParity;

            outPackets.push_back(std::move(packet));
        }

        // create parity buffers.

        std::vector<std::vector<std::byte>> parityBuffers(parityCount, std::vector<std::byte>(shardSize_));

        std::vector<std::span<const std::byte>> dataShards;
        std::vector<std::span<std::byte>> parityShards;

        dataShards.reserve(k);
        parityShards.reserve(parityCount);

        for (std::size_t i = 0; i < k; ++i) {
            const auto& packet = outPackets[outPackets.size() - k + i];

            dataShards.emplace_back(reinterpret_cast<const std::byte*>(packet.bytes.data() + HeaderSize), shardSize_);
        }

        for (auto& buffer : parityBuffers) {
            parityShards.emplace_back(buffer.data(), buffer.size());
        }

        codec.encode(dataShards, parityShards);

        // Turn parity into normal UDP packets.

        const auto* firstHeader = reinterpret_cast<const UDPStreamHeader*>(outPackets[outPackets.size() - k].bytes.data());

        for (std::size_t i = 0; i < parityCount; ++i) {
            Packet packet;
            packet.bytes.resize(MaxDatagramSize, 0);

            auto* header = reinterpret_cast<UDPStreamHeader*>(packet.bytes.data());

            header->timestamp = firstHeader->timestamp;
            header->sequenceNumber = htonl(nextSequenceNumber_++);
            header->packetIndex = 0;
            header->packetCount = htons(packetCount);
            header->fecBlockId = htonl(blockId);
            header->fecDataShards = static_cast<std::uint8_t>(k);
            header->fecPacketOffset = htons(static_cast<std::uint16_t>(packetOffset));
            header->fecIndex = static_cast<std::uint8_t>(k + i);
            header->payloadSize = htons(static_cast<std::uint16_t>(shardSize_));
            header->flags = PacketFlagParity;

            std::memcpy(packet.bytes.data() + HeaderSize, parityBuffers[i].data(), shardSize_);

            outPackets.push_back(std::move(packet));
        }

        packetOffset += k;
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