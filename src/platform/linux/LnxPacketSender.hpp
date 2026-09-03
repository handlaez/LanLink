#ifndef LNX_PACKET_SENDER_HPP
#define LNX_PACKET_SENDER_HPP

#include <cstdint>
#include <span>
#include <string>

#include <sys/socket.h>

#include "common/IPacketSender.hpp"

class PacketSender : public IPacketSender {
public:
    PacketSender();
    ~PacketSender();

    bool Open(const std::string& host, uint16_t port) override;
    bool Send(std::span<const uint8_t> bytes) override;
    void Close() override;

private:
    int m_socket{ -1 };
};

#endif