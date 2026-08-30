#include "UnifiedDepacketizer.hpp"
#include "UDPFrameHeader.hpp"

#if defined(_WIN32) 
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#endif

#include <cstring>

std::optional<EncodedFrame> UnifiedDepacketizer::processPacket(
    const uint8_t* packetData,
    size_t size)
{
    if (packetData == nullptr) {
        return std::nullopt;
    }

    if (size <= sizeof(UDPStreamHeader)) {
        return std::nullopt;
    }

    UDPStreamHeader header{};

    std::memcpy(&header, packetData, sizeof(UDPStreamHeader));

    header.timestamp = ntohll(header.timestamp);
    header.packetIndex = ntohs(header.packetIndex);
    header.packetCount = ntohs(header.packetCount);
    header.payloadSize = ntohs(header.payloadSize);

    if (header.packetIndex >= header.packetCount) {
        return std::nullopt;
    }

    if (sizeof(UDPStreamHeader) + header.payloadSize > size) {
        return std::nullopt;
    }

    auto& frame = m_frames[header.timestamp];

    if (frame.packets.empty()) {
        frame.packetCount = header.packetCount;
        frame.packets.resize(header.packetCount);
        frame.received.resize(header.packetCount, false);
    }

    // same timestamp but a different packet count means
    // that this frame cannot be assembled consistently.
    if (frame.packetCount != header.packetCount) {
        m_frames.erase(header.timestamp);
        return std::nullopt;
    }

    if (!frame.received[header.packetIndex]) {
        const uint8_t* payload = packetData + sizeof(UDPStreamHeader);

        frame.packets[header.packetIndex] = std::vector<uint8_t>(payload, payload + header.payloadSize);

        frame.received[header.packetIndex] = true;
        ++frame.receivedCount;
    }

    if (frame.receivedCount != frame.packetCount) {
        cullFrames(header.timestamp);
        return std::nullopt;
    }

    EncodedFrame result;

    size_t totalSize = 0;

    for (const auto& packet : frame.packets) {
        totalSize += packet.size();
    }

    result.data.reserve(totalSize);

    for (const auto& packet : frame.packets) {
        result.data.insert(
            result.data.end(),
            packet.begin(),
            packet.end());
    }

    result.timestamp = header.timestamp;
    result.type = EncodedFrameType::Unknown;

    m_frames.erase(header.timestamp);

    cullFrames(header.timestamp);

    return result;
}

void UnifiedDepacketizer::reset()
{
    m_frames.clear();
}

uint64_t UnifiedDepacketizer::ntohll(uint64_t value)
{
    return (static_cast<uint64_t>(ntohl(value & 0xFFFFFFFFULL)) << 32) |
        ntohl(value >> 32);
}

void UnifiedDepacketizer::cullFrames(uint64_t currentTimestamp)
{
    if (m_frames.size() <= 30) {
        return;
    }

    auto it = m_frames.find(currentTimestamp);

    if (it == m_frames.end()) {
        return;
    }

    auto currentFrame = std::move(it->second);

    m_frames.clear();
    m_frames.emplace(currentTimestamp, std::move(currentFrame));
}
