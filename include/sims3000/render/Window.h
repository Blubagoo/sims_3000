/**
 * @file Window.h
 * @brief SDL3 window and GPU device management.
 */

#ifndef SIMS3000_RENDER_WINDOW_H
#define SIMS3000_RENDER_WINDOW_H

#include <SDL3/SDL.h>
#include <cstdint>

namespace sims3000 {

/**
 * @class Window
 * @brief Manages SDL3 window and SDL_GPU device.
 *
 * RAII wrapper for SDL window and GPU resources.
 * Handles window creation, swapchain management,
 * resize handling, and fullscreen toggling.
 */
class Window {
public:
    /**
     * Create a window with the specified dimensions.
     * @param title Window title
     * @param width Initial window width
     * @param height Initial window height
     */
    Window(const char* title, int width, int height);
    ~Window();

    // Non-copyable, movable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    /**
     * Check if initialization succeeded.
     */
    bool isValid() const;

    /**
     * Get the SDL window handle.
     */
    SDL_Window* getWindow() const;

    /**
     * Get the GPU device handle.
     */
    SDL_GPUDevice* getDevice() const;

    /**
     * Get current window width.
     */
    int getWidth() const;

    /**
     * Get current window height.
     */
    int getHeight() const;

    /**
     * Handle window resize.
     * Called when SDL_EVENT_WINDOW_RESIZED is received.
     * @param newWidth New window width
     * @param newHeight New window height
     */
    void onResize(int newWidth, int newHeight);

    /**
     * Toggle fullscreen mode.
     * Uses borderless fullscreen desktop mode.
     */
    void toggleFullscreen();

    /**
     * Check if window is fullscreen.
     */
    bool isFullscreen() const;

    /**
     * Acquire command buffer for this frame.
     * @return Command buffer, or nullptr on failure
     */
    SDL_GPUCommandBuffer* acquireCommandBuffer();

    /**
     * Acquire swapchain texture for rendering.
     * @param cmdBuffer Active command buffer
     * @param outTexture Receives swapchain texture
     * @return true if texture acquired successfully
     */
    bool acquireSwapchainTexture(SDL_GPUCommandBuffer* cmdBuffer,
                                  SDL_GPUTexture** outTexture);

    /**
     * Submit command buffer.
     * @param cmdBuffer Command buffer to submit
     * @return true on success
     */
    bool submit(SDL_GPUCommandBuffer* cmdBuffer);

private:
    void cleanup();

    SDL_Window* m_window = nullptr;
    SDL_GPUDevice* m_device = nullptr;
    int m_width = 0;
    int m_height = 0;
    bool m_fullscreen = false;
    int m_windowedWidth = 0;
    int m_windowedHeight = 0;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_WINDOW_H
