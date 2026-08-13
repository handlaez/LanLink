#ifndef LNX_PACKET_RECEIVER_HPP
#define LNX_PACKET_RECEIVER_HPP

#include "IPacketReceiver.hpp"

class LnxPacketReceiver final : public IPacketReceiver {
public:
    LnxPacketReceiver() = default;
    ~LnxPacketReceiver() override;

    bool initialize(uint16_t port) override;

    int receivePacket(uint8_t* buffer, size_t maxCapacity) override;

    void close() override;

private:
    int m_socket = -1;
};

#endif // LNX_PACKET_RECEIVER_HPP