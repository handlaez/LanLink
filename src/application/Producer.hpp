#ifndef PRODUCER_HPP
#define PRODUCER_HPP

#include <cstdint>
#include <string>
#include <memory>
#include <atomic>

#ifdef _WIN32
#include "platform/windows/WinFrameConverter.hpp"
#include "platform/windows/WinFrameEncoder.hpp"
#include "platform/windows/WinFrameGrabber.hpp"
#include "platform/windows/WinPacketSender.hpp"
#else
#include "platform/linux/LnxFrameGrabber.hpp"
#include "platform/linux/LnxFrameConverter.hpp"
#include "platform/linux/LnxFrameEncoder.hpp"
#include "platform/linux/LnxPacketSender.hpp"
#endif

#include "common/UnifiedPacketizer.hpp"
#include "common/UnifiedFecPacketizer.hpp"

class Producer {
public:
	bool initialize(const std::string& address, uint16_t port);
	void run(std::atomic<bool>& running);

private:
	FrameGrabber frameGrabber_;
	std::unique_ptr<FrameConverter> frameConverter_;
	std::unique_ptr<FrameEncoder> frameEncoder_;
	std::unique_ptr<FecPacketizer> fecPacketizer_;
	UnifiedPacketizer packetizer_;
	PacketSender packetSender_;

	uint32_t width_ = 0;
	uint32_t height_ = 0;
};

#endif
