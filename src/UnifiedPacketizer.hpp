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
	void Packetize(const EncodedFrame& frame, std::vector<Packet>& outPackets) const;
};

#endif 