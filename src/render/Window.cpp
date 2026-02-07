/**
 * @file Window.cpp
 * @brief Window implementation with SDL_GPU swap chain integration.
 */

#include "sims3000/render/Window.h"
#include "sims3000/render/GPUDevice.h"
#include <SDL3/SDL_log.h>

namespace sims3000 {

// =============================================================================
// Free Functions
// =============================================================================

const char* getPresentModeName(PresentMode mode) {
    switch (mode) {
        case PresentMode::Immediate: return "Immediate";
        case PresentMode::VSync:     return "VSync";
        case PresentMode::Mailbox:   return "Mailbox";
        default:                     return "Unknown";
    }
}

SDL_GPUPresentMode toSDLPresentMode(PresentMode mode) {
    switch (mode) {
        case PresentMode::Immediate: return SDL_GPU_PRESENTMODE_IMMEDIATE;
        case PresentMode::VSync:     return SDL_GPU_PRESENTMODE_VSYNC;
        case PresentMode::Mailbox:   return SDL_GPU_PRESENTMODE_MAILBOX;
        default:                     return SDL_GPU_PRESENTMODE_VSYNC;
    }
}

PresentMode fromSDLPresentMode(SDL_GPUPresentMode mode) {
    switch (mode) {
        case SDL_GPU_PRESENTMODE_IMMEDIATE: return PresentMode::Immediate;
        case SDL_GPU_PRESENTMODE_VSYNC:     return PresentMode::VSync;
        case SDL_GPU_PRESENTMODE_MAILBOX:   return PresentMode::Mailbox;
        default:                            return PresentMode::VSync;
    }
}

// =============================================================================
// Construction / Destruction
// =============================================================================

Window::Window(const char* title, int width, int height)
    : m_width(width)
    , m_height(height)
    , m_windowedWidth(width)
    , m_windowedHeight(height)
{
    // Create window with resizable flag
    m_window = SDL_CreateWindow(
        title,
        width,
        height,
        SDL_WINDOW_RESIZABLE
    );

    if (!m_window) {
        m_lastError = "Failed to create window: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "%s", m_lastError.c_str());
        return;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
                "Window created: %s (%dx%d)", title, width, height);
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
    , m_presentMode(other.m_presentMode)
    , m_composition(other.m_composition)
    , m_lastError(std::move(other.m_lastError))
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
        m_presentMode = other.m_presentMode;
        m_composition = other.m_composition;
        m_lastError = std::move(other.m_lastError);

        other.m_window = nullptr;
        other.m_device = nullptr;
    }
    return *this;
}

void Window::cleanup() {
    // Release from GPU device first
    releaseFromDevice();

    // Destroy window
    if (m_window) {
        SDL_DestroyWindow(m_window);
        SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "Window: Destroyed window");
        m_window = nullptr;
    }
}

// =============================================================================
// State Queries
// =============================================================================

bool Window::isValid() const {
    return m_window != nullptr;
}

bool Window::isClaimed() const {
    return m_window != nullptr && m_device != nullptr;
}

SDL_Window* Window::getHandle() const {
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

const std::string& Window::getLastError() const {
    return m_lastError;
}

// =============================================================================
// GPU Device Integration
// =============================================================================

bool Window::claimForDevice(GPUDevice& device) {
    return claimForDevice(device.getHandle());
}

bool Window::claimForDevice(SDL_GPUDevice* device) {
    if (!m_window) {
        m_lastError = "Cannot claim: window is not valid";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    if (!device) {
        m_lastError = "Cannot claim: device is null";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    // Release any existing claim first
    if (m_device) {
        releaseFromDevice();
    }

    // Claim window for GPU device
    if (!SDL_ClaimWindowForGPUDevice(device, m_window)) {
        m_lastError = "Failed to claim window for GPU device: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    m_device = device;

    // Apply initial swap chain configuration
    if (!applySwapchainConfig()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                    "Window: Failed to apply initial swap chain config, using defaults");
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU,
                "Window: Claimed for GPU device (present mode: %s)",
                getPresentModeName(m_presentMode));

    return true;
}

void Window::releaseFromDevice() {
    if (m_device && m_window) {
        SDL_ReleaseWindowFromGPUDevice(m_device, m_window);
        SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "Window: Released from GPU device");
    }
    m_device = nullptr;
}

// =============================================================================
// Swap Chain Configuration
// =============================================================================

PresentMode Window::getPresentMode() const {
    return m_presentMode;
}

bool Window::setPresentMode(PresentMode mode) {
    if (!isClaimed()) {
        m_lastError = "Cannot set present mode: window not claimed";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    // Check if mode is supported
    if (!supportsPresentMode(mode)) {
        // Try to fall back gracefully
        if (mode == PresentMode::Mailbox) {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                        "Window: Mailbox mode not supported, falling back to VSync");
            mode = PresentMode::VSync;
        } else {
            m_lastError = "Present mode not supported: ";
            m_lastError += getPresentModeName(mode);
            SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
            return false;
        }
    }

    m_presentMode = mode;

    if (!applySwapchainConfig()) {
        m_lastError = "Failed to apply present mode: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU,
                "Window: Set present mode to %s", getPresentModeName(mode));

    return true;
}

bool Window::supportsPresentMode(PresentMode mode) const {
    if (!isClaimed()) {
        return false;
    }

    return SDL_WindowSupportsGPUPresentMode(
        m_device, m_window, toSDLPresentMode(mode));
}

SDL_GPUSwapchainComposition Window::getSwapchainComposition() const {
    return m_composition;
}

bool Window::setSwapchainComposition(SDL_GPUSwapchainComposition composition) {
    if (!isClaimed()) {
        m_lastError = "Cannot set composition: window not claimed";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    // Check if composition is supported
    if (!SDL_WindowSupportsGPUSwapchainComposition(m_device, m_window, composition)) {
        m_lastError = "Swapchain composition not supported";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    m_composition = composition;

    if (!applySwapchainConfig()) {
        m_lastError = "Failed to apply swapchain composition: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "Window: Set swapchain composition");

    return true;
}

SDL_GPUTextureFormat Window::getSwapchainTextureFormat() const {
    if (!isClaimed()) {
        return SDL_GPU_TEXTUREFORMAT_INVALID;
    }

    return SDL_GetGPUSwapchainTextureFormat(m_device, m_window);
}

bool Window::applySwapchainConfig() {
    if (!isClaimed()) {
        return false;
    }

    return SDL_SetGPUSwapchainParameters(
        m_device,
        m_window,
        m_composition,
        toSDLPresentMode(m_presentMode)
    );
}

// =============================================================================
// Swap Chain Operations
// =============================================================================

bool Window::acquireSwapchainTexture(SDL_GPUCommandBuffer* cmdBuffer,
                                      SDL_GPUTexture** outTexture) {
    uint32_t width, height;
    return acquireSwapchainTexture(cmdBuffer, outTexture, &width, &height);
}

bool Window::acquireSwapchainTexture(SDL_GPUCommandBuffer* cmdBuffer,
                                      SDL_GPUTexture** outTexture,
                                      uint32_t* outWidth, uint32_t* outHeight) {
    if (!isClaimed()) {
        m_lastError = "Cannot acquire swapchain: window not claimed";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    if (!cmdBuffer) {
        m_lastError = "Cannot acquire swapchain: command buffer is null";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    if (!outTexture) {
        m_lastError = "Cannot acquire swapchain: output texture pointer is null";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    // Wait for and acquire the swapchain texture
    // This blocks until a texture is available
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            cmdBuffer, m_window, outTexture, outWidth, outHeight)) {
        m_lastError = "Failed to acquire swapchain texture: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    // Update cached dimensions if they changed (e.g., after resize)
    if (outWidth && outHeight) {
        if (static_cast<int>(*outWidth) != m_width ||
            static_cast<int>(*outHeight) != m_height) {
            m_width = static_cast<int>(*outWidth);
            m_height = static_cast<int>(*outHeight);
        }
    }

    return true;
}

// =============================================================================
// Window State Management
// =============================================================================

void Window::onResize(int newWidth, int newHeight) {
    m_width = newWidth;
    m_height = newHeight;

    // Save windowed dimensions for fullscreen toggle
    if (!m_fullscreen) {
        m_windowedWidth = newWidth;
        m_windowedHeight = newHeight;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
                "Window: Resized to %dx%d", newWidth, newHeight);

    // Note: SDL_GPU automatically handles swap chain recreation on resize.
    // No explicit action needed here - the next acquireSwapchainTexture()
    // will get a texture with the new dimensions.
}

void Window::toggleFullscreen() {
    setFullscreen(!m_fullscreen);
}

void Window::setFullscreen(bool fullscreen) {
    if (!m_window) {
        return;
    }

    if (m_fullscreen == fullscreen) {
        return;  // No change needed
    }

    m_fullscreen = fullscreen;

    if (m_fullscreen) {
        // Enter borderless fullscreen desktop mode
        if (!SDL_SetWindowFullscreen(m_window, true)) {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
                         "Window: Failed to enter fullscreen: %s", SDL_GetError());
            m_fullscreen = false;
            return;
        }
    } else {
        // Exit fullscreen
        if (!SDL_SetWindowFullscreen(m_window, false)) {
            SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
                         "Window: Failed to exit fullscreen: %s", SDL_GetError());
            return;
        }

        // Restore windowed dimensions
        SDL_SetWindowSize(m_window, m_windowedWidth, m_windowedHeight);
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
                "Window: Fullscreen %s", m_fullscreen ? "enabled" : "disabled");

    // Note: SDL_GPU automatically handles swap chain recreation.
    // The next frame will use the new dimensions.
}

bool Window::isFullscreen() const {
    return m_fullscreen;
}

} // namespace sims3000
