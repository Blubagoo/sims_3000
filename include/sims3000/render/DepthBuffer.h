/**
 * @file DepthBuffer.h
 * @brief Depth texture management for SDL_GPU automatic occlusion handling.
 *
 * Manages depth texture creation, recreation on window resize, and depth clear
 * operations. Works with GPUDevice for texture allocation.
 *
 * Resource ownership:
 * - DepthBuffer owns the SDL_GPUTexture for depth
 * - DepthBuffer does NOT own the GPUDevice (external ownership)
 * - Must be recreated when window dimensions change
 * - Destruction order: release depth texture -> DepthBuffer destroyed
 */

#ifndef SIMS3000_RENDER_DEPTH_BUFFER_H
#define SIMS3000_RENDER_DEPTH_BUFFER_H

#include <SDL3/SDL.h>
#include <cstdint>
#include <string>

namespace sims3000 {

// Forward declarations
class GPUDevice;

/**
 * @enum DepthFormat
 * @brief Supported depth buffer formats.
 */
enum class DepthFormat {
    /**
     * 32-bit floating point depth.
     * Higher precision, no stencil buffer.
     * Preferred format for most use cases.
     * Maps to SDL_GPU_TEXTUREFORMAT_D32_FLOAT.
     */
    D32_FLOAT,

    /**
     * 24-bit depth with 8-bit stencil.
     * Standard format when stencil operations are needed.
     * Maps to SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT.
     */
    D24_UNORM_S8_UINT
};

/**
 * @class DepthBuffer
 * @brief Manages depth texture for automatic occlusion handling.
 *
 * RAII wrapper for SDL_GPU depth texture. Creates a depth texture at the
 * specified resolution, handles recreation on resize, and provides the
 * depth-stencil target info for render pass configuration.
 *
 * Usage:
 * @code
 *   GPUDevice device;
 *   Window window("My Game", 1280, 720);
 *   window.claimForDevice(device);
 *
 *   DepthBuffer depthBuffer(device, window.getWidth(), window.getHeight());
 *
 *   // In render loop
 *   SDL_GPUCommandBuffer* cmd = device.acquireCommandBuffer();
 *   SDL_GPUTexture* swapchain = nullptr;
 *   window.acquireSwapchainTexture(cmd, &swapchain);
 *
 *   // Configure render pass with depth
 *   SDL_GPUColorTargetInfo colorTarget = {};
 *   colorTarget.texture = swapchain;
 *   colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
 *   colorTarget.store_op = SDL_GPU_STOREOP_STORE;
 *   colorTarget.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
 *
 *   SDL_GPUDepthStencilTargetInfo depthTarget = depthBuffer.getDepthStencilTargetInfo();
 *   // Depth is cleared to 1.0 by default via getDepthStencilTargetInfo()
 *
 *   SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, &depthTarget);
 *   // ... render with automatic depth testing ...
 *   SDL_EndGPURenderPass(pass);
 *
 *   // On window resize
 *   window.onResize(newWidth, newHeight);
 *   depthBuffer.resize(newWidth, newHeight);
 * @endcode
 */
class DepthBuffer {
public:
    /**
     * Create a depth buffer with D32_FLOAT format (preferred).
     * @param device GPUDevice to create texture on
     * @param width Depth texture width (should match window/swapchain width)
     * @param height Depth texture height (should match window/swapchain height)
     */
    DepthBuffer(GPUDevice& device, uint32_t width, uint32_t height);

    /**
     * Create a depth buffer with specified format.
     * @param device GPUDevice to create texture on
     * @param width Depth texture width
     * @param height Depth texture height
     * @param format Depth format (D32_FLOAT or D24_UNORM_S8_UINT)
     */
    DepthBuffer(GPUDevice& device, uint32_t width, uint32_t height, DepthFormat format);

    ~DepthBuffer();

    // Non-copyable
    DepthBuffer(const DepthBuffer&) = delete;
    DepthBuffer& operator=(const DepthBuffer&) = delete;

    // Movable
    DepthBuffer(DepthBuffer&& other) noexcept;
    DepthBuffer& operator=(DepthBuffer&& other) noexcept;

    /**
     * Check if depth buffer was created successfully.
     * @return true if depth texture is valid
     */
    bool isValid() const;

    /**
     * Get the underlying SDL GPU texture handle.
     * @return Pointer to SDL_GPUTexture, or nullptr if not initialized
     */
    SDL_GPUTexture* getHandle() const;

    /**
     * Get the current depth buffer width.
     * @return Width in pixels
     */
    uint32_t getWidth() const;

    /**
     * Get the current depth buffer height.
     * @return Height in pixels
     */
    uint32_t getHeight() const;

    /**
     * Get the depth format being used.
     * @return Current depth format
     */
    DepthFormat getFormat() const;

    /**
     * Get the SDL texture format being used.
     * @return SDL_GPUTextureFormat corresponding to current depth format
     */
    SDL_GPUTextureFormat getSDLFormat() const;

    /**
     * Check if stencil operations are available.
     * @return true if format includes stencil buffer (D24_UNORM_S8_UINT)
     */
    bool hasStencil() const;

    /**
     * Get the last error message.
     * @return Error message string
     */
    const std::string& getLastError() const;

    // =========================================================================
    // Resize Operations
    // =========================================================================

    /**
     * Resize depth buffer to new dimensions.
     * Recreates the depth texture at the new resolution.
     * Call this when the window resizes.
     *
     * @param newWidth New width in pixels
     * @param newHeight New height in pixels
     * @return true if resize succeeded
     */
    bool resize(uint32_t newWidth, uint32_t newHeight);

    // =========================================================================
    // Render Pass Configuration
    // =========================================================================

    /**
     * Get depth-stencil target info for render pass configuration.
     * Pre-configured with:
     * - load_op: SDL_GPU_LOADOP_CLEAR (clears depth at frame start)
     * - store_op: SDL_GPU_STOREOP_DONT_CARE (depth not needed after pass)
     * - clear_depth: 1.0f (far plane, standard depth clear value)
     * - clear_stencil: 0 (if stencil available)
     *
     * @return Configured SDL_GPUDepthStencilTargetInfo ready for render pass
     */
    SDL_GPUDepthStencilTargetInfo getDepthStencilTargetInfo() const;

    /**
     * Get depth-stencil target info with custom clear depth.
     * @param clearDepth Depth clear value (0.0 = near, 1.0 = far)
     * @return Configured SDL_GPUDepthStencilTargetInfo
     */
    SDL_GPUDepthStencilTargetInfo getDepthStencilTargetInfo(float clearDepth) const;

    /**
     * Get depth-stencil target info with custom clear values.
     * @param clearDepth Depth clear value
     * @param clearStencil Stencil clear value (only used if format has stencil)
     * @return Configured SDL_GPUDepthStencilTargetInfo
     */
    SDL_GPUDepthStencilTargetInfo getDepthStencilTargetInfo(float clearDepth, uint8_t clearStencil) const;

    /**
     * Get depth-stencil target info that preserves existing depth.
     * Uses LOAD instead of CLEAR operation.
     * Useful for multi-pass rendering.
     *
     * @return Configured SDL_GPUDepthStencilTargetInfo with load operation
     */
    SDL_GPUDepthStencilTargetInfo getDepthStencilTargetInfoPreserve() const;

    /**
     * Get depth-stencil target info that stores depth for later sampling.
     * Uses STORE operation instead of DONT_CARE.
     * Required for post-process effects that read the depth buffer.
     *
     * @param clearDepth Depth clear value (0.0 = near, 1.0 = far)
     * @return Configured SDL_GPUDepthStencilTargetInfo with store operation
     */
    SDL_GPUDepthStencilTargetInfo getDepthStencilTargetInfoSampleable(float clearDepth = 1.0f) const;

private:
    /**
     * Create the depth texture with current settings.
     * @return true if texture was created successfully
     */
    bool createTexture();

    /**
     * Release the depth texture.
     */
    void releaseTexture();

    /**
     * Convert DepthFormat to SDL_GPUTextureFormat.
     * @param format Our depth format enum
     * @return Corresponding SDL texture format
     */
    static SDL_GPUTextureFormat toSDLFormat(DepthFormat format);

    SDL_GPUDevice* m_device = nullptr;  // Non-owning pointer
    SDL_GPUTexture* m_texture = nullptr;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    DepthFormat m_format = DepthFormat::D32_FLOAT;

    std::string m_lastError;
};

/**
 * Convert DepthFormat enum to string.
 * @param format Depth format enum value
 * @return Human-readable string representation
 */
const char* getDepthFormatName(DepthFormat format);

} // namespace sims3000

#endif // SIMS3000_RENDER_DEPTH_BUFFER_H
