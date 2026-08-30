#ifndef CONSUMER_HPP
#define CONSUMER_HPP

#include <atomic>

#include "platform/linux/LnxFrameDecoder.hpp"
#include "platform/linux/LnxFrameRenderer.hpp"
#include "platform/linux/LnxPacketReceiver.hpp"
#include "common/UnifiedDepacketizer.hpp"
#include "common/UnifiedFecDepacketizer.hpp"
#include "common/UDPFrameHeader.hpp"

class Consumer {
public:
	bool initialize(uint16_t port);
	void run(const std::atomic<bool>& running);

private:
	LnxPacketReceiver receiver_;
	FecDepacketizer fecDepacketizer_;
	UnifiedDepacketizer depacketizer_;
	LnxFrameDecoder decoder_;
	LnxFrameRenderer renderer_;

	bool rendererInitialized_;

	std::vector<Packet> fecPackets;
};

#endif 
