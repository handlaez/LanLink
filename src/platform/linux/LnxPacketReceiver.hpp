#ifndef LNX_PACKET_RECEIVER_HPP
#define LNX_PACKET_RECEIVER_HPP

#include "common/IPacketReceiver.hpp"

class PacketReceiver final : public IPacketReceiver {
public:
    PacketReceiver() = default;
    ~PacketReceiver() override;

    bool initialize(uint16_t port) override;

    int receivePacket(uint8_t* buffer, size_t maxCapacity) override;

    void close() override;

private:
    int m_socket = -1;
};

#endif // LNX_PACKET_RECEIVER_HPP