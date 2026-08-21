#include "Consumer.hpp"
#include "application/Consumer.hpp"

#include <cstdint>
#include <iostream>
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

    return true;
}

void Consumer::run(const std::atomic<bool>& running)
{
    std::vector<uint8_t> packetBuffer(1500);

    while (renderer_.pollEvents()) {
        const int packetSize = receiver_.receivePacket(packetBuffer.data(), packetBuffer.size());

        if (packetSize < 0) {
            logger().error("Packet receiver error.");
            break;
        }

        if (packetSize == 0) {
            continue;
        }

        const auto encodedFrame = depacketizer_.processPacket(packetBuffer.data(), static_cast<size_t>(packetSize));

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
                if (!renderer_.initialize(static_cast<int>(decodedFrame.width), static_cast<int>(decodedFrame.height), "LanLink")) 
                {
                    logger().error("Failed to initialize frame renderer.");
                    return;
                }

                rendererInitialized_ = true;
            }

            renderer_.render(decodedFrame);
        }
    }
}