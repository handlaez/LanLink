#ifndef ENCODED_FRAME_HPP
#define ENCODED_FRAME_HPP

#include <cstdint>
#include <vector>

enum class EncodedFrameType {
	Unknown,
	Keyframe,
	Delta
};

struct EncodedFrame {
	std::vector<uint8_t> data;
	uint64_t timestamp = 0;
	EncodedFrameType type = EncodedFrameType::Unknown;
};

#endif