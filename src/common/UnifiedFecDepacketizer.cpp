#include "UnifiedFecDepacketizer.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

namespace {
    constexpr std::size_t HeaderSize = sizeof(UDPStreamHeader);
    constexpr std::size_t ShardSize = MaxDatagramSize - HeaderSize;
} // namespace

void FecDepacketizer::processPacket(const std::uint8_t* data, std::size_t size, std::vector<Packet>& outPackets)
{
    outPackets.clear();

    if (!data || size != MaxDatagramSize) {
        return;
    }

    const auto* header = reinterpret_cast<const UDPStreamHeader*>(data);

    const std::uint32_t blockId = ntohl(header->fecBlockId);
    const std::size_t dataShards = header->fecDataShards;

    if (dataShards == 0 || dataShards > MaxDataShards) {
        return;
    }

    const std::size_t parityShards = dataShards / 5 + 1;
    const std::size_t totalShards = dataShards + parityShards;

    if (header->fecIndex >= totalShards) {
        return;
    }

    // new FEC block: discard whatever was incomplete.
    if (!block_.active || block_.blockId != blockId) {
        block_ = {};
        block_.active = true;
        block_.blockId = blockId;
        block_.dataShards = dataShards;
    }

    // sanity check in case a malformed packet claims a different K.
    if (block_.dataShards != dataShards) {
        return;
    }

    const std::size_t index = header->fecIndex;

    // ignore duplicates.
    if (block_.received[index]) {
        return;
    }

    block_.packets[index].bytes.assign(data, data + size);
    block_.received[index] = true;

    // count shards,
    const std::size_t receivedCount = std::count(block_.received.begin(), block_.received.begin() + totalShards, true);
    // we cannot recover anything until we have at least K shards.
    if (receivedCount < dataShards) {
        return;
    }

    // build the list of available shards.
    std::vector<std::span<const std::byte>> shards(dataShards);
    std::vector<std::uint8_t> shardIndices(dataShards);

    std::size_t parityIndex = dataShards;

    for (std::size_t i = 0; i < dataShards; ++i) {
        if (block_.received[i]) {
            const auto& packet = block_.packets[i];

            shards[i] = std::span<const std::byte>(reinterpret_cast<const std::byte*>(packet.bytes.data() + HeaderSize), ShardSize);
            shardIndices[i] = static_cast<std::uint8_t>(i);
        }
        else {
            while (parityIndex < totalShards && !block_.received[parityIndex]) {
                ++parityIndex;
            }

            if (parityIndex >= totalShards) {
                return;
            }

            const auto& packet = block_.packets[parityIndex];

            shards[i] = std::span<const std::byte>(reinterpret_cast<const std::byte*>(packet.bytes.data() + HeaderSize), ShardSize);
            shardIndices[i] = static_cast<std::uint8_t>(parityIndex);

            ++parityIndex;
        }
    }

    // determine missing shards.
    std::vector<std::size_t> missingDataIndices;

    for (std::size_t i = 0; i < dataShards; ++i) {
        if (!block_.received[i]) {
            missingDataIndices.push_back(i);
        }
    }

    // no recovery needed: all data arrived.
    if (missingDataIndices.empty()) {
        outPackets.reserve(dataShards);

        for (std::size_t i = 0; i < dataShards; ++i) {
            outPackets.push_back(std::move(block_.packets[i]));
        }

        block_ = {};
        return;
    }

    FecCodec& codec = getCodec(dataShards);

    // allocate reconstructed payloads.
    std::vector<std::vector<std::byte>> recoveredBuffers(missingDataIndices.size(), std::vector<std::byte>(ShardSize));
    std::vector<std::span<std::byte>> recoveredShards;

    recoveredShards.reserve(recoveredBuffers.size());

    for (auto& buffer : recoveredBuffers) {
        recoveredShards.emplace_back(buffer.data(), buffer.size());
    }

    if (!codec.decode(shards, shardIndices, recoveredShards)) {
        block_ = {};
        return;
    }

    // we need metadata for constructing recovered packets.
    const Packet* referencePacket = nullptr;

    for (std::size_t i = 0; i < dataShards; ++i) {
        if (block_.received[i]) {
            referencePacket = &block_.packets[i];
            break;
        }
    }

    if (!referencePacket) {
        block_ = {};
        return;
    }

    const auto* referenceHeader = reinterpret_cast<const UDPStreamHeader*>(referencePacket->bytes.data());

    outPackets.reserve(dataShards);

    std::size_t recoveredIndex = 0;
    const std::uint16_t packetOffset = ntohs(referenceHeader->fecPacketOffset);

    for (std::size_t i = 0; i < dataShards; ++i) {
        if (block_.received[i]) {
            outPackets.push_back(std::move(block_.packets[i]));
            continue;
        }

        Packet packet;
        packet.bytes.resize(MaxDatagramSize, 0);

        auto* recoveredHeader = reinterpret_cast<UDPStreamHeader*>(packet.bytes.data());

        recoveredHeader->timestamp = referenceHeader->timestamp;
        recoveredHeader->sequenceNumber = 0;
        recoveredHeader->packetIndex = htons(static_cast<std::uint16_t>(packetOffset + i));
        recoveredHeader->packetCount = referenceHeader->packetCount;
        recoveredHeader->fecDataShards = referenceHeader->fecDataShards;
        recoveredHeader->fecBlockId = referenceHeader->fecBlockId;
        recoveredHeader->fecPacketOffset = referenceHeader->fecPacketOffset;
        recoveredHeader->fecIndex = static_cast<std::uint8_t>(i);
        recoveredHeader->payloadSize = htons(static_cast<std::uint16_t>(ShardSize));
        recoveredHeader->flags = 0;

        std::memcpy(packet.bytes.data() + HeaderSize, recoveredBuffers[recoveredIndex].data(), ShardSize);

        ++recoveredIndex;

        outPackets.push_back(std::move(packet));
    }

    block_ = {};
}

FecCodec& FecDepacketizer::getCodec(std::size_t k)
{
    if (k == 0 || k > MaxDataShards) {
        throw std::out_of_range("Invalid FEC data shard count");
    }

    auto& codec = codecs_[k];

    if (!codec) {
        const auto parity = k / 5 + 1;

        codec = std::make_unique<FecCodec>(ShardSize, k, parity);
    }

    return *codec;
}