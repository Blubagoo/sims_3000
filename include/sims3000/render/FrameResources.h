/**
 * @file FrameResources.h
 * @brief Per-frame GPU resource management with double/triple buffering.
 *
 * Manages GPU resources that need to be per-frame to avoid CPU/GPU
 * synchronization stalls. Uses a ring buffer of frame resource sets
 * that rotate each frame.
 *
 * Resource ownership:
 * - FrameResources owns all per-frame GPU resources
 * - Each frame set contains its own transfer buffers
 * - Rotating between frames allows GPU to use resources from previous frames
 *   while CPU prepares the next frame
 *
 * Thread safety:
 * - Not thread-safe. Call from render thread only.
 */

#ifndef SIMS3000_RENDER_FRAME_RESOURCES_H
#define SIMS3000_RENDER_FRAME_RESOURCES_H

#include <SDL3/SDL.h>
#include <cstdint>
#include <vector>
#include <string>

namespace sims3000 {

// Forward declaration
class UniformBufferPool;

/**
 * @struct FrameResourceSet
 * @brief Resources belonging to a single frame in the ring buffer.
 */
struct FrameResourceSet {
    /// Transfer buffer for uploading uniform data
    SDL_GPUTransferBuffer* uniformTransfer = nullptr;

    /// Transfer buffer for uploading texture data
    SDL_GPUTransferBuffer* textureTransfer = nullptr;

    /// Current offset in uniform transfer buffer
    std::uint32_t uniformTransferOffset = 0;

    /// Current offset in texture transfer buffer
    std::uint32_t textureTransferOffset = 0;

    /// Frame number when this set was last used
    std::uint64_t lastUsedFrame = 0;

    /// Flag indicating if resources need to be released before reuse
    bool pendingRelease = false;
};

/**
 * @struct FrameResourcesConfig
 * @brief Configuration for frame resource allocation.
 */
struct FrameResourcesConfig {
    /// Number of frames to buffer (2 = double buffering, 3 = triple buffering)
    std::uint32_t frameCount = 2;

    /// Size of per-frame uniform transfer buffer (default 1MB)
    std::uint32_t uniformTransferSize = 1024 * 1024;

    /// Size of per-frame texture transfer buffer (default 4MB)
    std::uint32_t textureTransferSize = 4 * 1024 * 1024;
};

/**
 * @struct FrameResourcesStats
 * @brief Statistics about frame resource usage.
 */
struct FrameResourcesStats {
    std::uint32_t frameCount = 0;              ///< Number of frame sets
    std::uint32_t currentFrame = 0;            ///< Current frame index
    std::uint64_t totalFramesRendered = 0;     ///< Total frames since creation
    std::uint32_t uniformBytesUsed = 0;        ///< Uniform transfer bytes this frame
    std::uint32_t textureBytesUsed = 0;        ///< Texture transfer bytes this frame
    std::uint32_t uniformTransferCapacity = 0; ///< Uniform transfer buffer capacity
    std::uint32_t textureTransferCapacity = 0; ///< Texture transfer buffer capacity
};

/**
 * @class FrameResources
 * @brief Manages per-frame GPU resources with buffering.
 *
 * Implements double or triple buffering of GPU transfer resources
 * to allow pipelining of CPU and GPU work without synchronization stalls.
 *
 * The pattern is:
 * - Frame N: CPU writes to transfer buffer N, GPU reads from transfer buffer N-1
 * - Frame N+1: CPU writes to transfer buffer N+1, GPU reads from transfer buffer N
 * - etc.
 *
 * Usage:
 * @code
 *   FrameResourcesConfig config;
 *   config.frameCount = 2;  // Double buffering
 *   FrameResources frames(device, config);
 *
 *   // Each frame:
 *   frames.beginFrame();
 *
 *   // Allocate uniform data
 *   void* uniformPtr;
 *   std::uint32_t uniformOffset;
 *   if (frames.allocateUniformData(sizeof(UBO), &uniformPtr, &uniformOffset)) {
 *       memcpy(uniformPtr, &ubo, sizeof(UBO));
 *   }
 *
 *   frames.endFrame();
 * @endcode
 */
class FrameResources {
public:
    /**
     * Create frame resources with default configuration.
     * @param device SDL GPU device
     */
    explicit FrameResources(SDL_GPUDevice* device);

    /**
     * Create frame resources with custom configuration.
     * @param device SDL GPU device
     * @param config Frame resource configuration
     */
    FrameResources(SDL_GPUDevice* device, const FrameResourcesConfig& config);

    ~FrameResources();

    // Non-copyable
    FrameResources(const FrameResources&) = delete;
    FrameResources& operator=(const FrameResources&) = delete;

    // Movable
    FrameResources(FrameResources&& other) noexcept;
    FrameResources& operator=(FrameResources&& other) noexcept;

    /**
     * Begin a new frame.
     * Advances to next frame set and resets allocation offsets.
     * Must be called at the start of each frame before any allocations.
     */
    void beginFrame();

    /**
     * End the current frame.
     * Marks resources as in-use by GPU.
     * Call after submitting command buffer.
     */
    void endFrame();

    /**
     * Allocate space for uniform data in current frame's transfer buffer.
     *
     * @param size Size in bytes to allocate
     * @param outPtr Receives pointer to mapped memory for writing
     * @param outOffset Receives offset within the transfer buffer
     * @return true if allocation succeeded
     */
    bool allocateUniformData(std::uint32_t size, void** outPtr, std::uint32_t* outOffset);

    /**
     * Allocate space for texture data in current frame's transfer buffer.
     *
     * @param size Size in bytes to allocate
     * @param outPtr Receives pointer to mapped memory for writing
     * @param outOffset Receives offset within the transfer buffer
     * @return true if allocation succeeded
     */
    bool allocateTextureData(std::uint32_t size, void** outPtr, std::uint32_t* outOffset);

    /**
     * Get the current frame's uniform transfer buffer.
     * @return Transfer buffer handle
     */
    SDL_GPUTransferBuffer* getUniformTransferBuffer() const;

    /**
     * Get the current frame's texture transfer buffer.
     * @return Transfer buffer handle
     */
    SDL_GPUTransferBuffer* getTextureTransferBuffer() const;

    /**
     * Get the current frame index (0 to frameCount-1).
     * @return Current frame index
     */
    std::uint32_t getCurrentFrameIndex() const { return m_currentFrame; }

    /**
     * Get total frames rendered.
     * @return Frame count since creation
     */
    std::uint64_t getTotalFrames() const { return m_totalFrames; }

    /**
     * Get statistics about frame resource usage.
     * @return Usage statistics
     */
    FrameResourcesStats getStats() const;

    /**
     * Get the last error message.
     * @return Error string
     */
    const std::string& getLastError() const { return m_lastError; }

    /**
     * Check if frame resources are valid.
     * @return true if usable
     */
    bool isValid() const { return m_device != nullptr && !m_frameSets.empty(); }

    /**
     * Release all GPU resources.
     */
    void releaseAll();

private:
    /**
     * Initialize frame resource sets.
     * @return true if successful
     */
    bool initialize();

    /**
     * Create a single frame resource set.
     * @param set The set to populate
     * @return true if successful
     */
    bool createFrameSet(FrameResourceSet& set);

    /**
     * Destroy a single frame resource set.
     * @param set The set to destroy
     */
    void destroyFrameSet(FrameResourceSet& set);

    SDL_GPUDevice* m_device = nullptr;
    FrameResourcesConfig m_config;
    std::vector<FrameResourceSet> m_frameSets;
    std::uint32_t m_currentFrame = 0;
    std::uint64_t m_totalFrames = 0;
    bool m_inFrame = false;

    // Mapped pointers for current frame
    void* m_uniformMappedPtr = nullptr;
    void* m_textureMappedPtr = nullptr;

    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_FRAME_RESOURCES_H
