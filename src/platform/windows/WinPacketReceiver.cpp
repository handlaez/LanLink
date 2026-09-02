#include "WinPacketReceiver.hpp"
#include "Logger.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

namespace {
    constexpr int kReceiveBufferSize = 8 * 1024 * 1024;

    constexpr timeval kSelectTimeout{
        .tv_sec = 0,
        .tv_usec = 5000
    };
}

WinPacketReceiver::~WinPacketReceiver()
{
    close();
}

bool WinPacketReceiver::initialize(uint16_t port)
{
    close();

    WSADATA wsaData{};

    const int wsaResult = WSAStartup(
        MAKEWORD(2, 2),
        &wsaData);

    if (wsaResult != 0) {
        logger().error("Failed to initialize Winsock");
        return false;
    }

    m_wsaInitialized = true;

    m_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (m_socket == INVALID_SOCKET) {
        logger().error("Failed to create UDP socket");
        close();
        return false;
    }

    int receiveBufferSize = kReceiveBufferSize;

    if (::setsockopt(
        m_socket,
        SOL_SOCKET,
        SO_RCVBUF,
        reinterpret_cast<const char*>(&receiveBufferSize),
        sizeof(receiveBufferSize)) == SOCKET_ERROR) {
        logger().warn("Failed to set SO_RCVBUF");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (::bind(
        m_socket,
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)) == SOCKET_ERROR) {
        logger().error("Failed to bind UDP socket");
        close();
        return false;
    }

    u_long nonBlocking = 1;

    if (::ioctlsocket(
        m_socket,
        FIONBIO,
        &nonBlocking) == SOCKET_ERROR) {
        logger().error("Failed to configure non-blocking UDP socket");
        close();
        return false;
    }

    return true;
}

int WinPacketReceiver::receivePacket(
    uint8_t* buffer,
    size_t maxCapacity)
{
    if (m_socket == INVALID_SOCKET ||
        buffer == nullptr ||
        maxCapacity == 0) {
        return -1;
    }

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(m_socket, &readSet);

    timeval timeout = kSelectTimeout;

    const int result = ::select(
        0,
        &readSet,
        nullptr,
        nullptr,
        &timeout);

    if (result == SOCKET_ERROR) {
        const int error = WSAGetLastError();

        if (error == WSAEINTR) {
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

    const int receiveCapacity = static_cast<int>(
        std::min(maxCapacity, static_cast<size_t>(INT_MAX)));

    const int received = ::recv(
        m_socket,
        reinterpret_cast<char*>(buffer),
        receiveCapacity,
        0);

    if (received == SOCKET_ERROR) {
        const int error = WSAGetLastError();

        if (error == WSAEWOULDBLOCK) {
            return 0;
        }

        return -1;
    }

    return received;
}

void WinPacketReceiver::close()
{
    if (m_socket != INVALID_SOCKET) {
        ::closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    if (m_wsaInitialized) {
        ::WSACleanup();
        m_wsaInitialized = false;
    }
}