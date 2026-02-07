/**
 * @file UniformBufferPool.h
 * @brief Efficient GPU uniform buffer allocation pool.
 *
 * Provides a pooled allocation strategy for uniform buffers to minimize
 * GPU memory fragmentation and reduce allocation overhead during rendering.
 *
 * Resource ownership:
 * - UniformBufferPool owns all SDL_GPUBuffer instances it creates
 * - Allocations are reset per-frame, not freed individually
 * - Call reset() at the start of each frame before allocating
 *
 * Thread safety:
 * - Not thread-safe. Call from render thread only.
 */

#ifndef SIMS3000_RENDER_UNIFORM_BUFFER_POOL_H
#define SIMS3000_RENDER_UNIFORM_BUFFER_POOL_H

#include <SDL3/SDL.h>
#include <cstdint>
#include <vector>
#include <string>

namespace sims3000 {

/**
 * @struct UniformBufferAllocation
 * @brief Handle to an allocation within the uniform buffer pool.
 */
struct UniformBufferAllocation {
    SDL_GPUBuffer* buffer = nullptr;  ///< The GPU buffer containing the allocation
    std::uint32_t offset = 0;         ///< Byte offset within the buffer
    std::uint32_t size = 0;           ///< Size of the allocation in bytes

    bool isValid() const { return buffer != nullptr; }
};

/**
 * @struct UniformBufferPoolStats
 * @brief Statistics about pool usage.
 */
struct UniformBufferPoolStats {
    std::uint32_t totalBytesAllocated = 0;   ///< Total bytes allocated this frame
    std::uint32_t totalBytesCapacity = 0;    ///< Total capacity across all blocks
    std::uint32_t blockCount = 0;            ///< Number of GPU buffer blocks
    std::uint32_t allocationCount = 0;       ///< Number of allocations this frame
    std::uint32_t peakBytesAllocated = 0;    ///< Peak allocation across all frames
};

/**
 * @class UniformBufferPool
 * @brief Pool allocator for GPU uniform buffers.
 *
 * Uses a block-based allocation strategy where each block is a large
 * GPU buffer. Allocations are packed sequentially within blocks.
 * When a block is full, a new block is created.
 *
 * Memory layout per block:
 * [Allocation 1][Alignment Padding][Allocation 2][Alignment Padding]...
 *
 * Usage:
 * @code
 *   UniformBufferPool pool(device);
 *
 *   // Each frame:
 *   pool.reset();  // Reuse all allocations
 *
 *   auto alloc = pool.allocate(sizeof(MyUniformData));
 *   if (alloc.isValid()) {
 *       // Write data via transfer buffer
 *   }
 * @endcode
 */
class UniformBufferPool {
public:
    /// Default block size (256KB - good balance between allocation efficiency and memory usage)
    static constexpr std::uint32_t DEFAULT_BLOCK_SIZE = 256 * 1024;

    /// Alignment for uniform buffer bindings (typically 256 bytes for D3D12/Vulkan)
    static constexpr std::uint32_t UNIFORM_ALIGNMENT = 256;

    /**
     * Create a uniform buffer pool.
     * @param device SDL GPU device for buffer creation
     * @param blockSize Size of each buffer block in bytes (default 256KB)
     */
    explicit UniformBufferPool(SDL_GPUDevice* device,
                               std::uint32_t blockSize = DEFAULT_BLOCK_SIZE);

    ~UniformBufferPool();

    // Non-copyable
    UniformBufferPool(const UniformBufferPool&) = delete;
    UniformBufferPool& operator=(const UniformBufferPool&) = delete;

    // Movable
    UniformBufferPool(UniformBufferPool&& other) noexcept;
    UniformBufferPool& operator=(UniformBufferPool&& other) noexcept;

    /**
     * Allocate uniform buffer memory.
     * Automatically aligns to UNIFORM_ALIGNMENT boundary.
     *
     * @param size Required size in bytes
     * @return Allocation handle, or invalid allocation if failed
     */
    UniformBufferAllocation allocate(std::uint32_t size);

    /**
     * Reset all allocations for a new frame.
     * Does not free GPU memory - just resets allocation pointers.
     * Call at the start of each frame before allocating.
     */
    void reset();

    /**
     * Release all GPU buffers.
     * Frees all GPU memory. Pool can still be used after this call.
     */
    void releaseAll();

    /**
     * Get current pool statistics.
     * @return Pool usage statistics
     */
    UniformBufferPoolStats getStats() const;

    /**
     * Get the last error message.
     * @return Error string from last failed operation
     */
    const std::string& getLastError() const;

    /**
     * Check if pool is valid (has a device).
     * @return true if pool can allocate
     */
    bool isValid() const { return m_device != nullptr; }

private:
    /**
     * @struct Block
     * @brief A single GPU buffer used for allocations.
     */
    struct Block {
        SDL_GPUBuffer* buffer = nullptr;
        std::uint32_t currentOffset = 0;
        std::uint32_t capacity = 0;
    };

    /**
     * Create a new buffer block.
     * @return true if block was created successfully
     */
    bool createBlock();

    /**
     * Align size up to UNIFORM_ALIGNMENT boundary.
     * @param size Size to align
     * @return Aligned size
     */
    static std::uint32_t alignUp(std::uint32_t size);

    SDL_GPUDevice* m_device = nullptr;
    std::uint32_t m_blockSize;
    std::vector<Block> m_blocks;
    std::uint32_t m_currentBlockIndex = 0;

    // Statistics
    std::uint32_t m_frameAllocations = 0;
    std::uint32_t m_frameBytesAllocated = 0;
    std::uint32_t m_peakBytesAllocated = 0;

    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_UNIFORM_BUFFER_POOL_H
