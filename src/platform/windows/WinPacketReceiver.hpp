#ifndef WIN_PACKET_RECEIVER_HPP
#define WIN_PACKET_RECEIVER_HPP

#include "common/IPacketReceiver.hpp"

#include <winsock2.h>

class WinPacketReceiver final : public IPacketReceiver {
public:
    WinPacketReceiver() = default;
    ~WinPacketReceiver() override;

    bool initialize(uint16_t port) override;

    int receivePacket(uint8_t* buffer, size_t maxCapacity) override;

    void close() override;

private:
    SOCKET m_socket = INVALID_SOCKET;
    bool m_wsaInitialized = false;
};

#endif // WIN_PACKET_RECEIVER_HPP