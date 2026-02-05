#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cstdint>

namespace poc1 {

/**
 * Per-instance data for GPU instanced rendering.
 * This struct is uploaded to the GPU storage buffer.
 */
struct InstanceData {
    glm::mat4 modelMatrix;  // 64 bytes - transformation matrix
    glm::vec4 color;        // 16 bytes - RGBA color
    // Total: 80 bytes per instance
};

/**
 * Manages a GPU storage buffer for instance data.
 * Used for instanced rendering to batch similar draw calls.
 */
class InstanceBuffer {
public:
    /**
     * Creates an instance buffer with capacity for maxInstances.
     * @param device The SDL GPU device
     * @param maxInstances Maximum number of instances this buffer can hold
     */
    InstanceBuffer(SDL_GPUDevice* device, size_t maxInstances);

    /**
     * Destructor - releases GPU resources.
     */
    ~InstanceBuffer();

    // Non-copyable
    InstanceBuffer(const InstanceBuffer&) = delete;
    InstanceBuffer& operator=(const InstanceBuffer&) = delete;

    // Movable
    InstanceBuffer(InstanceBuffer&& other) noexcept;
    InstanceBuffer& operator=(InstanceBuffer&& other) noexcept;

    /**
     * Updates the instance buffer with new data.
     * @param commandBuffer The command buffer for GPU operations
     * @param instances Vector of instance data to upload
     * @return true if upload succeeded, false otherwise
     */
    bool Update(SDL_GPUCommandBuffer* commandBuffer, const std::vector<InstanceData>& instances);

    /**
     * Gets the underlying GPU buffer.
     * @return The SDL GPU buffer, or nullptr if not initialized
     */
    SDL_GPUBuffer* GetBuffer() const { return m_buffer; }

    /**
     * Gets the current number of instances in the buffer.
     * @return The number of active instances
     */
    uint32_t GetInstanceCount() const { return m_instanceCount; }

    /**
     * Gets the maximum capacity of this buffer.
     * @return Maximum number of instances
     */
    size_t GetMaxInstances() const { return m_maxInstances; }

private:
    SDL_GPUDevice* m_device = nullptr;
    SDL_GPUBuffer* m_buffer = nullptr;
    SDL_GPUTransferBuffer* m_transferBuffer = nullptr;
    size_t m_maxInstances = 0;
    uint32_t m_instanceCount = 0;
    size_t m_bufferSize = 0;

    void Release();
};

} // namespace poc1
