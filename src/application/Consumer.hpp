#ifndef CONSUMER_HPP
#define CONSUMER_HPP

#include <atomic>

#include "common/UnifiedFrameDecoder.hpp"
#include "common/UnifiedFrameRenderer.hpp"
#include "common/UnifiedDepacketizer.hpp"
#include "common/UnifiedFecDepacketizer.hpp"
#include "common/UDPFrameHeader.hpp"

#ifdef _WIN32
#include "platform/windows/WinPacketReceiver.hpp"
#else
#include "platform/linux/LnxPacketReceiver.hpp"
#endif

class Consumer {
public:
	bool initialize(uint16_t port);
	void run(const std::atomic<bool>& running);

private:
	PacketReceiver receiver_;
	FecDepacketizer fecDepacketizer_;
	UnifiedDepacketizer depacketizer_;
	FrameDecoder decoder_;
	FrameRenderer renderer_;

	bool rendererInitialized_;

	std::vector<Packet> fecPackets;
};

#endif 
