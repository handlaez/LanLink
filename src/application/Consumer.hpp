#ifndef CONSUMER_HPP
#define CONSUMER_HPP

#include "platform/linux/LnxFrameDecoder.hpp"
#include "platform/linux/LnxFrameRenderer.hpp"
#include "platform/linux/LnxPacketReceiver.hpp"
#include "common/UnifiedDepacketizer.hpp"

class Consumer {
public:
	bool initialize(uint16_t port);
	void run();

private:
	LnxPacketReceiver receiver_;
	UnifiedDepacketizer depacketizer_;
	LnxFrameDecoder decoder_;
	LnxFrameRenderer renderer;

	bool rendererInitialized_;
};

#endif 
