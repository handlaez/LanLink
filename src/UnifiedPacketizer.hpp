#ifndef UNIFIED_PACKETIZER_HPP
#define UNIFIED_PACKETIZER_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include "IPacketizer.hpp"

class UnifiedPacketizer
{
public:
	static constexpr std::size_t MaxDatagramSize = 1400;
	std::vector<Packet> Packetize(const uint8_t* frameData, std::size_t frameSize, uint32_t frameId) const;
};

#endif 