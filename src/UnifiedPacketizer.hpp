#ifndef UNIFIED_PACKETIZER_HPP
#define UNIFIED_PACKETIZER_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include "IPacketizer.hpp"
#include "EncodedFrame.hpp"

class UnifiedPacketizer
{
public:
	static constexpr std::size_t MaxDatagramSize = 1200;
	std::vector<Packet> Packetize(EncodedFrame& frame) const;
};

#endif 