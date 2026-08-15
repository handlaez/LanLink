#ifndef UNIFIED_DEPACKETIZER_HPP
#define UNIFIED_DEPACKETIZER_HPP

#include "IDepacketizer.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

struct FrameAssembly {
    uint16_t packetCount = 0;
    std::vector<std::vector<uint8_t>> packets;
    std::vector<bool> received;
    size_t receivedCount = 0;
};

class UnifiedDepacketizer final : public IDepacketizer {
public:
    UnifiedDepacketizer() = default;
    ~UnifiedDepacketizer() override = default;

    std::optional<EncodedFrame> processPacket(const uint8_t* packetData, size_t size) override;

    void reset() override;

private:
    std::unordered_map<uint64_t, FrameAssembly> m_frames;

    static uint64_t ntohll(uint64_t value);
    void cullFrames(uint64_t currentTimestamp);
};

#endif // UNIFIED_DEPACKETIZER_HPP