#ifndef VIDEO_FRAME_HPP
#define VIDEO_FRAME_HPP

#include <cstdint>

struct VideoFrame {
	void* nativeResource = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	uint64_t timestamp = 0;
};

#endif // !VIDEO_FRAME_HPP
