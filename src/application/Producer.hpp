#ifndef PRODUCER_HPP
#define PRODUCER_HPP

#include <cstdint>
#include <string>
#include <memory>

// temporarily here:
#include <d3d11.h>
#include <wrl/client.h>
// 

#include "platform/windows/WinFrameConverter.hpp"
#include "platform/windows/WinFrameEncoder.hpp"
#include "platform/windows/WinFrameGrabber.hpp"
#include "platform/windows/WinPacketSender.hpp"
#include "common/UnifiedPacketizer.hpp"

class Producer {
public:
	bool initialize(const std::string& address, uint16_t port);
	void run();

private:
	WinFrameGrabber frameGrabber_;
	std::unique_ptr<WinFrameConverter> frameConverter_;
	std::unique_ptr<WinFrameEncoder> frameEncoder_;
	UnifiedPacketizer packetizer_;
	WinPacketSender packetSender_;

	uint32_t width_ = 0;
	uint32_t height_ = 0;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> nv12Texture_;
	VideoFrame nv12Frame_{};
};

#endif
