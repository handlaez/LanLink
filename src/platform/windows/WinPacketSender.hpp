#ifndef WIN_PACKET_SENDER_HPP
#define WIN_PACKET_SENDER_HPP

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "common/IPacketSender.hpp"

class PacketSender : public IPacketSender {
public:
	PacketSender();
	~PacketSender();

	bool Open(const std::string& host, uint16_t port) override;
	bool Send(std::span<const uint8_t> bytes) override;
	void Close() override;

private:
	SOCKET m_socket{ INVALID_SOCKET };
};

#endif