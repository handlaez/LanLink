#ifndef I_FRAME_RENDERER_HPP
#define I_FRAME_RENDERER_HPP

#include "common/VideoFrame.hpp"

class IFrameRenderer {
public:
    virtual ~IFrameRenderer() = default;

    virtual bool initialize(int width, int height, const char* windowTitle) = 0;

    virtual void render(const VideoFrame& frame) = 0;

    virtual bool pollEvents() = 0;
};

#endif