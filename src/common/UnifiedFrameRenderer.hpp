#ifndef UNIFIED_FRAME_RENDERER_HPP
#define UNIFIED_FRAME_RENDERER_HPP

#include "common/IFrameRenderer.hpp"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class FrameRenderer final : public IFrameRenderer {
public:
    FrameRenderer() = default;
    ~FrameRenderer() override;

    bool initialize(int width, int height, const char* windowTitle) override;

    void render(const VideoFrame& frame) override;

    bool pollEvents() override;

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;

    int m_width = 0;
    int m_height = 0;
};

#endif // UNIFIED_FRAME_RENDERER_HPP