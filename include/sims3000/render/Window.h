/**
 * @file Window.h
 * @brief SDL3 window management with GPU swap chain integration.
 *
 * Manages SDL window lifecycle, swap chain configuration, resize handling,
 * and fullscreen toggling. Works with an external GPUDevice for rendering.
 *
 * Resource ownership:
 * - Window owns the SDL_Window
 * - Window does NOT own the GPUDevice (external ownership)
 * - Swap chain is managed by SDL_GPU when window is claimed
 * - Destruction order: release window claim -> destroy window
 */

#ifndef SIMS3000_RENDER_WINDOW_H
#define SIMS3000_RENDER_WINDOW_H

#include <SDL3/SDL.h>
#include <cstdint>
#include <string>

namespace sims3000 {

// Forward declaration
class GPUDevice;

/**
 * @enum PresentMode
 * @brief Swap chain presentation modes for controlling vsync behavior.
 */
enum class PresentMode {
    /**
     * Immediate mode (no vsync).
     * Lowest latency but may cause screen tearing.
     * Maps to SDL_GPU_PRESENTMODE_IMMEDIATE.
     */
    Immediate,

    /**
     * VSync mode.
     * Waits for vertical blank, no tearing but higher latency.
     * Maps to SDL_GPU_PRESENTMODE_VSYNC.
     */
    VSync,

    /**
     * Mailbox mode (adaptive vsync / triple buffering).
     * Low latency without tearing when possible.
     * Maps to SDL_GPU_PRESENTMODE_MAILBOX.
     * Falls back to VSync if not supported.
     */
    Mailbox
};

/**
 * @struct SwapChainConfig
 * @brief Configuration options for swap chain creation.
 */
struct SwapChainConfig {
    PresentMode presentMode = PresentMode::VSync;
    SDL_GPUSwapchainComposition composition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
};

/**
 * @class Window
 * @brief Manages SDL3 window with GPU swap chain integration.
 *
 * RAII wrapper for SDL window. Handles window creation, swap chain claiming
 * via external GPUDevice, resize handling, present mode configuration,
 * and fullscreen toggling.
 *
 * Usage:
 * @code
 *   GPUDevice device;
 *   Window window("My Game", 1280, 720);
 *
 *   if (window.isValid() && window.claimForDevice(device)) {
 *       // Configure swap chain
 *       window.setPresentMode(PresentMode::VSync);
 *
 *       // Render loop
 *       SDL_GPUCommandBuffer* cmd = device.acquireCommandBuffer();
 *       SDL_GPUTexture* swapchain = nullptr;
 *       if (window.acquireSwapchainTexture(cmd, &swapchain)) {
 *           // ... render to swapchain ...
 *       }
 *       device.submit(cmd);
 *   }
 * @endcode
 */
class Window {
public:
    /**
     * Create a window with the specified dimensions.
     * Window is created but not yet claimed for GPU rendering.
     * Call claimForDevice() to enable GPU rendering.
     *
     * @param title Window title
     * @param width Initial window width
     * @param height Initial window height
     */
    Window(const char* title, int width, int height);

    ~Window();

    // Non-copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Movable
    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    /**
     * Check if window was created successfully.
     * @return true if window is valid
     */
    bool isValid() const;

    /**
     * Check if window is claimed by a GPU device.
     * @return true if claimed for GPU rendering
     */
    bool isClaimed() const;

    /**
     * Get the SDL window handle.
     * @return Pointer to SDL_Window, or nullptr if not initialized
     */
    SDL_Window* getHandle() const;

    /**
     * Get the associated GPU device (if claimed).
     * @return Pointer to SDL_GPUDevice, or nullptr if not claimed
     */
    SDL_GPUDevice* getDevice() const;

    /**
     * Get current window width.
     * @return Window width in pixels
     */
    int getWidth() const;

    /**
     * Get current window height.
     * @return Window height in pixels
     */
    int getHeight() const;

    /**
     * Get the last error message.
     * @return Error message string
     */
    const std::string& getLastError() const;

    // =========================================================================
    // GPU Device Integration
    // =========================================================================

    /**
     * Claim this window for GPU rendering via the specified device.
     * Must be called before using swap chain operations.
     *
     * @param device GPUDevice to claim this window for
     * @return true if successfully claimed
     */
    bool claimForDevice(GPUDevice& device);

    /**
     * Claim this window for GPU rendering via raw SDL device handle.
     * Prefer claimForDevice(GPUDevice&) when possible.
     *
     * @param device SDL_GPUDevice handle
     * @return true if successfully claimed
     */
    bool claimForDevice(SDL_GPUDevice* device);

    /**
     * Release this window from GPU device ownership.
     * Safe to call even if not claimed.
     */
    void releaseFromDevice();

    // =========================================================================
    // Swap Chain Configuration
    // =========================================================================

    /**
     * Get the current present mode.
     * @return Current presentation mode
     */
    PresentMode getPresentMode() const;

    /**
     * Set the presentation mode (vsync behavior).
     * Takes effect on next frame.
     *
     * @param mode Desired presentation mode
     * @return true if mode was set successfully
     */
    bool setPresentMode(PresentMode mode);

    /**
     * Check if a specific present mode is supported.
     * @param mode Present mode to check
     * @return true if mode is supported by the current device
     */
    bool supportsPresentMode(PresentMode mode) const;

    /**
     * Get the current swap chain composition.
     * @return Current composition mode (SDR, HDR, etc.)
     */
    SDL_GPUSwapchainComposition getSwapchainComposition() const;

    /**
     * Set swap chain composition (SDR/HDR mode).
     * @param composition Desired composition mode
     * @return true if composition was set successfully
     */
    bool setSwapchainComposition(SDL_GPUSwapchainComposition composition);

    /**
     * Get the current swap chain texture format.
     * @return Texture format of the swap chain
     */
    SDL_GPUTextureFormat getSwapchainTextureFormat() const;

    // =========================================================================
    // Swap Chain Operations
    // =========================================================================

    /**
     * Acquire swapchain texture for rendering.
     * Blocks until a swapchain texture is available.
     *
     * @param cmdBuffer Active command buffer (from GPUDevice)
     * @param outTexture Receives swapchain texture
     * @return true if texture acquired successfully
     */
    bool acquireSwapchainTexture(SDL_GPUCommandBuffer* cmdBuffer,
                                  SDL_GPUTexture** outTexture);

    /**
     * Acquire swapchain texture with dimensions.
     * @param cmdBuffer Active command buffer
     * @param outTexture Receives swapchain texture
     * @param outWidth Receives texture width
     * @param outHeight Receives texture height
     * @return true if texture acquired successfully
     */
    bool acquireSwapchainTexture(SDL_GPUCommandBuffer* cmdBuffer,
                                  SDL_GPUTexture** outTexture,
                                  uint32_t* outWidth, uint32_t* outHeight);

    // =========================================================================
    // Window State Management
    // =========================================================================

    /**
     * Handle window resize event.
     * Called when SDL_EVENT_WINDOW_RESIZED is received.
     * Swap chain is automatically recreated by SDL_GPU.
     *
     * @param newWidth New window width
     * @param newHeight New window height
     */
    void onResize(int newWidth, int newHeight);

    /**
     * Toggle between windowed and fullscreen modes.
     * Uses borderless fullscreen desktop mode.
     * Swap chain is automatically handled.
     */
    void toggleFullscreen();

    /**
     * Set fullscreen mode explicitly.
     * @param fullscreen true for fullscreen, false for windowed
     */
    void setFullscreen(bool fullscreen);

    /**
     * Check if window is in fullscreen mode.
     * @return true if fullscreen
     */
    bool isFullscreen() const;

    // =========================================================================
    // Deprecated API (for backward compatibility)
    // =========================================================================

    /**
     * @deprecated Use getHandle() instead.
     */
    SDL_Window* getWindow() const { return getHandle(); }

private:
    void cleanup();

    /**
     * Apply current swap chain configuration to the device.
     * @return true if configuration was applied successfully
     */
    bool applySwapchainConfig();

    SDL_Window* m_window = nullptr;
    SDL_GPUDevice* m_device = nullptr;  // Non-owning pointer

    int m_width = 0;
    int m_height = 0;
    bool m_fullscreen = false;
    int m_windowedWidth = 0;
    int m_windowedHeight = 0;

    // Swap chain configuration
    PresentMode m_presentMode = PresentMode::VSync;
    SDL_GPUSwapchainComposition m_composition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;

    std::string m_lastError;
};

/**
 * Convert PresentMode enum to string.
 * @param mode Present mode enum value
 * @return Human-readable string representation
 */
const char* getPresentModeName(PresentMode mode);

/**
 * Convert PresentMode to SDL_GPUPresentMode.
 * @param mode Present mode enum value
 * @return SDL presentation mode constant
 */
SDL_GPUPresentMode toSDLPresentMode(PresentMode mode);

/**
 * Convert SDL_GPUPresentMode to PresentMode.
 * @param mode SDL presentation mode constant
 * @return PresentMode enum value
 */
PresentMode fromSDLPresentMode(SDL_GPUPresentMode mode);

} // namespace sims3000

#endif // SIMS3000_RENDER_WINDOW_H
