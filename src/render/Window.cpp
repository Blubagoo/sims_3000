/**
 * @file Window.cpp
 * @brief Window implementation with SDL_GPU.
 */

#include "sims3000/render/Window.h"
#include <SDL3/SDL_log.h>

namespace sims3000 {

Window::Window(const char* title, int width, int height)
    : m_width(width)
    , m_height(height)
    , m_windowedWidth(width)
    , m_windowedHeight(height)
{
    // Create window
    m_window = SDL_CreateWindow(
        title,
        width,
        height,
        SDL_WINDOW_RESIZABLE
    );

    if (!m_window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return;
    }

    // Create GPU device with SPIRV and DXIL support
    m_device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL,
        true,  // debug mode
        nullptr  // prefer any device
    );

    if (!m_device) {
        SDL_Log("Failed to create GPU device: %s", SDL_GetError());
        cleanup();
        return;
    }

    // Claim window for GPU rendering
    if (!SDL_ClaimWindowForGPUDevice(m_device, m_window)) {
        SDL_Log("Failed to claim window for GPU: %s", SDL_GetError());
        cleanup();
        return;
    }

    SDL_Log("Window created: %dx%d", width, height);
    SDL_Log("GPU Device: %s", SDL_GetGPUDeviceDriver(m_device));
}

Window::~Window() {
    cleanup();
}

Window::Window(Window&& other) noexcept
    : m_window(other.m_window)
    , m_device(other.m_device)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_fullscreen(other.m_fullscreen)
    , m_windowedWidth(other.m_windowedWidth)
    , m_windowedHeight(other.m_windowedHeight)
{
    other.m_window = nullptr;
    other.m_device = nullptr;
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        cleanup();

        m_window = other.m_window;
        m_device = other.m_device;
        m_width = other.m_width;
        m_height = other.m_height;
        m_fullscreen = other.m_fullscreen;
        m_windowedWidth = other.m_windowedWidth;
        m_windowedHeight = other.m_windowedHeight;

        other.m_window = nullptr;
        other.m_device = nullptr;
    }
    return *this;
}

bool Window::isValid() const {
    return m_window != nullptr && m_device != nullptr;
}

SDL_Window* Window::getWindow() const {
    return m_window;
}

SDL_GPUDevice* Window::getDevice() const {
    return m_device;
}

int Window::getWidth() const {
    return m_width;
}

int Window::getHeight() const {
    return m_height;
}

void Window::onResize(int newWidth, int newHeight) {
    m_width = newWidth;
    m_height = newHeight;

    if (!m_fullscreen) {
        m_windowedWidth = newWidth;
        m_windowedHeight = newHeight;
    }

    SDL_Log("Window resized: %dx%d", newWidth, newHeight);
}

void Window::toggleFullscreen() {
    m_fullscreen = !m_fullscreen;

    if (m_fullscreen) {
        SDL_SetWindowFullscreen(m_window, true);
    } else {
        SDL_SetWindowFullscreen(m_window, false);
        SDL_SetWindowSize(m_window, m_windowedWidth, m_windowedHeight);
    }

    SDL_Log("Fullscreen: %s", m_fullscreen ? "on" : "off");
}

bool Window::isFullscreen() const {
    return m_fullscreen;
}

SDL_GPUCommandBuffer* Window::acquireCommandBuffer() {
    return SDL_AcquireGPUCommandBuffer(m_device);
}

bool Window::acquireSwapchainTexture(SDL_GPUCommandBuffer* cmdBuffer,
                                      SDL_GPUTexture** outTexture) {
    Uint32 width, height;
    return SDL_WaitAndAcquireGPUSwapchainTexture(
        cmdBuffer,
        m_window,
        outTexture,
        &width,
        &height
    );
}

bool Window::submit(SDL_GPUCommandBuffer* cmdBuffer) {
    return SDL_SubmitGPUCommandBuffer(cmdBuffer);
}

void Window::cleanup() {
    if (m_device) {
        if (m_window) {
            SDL_ReleaseWindowFromGPUDevice(m_device, m_window);
        }
        SDL_DestroyGPUDevice(m_device);
        m_device = nullptr;
    }

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
}

} // namespace sims3000
