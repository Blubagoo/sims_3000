/**
 * @file NormalBuffer.h
 * @brief Normal G-buffer for screen-space edge detection.
 *
 * Stores view-space normals rendered during the scene pass for use
 * in post-process edge detection. Works alongside the depth buffer
 * to provide the primary signal for cartoon outline detection.
 *
 * Storage format: RGBA16F with normals encoded as (N * 0.5 + 0.5)
 * The alpha channel is unused but included for alignment.
 *
 * Resource ownership:
 * - NormalBuffer owns the SDL_GPUTexture for normals
 * - NormalBuffer does NOT own the GPUDevice (external ownership)
 * - Must be recreated when window dimensions change
 * - Destruction order: release normal texture -> NormalBuffer destroyed
 */

#ifndef SIMS3000_RENDER_NORMAL_BUFFER_H
#define SIMS3000_RENDER_NORMAL_BUFFER_H

#include <SDL3/SDL.h>
#include <cstdint>
#include <string>

namespace sims3000 {

// Forward declarations
class GPUDevice;

/**
 * @class NormalBuffer
 * @brief Manages view-space normal texture for edge detection.
 *
 * RAII wrapper for SDL_GPU normal texture. Creates a normal texture at the
 * specified resolution, handles recreation on resize, and provides the
 * color target info for render pass configuration.
 *
 * Usage:
 * @code
 *   GPUDevice device;
 *   NormalBuffer normalBuffer(device, 1280, 720);
 *
 *   // In render pass setup - render to both color and normal targets:
 *   SDL_GPUColorTargetInfo colorTargets[2] = {
 *       { .texture = swapchainTexture, ... },
 *       normalBuffer.getColorTargetInfo()
 *   };
 *
 *   // In edge detection pass:
 *   SDL_GPUTextureSamplerBinding normalBinding = {
 *       .texture = normalBuffer.getHandle(),
 *       .sampler = pointSampler
 *   };
 * @endcode
 */
class NormalBuffer {
public:
    /**
     * Create a normal buffer.
     * @param device GPUDevice to create texture on
     * @param width Normal texture width (should match window/swapchain width)
     * @param height Normal texture height (should match window/swapchain height)
     */
    NormalBuffer(GPUDevice& device, std::uint32_t width, std::uint32_t height);

    ~NormalBuffer();

    // Non-copyable
    NormalBuffer(const NormalBuffer&) = delete;
    NormalBuffer& operator=(const NormalBuffer&) = delete;

    // Movable
    NormalBuffer(NormalBuffer&& other) noexcept;
    NormalBuffer& operator=(NormalBuffer&& other) noexcept;

    /**
     * Check if normal buffer was created successfully.
     * @return true if normal texture is valid
     */
    bool isValid() const;

    /**
     * Get the underlying SDL GPU texture handle.
     * @return Pointer to SDL_GPUTexture, or nullptr if not initialized
     */
    SDL_GPUTexture* getHandle() const;

    /**
     * Get the current normal buffer width.
     * @return Width in pixels
     */
    std::uint32_t getWidth() const;

    /**
     * Get the current normal buffer height.
     * @return Height in pixels
     */
    std::uint32_t getHeight() const;

    /**
     * Get the SDL texture format being used.
     * @return SDL_GPUTextureFormat (always RGBA16_FLOAT)
     */
    SDL_GPUTextureFormat getFormat() const;

    /**
     * Get the last error message.
     * @return Error message string
     */
    const std::string& getLastError() const;

    // =========================================================================
    // Resize Operations
    // =========================================================================

    /**
     * Resize normal buffer to new dimensions.
     * Recreates the normal texture at the new resolution.
     * Call this when the window resizes.
     *
     * @param newWidth New width in pixels
     * @param newHeight New height in pixels
     * @return true if resize succeeded
     */
    bool resize(std::uint32_t newWidth, std::uint32_t newHeight);

    // =========================================================================
    // Render Pass Configuration
    // =========================================================================

    /**
     * Get color target info for render pass configuration.
     * Pre-configured with:
     * - load_op: SDL_GPU_LOADOP_CLEAR (clear to neutral normal)
     * - store_op: SDL_GPU_STOREOP_STORE (needed for sampling in edge pass)
     * - clear_color: (0.5, 0.5, 1.0, 1.0) (neutral up-facing normal)
     *
     * @return Configured SDL_GPUColorTargetInfo ready for render pass
     */
    SDL_GPUColorTargetInfo getColorTargetInfo() const;

    /**
     * Get color target info with custom clear color.
     * @param clearR Red clear value
     * @param clearG Green clear value
     * @param clearB Blue clear value
     * @param clearA Alpha clear value
     * @return Configured SDL_GPUColorTargetInfo
     */
    SDL_GPUColorTargetInfo getColorTargetInfo(float clearR, float clearG,
                                               float clearB, float clearA) const;

    /**
     * Get color target info that preserves existing content.
     * Uses LOAD instead of CLEAR operation.
     * Useful for multi-pass rendering.
     *
     * @return Configured SDL_GPUColorTargetInfo with load operation
     */
    SDL_GPUColorTargetInfo getColorTargetInfoPreserve() const;

private:
    /**
     * Create the normal texture with current settings.
     * @return true if texture was created successfully
     */
    bool createTexture();

    /**
     * Release the normal texture.
     */
    void releaseTexture();

    SDL_GPUDevice* m_device = nullptr;  // Non-owning pointer
    SDL_GPUTexture* m_texture = nullptr;

    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;

    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_NORMAL_BUFFER_H
