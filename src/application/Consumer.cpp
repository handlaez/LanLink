#include "application/Consumer.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

bool Consumer::initialize(uint16_t port)
{
    if (!receiver_.initialize(port)) {
        std::cerr << "Failed to initialize packet receiver.\n";
        return false;
    }

    if (!decoder_.initialize()) {
        std::cerr << "Failed to initialize frame decoder.\n";
        receiver_.close();
        return false;
    }

    return true;
}

void Consumer::run()
{
    std::vector<uint8_t> packetBuffer(1500);

    while (true) {
        const int packetSize =
            receiver_.receivePacket(
                packetBuffer.data(),
                packetBuffer.size());

        if (packetSize < 0) {
            std::cerr << "Packet receiver error.\n";
            break;
        }

        if (packetSize == 0) {
            continue;
        }

        const auto encodedFrame =
            depacketizer_.processPacket(
                packetBuffer.data(),
                static_cast<size_t>(packetSize));

        if (!encodedFrame.has_value()) {
            continue;
        }

        if (!decoder_.sendPacket(*encodedFrame)) {
            std::cerr << "Failed to send encoded frame to decoder.\n";
            continue;
        }

        VideoFrame decodedFrame{};

        while (decoder_.receiveFrame(decodedFrame)) {
            if (!rendererInitialized_) {
                if (!renderer_.initialize(
                    static_cast<int>(decodedFrame.width),
                    static_cast<int>(decodedFrame.height),
                    "LanLink")) {
                    std::cerr << "Failed to initialize frame renderer.\n";
                    return;
                }

                rendererInitialized_ = true;
            }

            renderer_.render(decodedFrame);

            if (!renderer_.pollEvents()) {
                return;
            }
        }
    }
}