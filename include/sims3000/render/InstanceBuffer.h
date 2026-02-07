/**
 * @file InstanceBuffer.h
 * @brief GPU instance buffer management for instanced rendering.
 *
 * Provides efficient management of per-instance data for rendering many
 * instances of the same model with a single draw call. Instance data includes:
 * - Per-instance model matrix (transform)
 * - Per-instance tint color
 * - Per-instance emissive intensity/color for powered/unpowered state
 *
 * Supports chunked instancing for large instance counts (512x512 maps with
 * up to 262k tiles). Chunks are sized to fit within GPU limits and enable
 * efficient frustum culling.
 *
 * Resource ownership:
 * - InstanceBuffer owns the SDL_GPUBuffer for instance data
 * - Caller must ensure GPUDevice outlives InstanceBuffer
 * - Call destroy() or destructor to release GPU resources
 *
 * Usage:
 * @code
 *   InstanceBuffer buffer(device);
 *   buffer.create(10000);  // Capacity for 10k instances
 *
 *   // Each frame:
 *   buffer.begin();
 *   for (const auto& entity : visibleEntities) {
 *       InstanceData data = createInstanceData(...);
 *       buffer.add(data);
 *   }
 *   buffer.end(cmdBuffer);  // Upload to GPU
 *
 *   // Bind and draw
 *   buffer.bind(renderPass, 0);  // Bind to storage buffer slot 0
 *   SDL_DrawGPUIndexedPrimitives(renderPass, mesh.indexCount, buffer.getInstanceCount(), ...);
 * @endcode
 */

#ifndef SIMS3000_RENDER_INSTANCE_BUFFER_H
#define SIMS3000_RENDER_INSTANCE_BUFFER_H

#include "sims3000/render/ToonShader.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <cstdint>
#include <limits>

namespace sims3000 {

// Forward declarations
class GPUDevice;

/**
 * @struct InstanceBufferStats
 * @brief Statistics about instance buffer usage.
 */
struct InstanceBufferStats {
    std::uint32_t instanceCount = 0;       ///< Current number of instances
    std::uint32_t capacity = 0;            ///< Maximum capacity
    std::uint32_t bytesUsed = 0;           ///< Current bytes used
    std::uint32_t bytesCapacity = 0;       ///< Total capacity in bytes
    std::uint32_t uploadCount = 0;         ///< Number of uploads this frame
    std::uint32_t chunkCount = 0;          ///< Number of chunks (for large buffers)
};

/**
 * @struct InstanceChunk
 * @brief A chunk of instances for chunked instancing.
 *
 * Large maps (512x512) can have up to 262k tiles. Chunked instancing
 * divides instances into manageable chunks for:
 * - Better frustum culling granularity
 * - Staying within GPU buffer size limits
 * - Reduced upload overhead per frame
 */
struct InstanceChunk {
    std::uint32_t startIndex = 0;          ///< Start index in the instance array
    std::uint32_t count = 0;               ///< Number of instances in this chunk
    glm::vec3 boundsMin{0.0f};             ///< AABB minimum for frustum culling
    glm::vec3 boundsMax{0.0f};             ///< AABB maximum for frustum culling
    bool visible = true;                    ///< Visibility flag after frustum culling
};

/**
 * @class InstanceBuffer
 * @brief GPU buffer for per-instance rendering data.
 *
 * Manages a GPU storage buffer containing ToonInstanceData for each instance.
 * Supports dynamic resizing and efficient per-frame updates.
 */
class InstanceBuffer {
public:
    /// Default chunk size for chunked instancing (4096 instances per chunk)
    /// Chosen to balance frustum culling granularity with overhead
    static constexpr std::uint32_t DEFAULT_CHUNK_SIZE = 4096;

    /// Maximum instances per buffer (limited by practical GPU memory)
    /// 262144 = 512x512 map tiles
    static constexpr std::uint32_t MAX_INSTANCES = 262144;

    /**
     * Create an instance buffer.
     * @param device GPU device for buffer creation
     */
    explicit InstanceBuffer(GPUDevice& device);

    ~InstanceBuffer();

    // Non-copyable
    InstanceBuffer(const InstanceBuffer&) = delete;
    InstanceBuffer& operator=(const InstanceBuffer&) = delete;

    // Movable
    InstanceBuffer(InstanceBuffer&& other) noexcept;
    InstanceBuffer& operator=(InstanceBuffer&& other) noexcept;

    /**
     * Create the GPU buffer with specified capacity.
     *
     * @param capacity Maximum number of instances
     * @param enableChunking Enable chunked instancing for large maps
     * @param chunkSize Instances per chunk (if chunking enabled)
     * @return true if buffer created successfully
     */
    bool create(
        std::uint32_t capacity,
        bool enableChunking = false,
        std::uint32_t chunkSize = DEFAULT_CHUNK_SIZE);

    /**
     * Release GPU resources.
     */
    void destroy();

    /**
     * Check if buffer is valid and ready for use.
     * @return true if buffer is created
     */
    bool isValid() const { return m_buffer != nullptr; }

    // =========================================================================
    // Instance Building API
    // =========================================================================

    /**
     * Begin building instances for this frame.
     * Clears the staging buffer.
     */
    void begin();

    /**
     * Add an instance to the staging buffer.
     *
     * @param data Instance data (model matrix, colors, etc.)
     * @return Instance index, or UINT32_MAX if buffer is full
     */
    std::uint32_t add(const ToonInstanceData& data);

    /**
     * Add an instance with individual parameters (convenience method).
     *
     * @param modelMatrix Model-to-world transform
     * @param tintColor Per-instance tint color (RGBA)
     * @param emissiveColor Emissive color (RGB) + intensity (A)
     * @param ambientOverride Per-instance ambient (0 = use global)
     * @return Instance index, or UINT32_MAX if buffer is full
     */
    std::uint32_t add(
        const glm::mat4& modelMatrix,
        const glm::vec4& tintColor = glm::vec4(1.0f),
        const glm::vec4& emissiveColor = glm::vec4(0.0f),
        float ambientOverride = 0.0f);

    /**
     * Finish building and upload instances to GPU.
     *
     * @param cmdBuffer Command buffer for upload commands
     * @return true if upload succeeded
     */
    bool end(SDL_GPUCommandBuffer* cmdBuffer);

    /**
     * Get the current number of instances.
     * @return Number of instances added since begin()
     */
    std::uint32_t getInstanceCount() const { return static_cast<std::uint32_t>(m_stagingData.size()); }

    /**
     * Get the buffer capacity.
     * @return Maximum number of instances
     */
    std::uint32_t getCapacity() const { return m_capacity; }

    // =========================================================================
    // Binding API
    // =========================================================================

    /**
     * Bind the instance buffer to a render pass.
     *
     * @param renderPass Active render pass
     * @param slot Vertex storage buffer slot (default 0)
     */
    void bind(SDL_GPURenderPass* renderPass, std::uint32_t slot = 0) const;

    /**
     * Get the raw GPU buffer handle.
     * @return SDL_GPUBuffer handle
     */
    SDL_GPUBuffer* getBuffer() const { return m_buffer; }

    // =========================================================================
    // Chunked Instancing API
    // =========================================================================

    /**
     * Check if chunked instancing is enabled.
     * @return true if buffer uses chunked instancing
     */
    bool isChunked() const { return m_enableChunking; }

    /**
     * Get the number of chunks.
     * @return Number of instance chunks
     */
    std::uint32_t getChunkCount() const { return static_cast<std::uint32_t>(m_chunks.size()); }

    /**
     * Get a chunk by index.
     * @param index Chunk index
     * @return Chunk data, or empty chunk if index is invalid
     */
    const InstanceChunk& getChunk(std::uint32_t index) const;

    /**
     * Update chunk visibility based on frustum culling.
     *
     * @param frustumPlanes Array of 6 frustum planes
     */
    void updateChunkVisibility(const glm::vec4 frustumPlanes[6]);

    /**
     * Get visible chunks (after frustum culling).
     * @return Vector of indices for visible chunks
     */
    std::vector<std::uint32_t> getVisibleChunks() const;

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * Get buffer statistics.
     * @return Current buffer statistics
     */
    InstanceBufferStats getStats() const;

    /**
     * Get the last error message.
     * @return Error string from last failed operation
     */
    const std::string& getLastError() const { return m_lastError; }

private:
    /**
     * Rebuild chunk bounds from instance data.
     */
    void rebuildChunks();

    /**
     * Check if an AABB is visible in the frustum.
     *
     * @param boundsMin AABB minimum
     * @param boundsMax AABB maximum
     * @param frustumPlanes Array of 6 frustum planes
     * @return true if AABB intersects or is inside the frustum
     */
    static bool isAABBVisible(
        const glm::vec3& boundsMin,
        const glm::vec3& boundsMax,
        const glm::vec4 frustumPlanes[6]);

    GPUDevice* m_device = nullptr;
    SDL_GPUBuffer* m_buffer = nullptr;
    SDL_GPUTransferBuffer* m_transferBuffer = nullptr;

    std::vector<ToonInstanceData> m_stagingData;
    std::uint32_t m_capacity = 0;

    // Chunking
    bool m_enableChunking = false;
    std::uint32_t m_chunkSize = DEFAULT_CHUNK_SIZE;
    std::vector<InstanceChunk> m_chunks;
    static const InstanceChunk s_emptyChunk;

    // Statistics
    std::uint32_t m_uploadCount = 0;

    std::string m_lastError;
};

/**
 * @class InstanceBufferPool
 * @brief Pool of instance buffers for different model types.
 *
 * Manages multiple instance buffers, one per unique model/mesh combination.
 * Enables efficient batching of instances by model type.
 */
class InstanceBufferPool {
public:
    /**
     * Create an instance buffer pool.
     * @param device GPU device for buffer creation
     */
    explicit InstanceBufferPool(GPUDevice& device);

    ~InstanceBufferPool();

    // Non-copyable, non-movable
    InstanceBufferPool(const InstanceBufferPool&) = delete;
    InstanceBufferPool& operator=(const InstanceBufferPool&) = delete;

    /**
     * Get or create an instance buffer for a model.
     *
     * @param modelId Unique identifier for the model/mesh
     * @param initialCapacity Initial capacity if creating new buffer
     * @return Pointer to instance buffer, or nullptr on failure
     */
    InstanceBuffer* getOrCreate(
        std::uint64_t modelId,
        std::uint32_t initialCapacity = 1024);

    /**
     * Begin a new frame, resetting all instance buffers.
     */
    void beginFrame();

    /**
     * End frame and upload all instance buffers.
     *
     * @param cmdBuffer Command buffer for upload commands
     * @return true if all uploads succeeded
     */
    bool endFrame(SDL_GPUCommandBuffer* cmdBuffer);

    /**
     * Get total instance count across all buffers.
     * @return Total instances
     */
    std::uint32_t getTotalInstanceCount() const;

    /**
     * Get number of active buffers.
     * @return Number of buffers with instances
     */
    std::uint32_t getActiveBufferCount() const;

    /**
     * Release all buffers.
     */
    void releaseAll();

private:
    GPUDevice* m_device = nullptr;
    std::unordered_map<std::uint64_t, std::unique_ptr<InstanceBuffer>> m_buffers;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_INSTANCE_BUFFER_H
