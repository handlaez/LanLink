#include "LnxPacketReceiver.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

namespace {
    constexpr int kReceiveBufferSize = 8 * 1024 * 1024;
    constexpr timeval kSelectTimeout{
        .tv_sec = 0,
        .tv_usec = 5000
    };
}

LnxPacketReceiver::~LnxPacketReceiver()
{
    close();
}

bool LnxPacketReceiver::initialize(uint16_t port)
{
    close();

    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket < 0) {
        logger().error("Failed to create UDP socket");
        return false;
    }

    int receiveBufferSize = kReceiveBufferSize;

    if (setsockopt( m_socket, SOL_SOCKET, SO_RCVBUF, &receiveBufferSize, sizeof(receiveBufferSize)) < 0) {
        logger().warn("Failed to set SO_RCVBUF");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(
        m_socket,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)) < 0) {
        logger().error("Failed to bind UDP socket");
        close();
        return false;
    }

    const int flags = fcntl(m_socket, F_GETFL, 0);
    if (flags < 0 || fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        logger().error("Failed to configure non-blocking UDP socket");
        close();
        return false;
    }

    return true;
}

int LnxPacketReceiver::receivePacket(
    uint8_t* buffer,
    size_t maxCapacity)
{
    if (m_socket < 0 || buffer == nullptr || maxCapacity == 0) {
        return -1;
    }

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(m_socket, &readSet);

    timeval timeout = kSelectTimeout;

    const int result = select(m_socket + 1, &readSet, nullptr, nullptr,  &timeout);

    if (result < 0) {
        if (errno == EINTR) {
            return 0;
        }

        return -1;
    }

    if (result == 0) {
        // no packet available within the timeout.
        return 0;
    }

    if (!FD_ISSET(m_socket, &readSet)) {
        return 0;
    }

    const ssize_t received = recv(m_socket, buffer, maxCapacity, 0);

    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }

        return -1;
    }

    return static_cast<int>(received);
}

void LnxPacketReceiver::close()
{
    if (m_socket >= 0) {
        ::close(m_socket);
        m_socket = -1;
    }
}