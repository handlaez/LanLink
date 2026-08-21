#include <SDL2/SDL.h>

extern "C" {
#include <libavutil/frame.h>
}

#include "LnxFrameRenderer.hpp"
#include "Logger.hpp"

LnxFrameRenderer::~LnxFrameRenderer()
{
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }

    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
}

bool LnxFrameRenderer::initialize(
    int width,
    int height,
    const char* windowTitle)
{
    if (width <= 0 || height <= 0 || windowTitle == nullptr) {
        return false;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        logger().error(QString("SDL_Init failed: %1").arg(SDL_GetError()));
        return false;
    }

    m_window = SDL_CreateWindow(
        windowTitle,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width,
        height,
        SDL_WINDOW_FULLSCREEN_DESKTOP);

    if (!m_window) {
        logger().error(QString("SDL_CreateWindow failed: %1").arg(SDL_GetError()));
        SDL_Quit();
        return false;
    }

    m_renderer = SDL_CreateRenderer(
        m_window,
        -1,
        SDL_RENDERER_ACCELERATED);

    if (!m_renderer) {
        logger().error(QString("SDL_CreateRenderer failed: %1").arg(SDL_GetError()));
        SDL_DestroyWindow(m_window);
        m_window = nullptr;

        SDL_Quit();
        return false;
    }

    m_width = width;
    m_height = height;

    return true;
}

void LnxFrameRenderer::render(const VideoFrame& frame)
{
    if (!m_renderer || frame.nativeResource == nullptr) {
        return;
    }

    auto* avFrame = static_cast<AVFrame*>(frame.nativeResource);

    if (!avFrame ||
        avFrame->data[0] == nullptr ||
        avFrame->data[1] == nullptr ||
        avFrame->data[2] == nullptr) {
        return;
    }

    if (!m_texture ||
        frame.width != static_cast<uint32_t>(m_width) ||
        frame.height != static_cast<uint32_t>(m_height)) {

        if (m_texture) {
            SDL_DestroyTexture(m_texture);
            m_texture = nullptr;
        }

        m_texture = SDL_CreateTexture(
            m_renderer,
            SDL_PIXELFORMAT_IYUV,
            SDL_TEXTUREACCESS_STREAMING,
            static_cast<int>(frame.width),
            static_cast<int>(frame.height));

        if (!m_texture) {
            logger().error(QString("SDL_CreateTexture failed: %1").arg(SDL_GetError()));
            return;
        }

        m_width = static_cast<int>(frame.width);
        m_height = static_cast<int>(frame.height);
    }

    SDL_UpdateYUVTexture(
        m_texture,
        nullptr,
        avFrame->data[0],
        avFrame->linesize[0],
        avFrame->data[1],
        avFrame->linesize[1],
        avFrame->data[2],
        avFrame->linesize[2]);

    SDL_RenderClear(m_renderer);

    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);

    SDL_RenderPresent(m_renderer);
}

bool LnxFrameRenderer::pollEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
    }

    return true;
}