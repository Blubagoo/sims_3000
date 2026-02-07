/**
 * @file UniformBufferPool.cpp
 * @brief UniformBufferPool implementation.
 */

#include "sims3000/render/UniformBufferPool.h"
#include <SDL3/SDL_log.h>
#include <utility>

namespace sims3000 {

UniformBufferPool::UniformBufferPool(SDL_GPUDevice* device, std::uint32_t blockSize)
    : m_device(device)
    , m_blockSize(blockSize)
{
    if (!device) {
        m_lastError = "Cannot create UniformBufferPool: device is null";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return;
    }

    // Pre-allocate first block
    if (!createBlock()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "UniformBufferPool: Failed to create initial block");
    }
}

UniformBufferPool::~UniformBufferPool() {
    releaseAll();
}

UniformBufferPool::UniformBufferPool(UniformBufferPool&& other) noexcept
    : m_device(other.m_device)
    , m_blockSize(other.m_blockSize)
    , m_blocks(std::move(other.m_blocks))
    , m_currentBlockIndex(other.m_currentBlockIndex)
    , m_frameAllocations(other.m_frameAllocations)
    , m_frameBytesAllocated(other.m_frameBytesAllocated)
    , m_peakBytesAllocated(other.m_peakBytesAllocated)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_blocks.clear();
    other.m_currentBlockIndex = 0;
}

UniformBufferPool& UniformBufferPool::operator=(UniformBufferPool&& other) noexcept {
    if (this != &other) {
        releaseAll();

        m_device = other.m_device;
        m_blockSize = other.m_blockSize;
        m_blocks = std::move(other.m_blocks);
        m_currentBlockIndex = other.m_currentBlockIndex;
        m_frameAllocations = other.m_frameAllocations;
        m_frameBytesAllocated = other.m_frameBytesAllocated;
        m_peakBytesAllocated = other.m_peakBytesAllocated;
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
        other.m_blocks.clear();
        other.m_currentBlockIndex = 0;
    }
    return *this;
}

UniformBufferAllocation UniformBufferPool::allocate(std::uint32_t size) {
    UniformBufferAllocation result;

    if (!m_device) {
        m_lastError = "Cannot allocate: pool has no device";
        return result;
    }

    if (size == 0) {
        m_lastError = "Cannot allocate: size is zero";
        return result;
    }

    // Align the size
    std::uint32_t alignedSize = alignUp(size);

    // Check if allocation is larger than a single block
    if (alignedSize > m_blockSize) {
        m_lastError = "Allocation size (" + std::to_string(alignedSize) +
                      ") exceeds block size (" + std::to_string(m_blockSize) + ")";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return result;
    }

    // Find a block with enough space
    while (m_currentBlockIndex < m_blocks.size()) {
        Block& block = m_blocks[m_currentBlockIndex];
        std::uint32_t alignedOffset = alignUp(block.currentOffset);

        if (alignedOffset + alignedSize <= block.capacity) {
            // Allocate from this block
            result.buffer = block.buffer;
            result.offset = alignedOffset;
            result.size = size;

            block.currentOffset = alignedOffset + alignedSize;

            m_frameAllocations++;
            m_frameBytesAllocated += alignedSize;

            return result;
        }

        // Try next block
        m_currentBlockIndex++;
    }

    // Need a new block
    if (!createBlock()) {
        return result;  // Failed to create block
    }

    // Allocate from new block
    Block& newBlock = m_blocks.back();
    result.buffer = newBlock.buffer;
    result.offset = 0;
    result.size = size;

    newBlock.currentOffset = alignedSize;
    m_currentBlockIndex = static_cast<std::uint32_t>(m_blocks.size() - 1);

    m_frameAllocations++;
    m_frameBytesAllocated += alignedSize;

    return result;
}

void UniformBufferPool::reset() {
    // Update peak tracking
    if (m_frameBytesAllocated > m_peakBytesAllocated) {
        m_peakBytesAllocated = m_frameBytesAllocated;
    }

    // Reset all block offsets
    for (auto& block : m_blocks) {
        block.currentOffset = 0;
    }

    // Start from first block
    m_currentBlockIndex = 0;

    // Reset frame stats
    m_frameAllocations = 0;
    m_frameBytesAllocated = 0;
}

void UniformBufferPool::releaseAll() {
    if (m_device) {
        for (auto& block : m_blocks) {
            if (block.buffer) {
                SDL_ReleaseGPUBuffer(m_device, block.buffer);
                block.buffer = nullptr;
            }
        }
    }

    m_blocks.clear();
    m_currentBlockIndex = 0;
    m_frameAllocations = 0;
    m_frameBytesAllocated = 0;
}

UniformBufferPoolStats UniformBufferPool::getStats() const {
    UniformBufferPoolStats stats;
    stats.allocationCount = m_frameAllocations;
    stats.totalBytesAllocated = m_frameBytesAllocated;
    stats.peakBytesAllocated = m_peakBytesAllocated;
    stats.blockCount = static_cast<std::uint32_t>(m_blocks.size());

    for (const auto& block : m_blocks) {
        stats.totalBytesCapacity += block.capacity;
    }

    return stats;
}

const std::string& UniformBufferPool::getLastError() const {
    return m_lastError;
}

bool UniformBufferPool::createBlock() {
    if (!m_device) {
        return false;
    }

    SDL_GPUBufferCreateInfo createInfo = {};
    // Use GRAPHICS_STORAGE_READ for shader-accessible uniform-like data
    // SDL_GPU doesn't have a dedicated uniform buffer usage flag;
    // uniform data is typically pushed via SDL_PushGPUVertexUniformData
    // or accessed through storage buffers
    createInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    createInfo.size = m_blockSize;

    SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(m_device, &createInfo);
    if (!buffer) {
        m_lastError = "Failed to create uniform buffer block: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    Block block;
    block.buffer = buffer;
    block.currentOffset = 0;
    block.capacity = m_blockSize;

    m_blocks.push_back(block);

    SDL_LogDebug(SDL_LOG_CATEGORY_GPU,
                 "UniformBufferPool: Created block %zu (size=%u)",
                 m_blocks.size(), m_blockSize);

    return true;
}

std::uint32_t UniformBufferPool::alignUp(std::uint32_t size) {
    return (size + UNIFORM_ALIGNMENT - 1) & ~(UNIFORM_ALIGNMENT - 1);
}

} // namespace sims3000
