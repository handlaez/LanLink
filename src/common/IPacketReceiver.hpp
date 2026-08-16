#ifndef I_PACKET_RECEIVER_HPP
#define I_PACKET_RECEIVER_HPP

#include <cstdint>
#include <cstddef>
#include <vector>

class IPacketReceiver {
public: 
	virtual ~IPacketReceiver() = default;

	virtual bool initialize(uint16_t port) = 0;

	virtual int receivePacket(uint8_t* buffer, size_t maxCapacity) = 0;

	virtual void close() = 0;
};

#endif // !I_PACKET_RECEIVER_HPP
