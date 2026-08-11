#include "WinPacketSender.hpp"

WinPacketSender::WinPacketSender()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

WinPacketSender::~WinPacketSender()
{
	Close();
	WSACleanup();
}

bool WinPacketSender::Open(const std::string& host, uint16_t port)
{
	Close();

	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	if (m_socket == INVALID_SOCKET) 
		return false;

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
		Close();
		return false;
	}

	if (connect(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
		Close();
		return false;
	}

	return true;
}

bool WinPacketSender::Send(std::span<const uint8_t> bytes)
{
	if (m_socket == INVALID_SOCKET) 
		return false;

	int sent = send(m_socket, reinterpret_cast<const char*>(bytes.data()), static_cast<int>(bytes.size()), 0);
	return sent == static_cast<int>(bytes.size());
}

void WinPacketSender::Close()
{
	if (m_socket != INVALID_SOCKET) {
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
}
