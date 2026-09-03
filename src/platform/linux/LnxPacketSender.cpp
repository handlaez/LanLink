#include "LnxPacketSender.hpp"

#include <arpa/inet.h>
#include <unistd.h>

PacketSender::PacketSender() = default;

PacketSender::~PacketSender()
{
    Close();
}

bool PacketSender::Open(const std::string& host, uint16_t port)
{
    Close();

    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (m_socket == -1)
        return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        Close();
        return false;
    }

    if (connect(m_socket, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == -1) {
        Close();
        return false;
    }

    return true;
}

bool PacketSender::Send(std::span<const uint8_t> bytes)
{
    if (m_socket == -1)
        return false;

    const ssize_t sent = send(m_socket, bytes.data(), bytes.size(), 0);

    return sent == static_cast<ssize_t>(bytes.size());
}

void PacketSender::Close()
{
    if (m_socket != -1) {
        close(m_socket);
        m_socket = -1;
    }
}