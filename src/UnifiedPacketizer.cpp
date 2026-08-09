#include <algorithm> 
#include <cstring>

#include "UnifiedPacketizer.hpp" 

#ifdef _WIN32
	#include <winsock2.h> 
#else 
	#include <arpa/inet.h> 
#endif


std::vector<Packet> UnifiedPacketizer::Packetize(const uint8_t* frameData, std::size_t frameSize, uint32_t frameId) const
{
	std::vector<Packet> packets;

	if (frameData == nullptr || frameSize == 0)
		return packets;

	constexpr std::size_t headerSize = sizeof(UDPFrameHeader);

	static_assert(MaxDatagramSize > headerSize, "MaxDatagramSize must be larger than PacketHeader");

	const std::size_t maxPayloadSize = MaxDatagramSize - headerSize;
	const uint16_t packetCount = static_cast<uint16_t>((frameSize + maxPayloadSize - 1) / maxPayloadSize);

	packets.reserve(packetCount);

	std::size_t offset = 0;

	for (uint16_t packetIndex = 0; packetIndex < packetCount; ++packetIndex)
	{
		const std::size_t remaining = frameSize - offset;
		const std::size_t payloadSize = std::min(maxPayloadSize, remaining);

		Packet packet;
		packet.bytes.resize(headerSize + payloadSize);

		auto* header = reinterpret_cast<UDPFrameHeader*>(packet.bytes.data());

		header->frameId = htonl(frameId);
		header->packetIndex = htons(packetIndex);
		header->packetCount = htons(packetCount);
		header->payloadSize = htonl(static_cast<uint32_t>(payloadSize));
		
		std::memcpy(packet.bytes.data() + headerSize, frameData + offset, payloadSize);
		
		packets.push_back(std::move(packet));
		
		offset += payloadSize;
	}

	return packets;
}
