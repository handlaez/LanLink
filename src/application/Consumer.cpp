#include "application/Consumer.hpp"
#include "Logger.hpp"

#include <cstdint>
#include <vector>

bool Consumer::initialize(uint16_t port)
{
    if (!receiver_.initialize(port)) {
        logger().error("Failed to initialize packet receiver.");
        return false;
    }

    if (!decoder_.initialize()) {
        logger().error("Failed to initialize frame decoder.");
        receiver_.close();
        return false;
    }

    logger().info("Initialization successful.");
    logger().info(QString("Listening on port: %1").arg(port));

    return true;
}

void Consumer::run(const std::atomic<bool>& running)
{
    std::vector<uint8_t> packetBuffer(65536);
    std::vector<Packet> fecPackets;

    while (running.load(std::memory_order_relaxed)) {
        const int packetSize = receiver_.receivePacket(packetBuffer.data(), packetBuffer.size());

        if (packetSize < 0) {
            logger().error("Packet receiver error.");
            break;
        }

        if (packetSize == 0) {
            continue;
        }

        fecDepacketizer_.processPacket(packetBuffer.data(), static_cast<std::size_t>(packetSize),fecPackets);

        for (const auto& packet : fecPackets) {
            const auto encodedFrame = depacketizer_.processPacket(packet.bytes.data(), packet.bytes.size());

            if (!encodedFrame.has_value()) {
                continue;
            }

            if (!decoder_.sendPacket(*encodedFrame)) {
                logger().error("Failed to send encoded frame to decoder.");
                continue;
            }

            VideoFrame decodedFrame{};

            while (decoder_.receiveFrame(decodedFrame)) {
                if (!rendererInitialized_) {
                    if (!renderer_.initialize(static_cast<int>(decodedFrame.width), static_cast<int>(decodedFrame.height), "LanLink")) {
                        logger().error("Failed to initialize frame renderer.");
                        return;
                    }

                    rendererInitialized_ = true;
                }

                renderer_.render(decodedFrame);
            }
        }
    }

    logger().info("Receiving halted.\n");
}