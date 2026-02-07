/**
 * @file InstanceBuffer.cpp
 * @brief Implementation of GPU instance buffer management.
 */

#include "sims3000/render/InstanceBuffer.h"
#include "sims3000/render/GPUDevice.h"

#include <SDL3/SDL_log.h>
#include <algorithm>
#include <cstring>
#include <limits>

namespace sims3000 {

// Static empty chunk for invalid index returns
const InstanceChunk InstanceBuffer::s_emptyChunk{};

// =============================================================================
// InstanceBuffer Implementation
// =============================================================================

InstanceBuffer::InstanceBuffer(GPUDevice& device)
    : m_device(&device)
{
}

InstanceBuffer::~InstanceBuffer() {
    destroy();
}

InstanceBuffer::InstanceBuffer(InstanceBuffer&& other) noexcept
    : m_device(other.m_device)
    , m_buffer(other.m_buffer)
    , m_transferBuffer(other.m_transferBuffer)
    , m_stagingData(std::move(other.m_stagingData))
    , m_capacity(other.m_capacity)
    , m_enableChunking(other.m_enableChunking)
    , m_chunkSize(other.m_chunkSize)
    , m_chunks(std::move(other.m_chunks))
    , m_uploadCount(other.m_uploadCount)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_buffer = nullptr;
    other.m_transferBuffer = nullptr;
    other.m_capacity = 0;
}

InstanceBuffer& InstanceBuffer::operator=(InstanceBuffer&& other) noexcept {
    if (this != &other) {
        destroy();

        m_device = other.m_device;
        m_buffer = other.m_buffer;
        m_transferBuffer = other.m_transferBuffer;
        m_stagingData = std::move(other.m_stagingData);
        m_capacity = other.m_capacity;
        m_enableChunking = other.m_enableChunking;
        m_chunkSize = other.m_chunkSize;
        m_chunks = std::move(other.m_chunks);
        m_uploadCount = other.m_uploadCount;
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
        other.m_buffer = nullptr;
        other.m_transferBuffer = nullptr;
        other.m_capacity = 0;
    }
    return *this;
}

bool InstanceBuffer::create(
    std::uint32_t capacity,
    bool enableChunking,
    std::uint32_t chunkSize)
{
    if (!m_device || !m_device->isValid()) {
        m_lastError = "InstanceBuffer::create: invalid GPU device";
        return false;
    }

    if (capacity == 0) {
        m_lastError = "InstanceBuffer::create: capacity cannot be zero";
        return false;
    }

    if (capacity > MAX_INSTANCES) {
        m_lastError = "InstanceBuffer::create: capacity exceeds maximum ("
            + std::to_string(MAX_INSTANCES) + ")";
        return false;
    }

    // Destroy existing buffer if any
    destroy();

    m_capacity = capacity;
    m_enableChunking = enableChunking;
    m_chunkSize = chunkSize > 0 ? chunkSize : DEFAULT_CHUNK_SIZE;

    // Calculate buffer size
    std::uint32_t bufferSize = capacity * sizeof(ToonInstanceData);

    // Create GPU storage buffer
    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    bufferInfo.size = bufferSize;

    m_buffer = SDL_CreateGPUBuffer(m_device->getHandle(), &bufferInfo);
    if (!m_buffer) {
        m_lastError = "InstanceBuffer::create: failed to create GPU buffer - ";
        m_lastError += SDL_GetError();
        return false;
    }

    // Create transfer buffer for uploads
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = bufferSize;

    m_transferBuffer = SDL_CreateGPUTransferBuffer(m_device->getHandle(), &transferInfo);
    if (!m_transferBuffer) {
        m_lastError = "InstanceBuffer::create: failed to create transfer buffer - ";
        m_lastError += SDL_GetError();
        SDL_ReleaseGPUBuffer(m_device->getHandle(), m_buffer);
        m_buffer = nullptr;
        return false;
    }

    // Reserve staging buffer capacity
    m_stagingData.reserve(capacity);

    // Initialize chunks if chunking is enabled
    if (m_enableChunking) {
        std::uint32_t chunkCount = (capacity + m_chunkSize - 1) / m_chunkSize;
        m_chunks.resize(chunkCount);
        for (std::uint32_t i = 0; i < chunkCount; ++i) {
            m_chunks[i].startIndex = i * m_chunkSize;
            m_chunks[i].count = 0;
            m_chunks[i].visible = true;
        }
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_RENDER,
        "InstanceBuffer: created with capacity %u (%u bytes), chunking=%s, chunkSize=%u",
        capacity, bufferSize, enableChunking ? "yes" : "no", m_chunkSize);

    return true;
}

void InstanceBuffer::destroy() {
    if (m_device && m_device->isValid()) {
        if (m_transferBuffer) {
            SDL_ReleaseGPUTransferBuffer(m_device->getHandle(), m_transferBuffer);
            m_transferBuffer = nullptr;
        }
        if (m_buffer) {
            SDL_ReleaseGPUBuffer(m_device->getHandle(), m_buffer);
            m_buffer = nullptr;
        }
    }

    m_stagingData.clear();
    m_chunks.clear();
    m_capacity = 0;
    m_uploadCount = 0;
}

void InstanceBuffer::begin() {
    m_stagingData.clear();
    m_uploadCount = 0;

    // Reset chunk counts
    for (auto& chunk : m_chunks) {
        chunk.count = 0;
        chunk.boundsMin = glm::vec3(std::numeric_limits<float>::max());
        chunk.boundsMax = glm::vec3(std::numeric_limits<float>::lowest());
        chunk.visible = true;
    }
}

std::uint32_t InstanceBuffer::add(const ToonInstanceData& data) {
    if (m_stagingData.size() >= m_capacity) {
        // Buffer is full - do not add
        return UINT32_MAX;
    }

    std::uint32_t index = static_cast<std::uint32_t>(m_stagingData.size());
    m_stagingData.push_back(data);

    // Update chunk bounds if chunking is enabled
    if (m_enableChunking && !m_chunks.empty()) {
        std::uint32_t chunkIndex = index / m_chunkSize;
        if (chunkIndex < m_chunks.size()) {
            InstanceChunk& chunk = m_chunks[chunkIndex];
            chunk.count++;

            // Extract position from model matrix (column 3, first 3 elements)
            glm::vec3 position(
                data.model[3][0],
                data.model[3][1],
                data.model[3][2]
            );

            // Expand chunk bounds
            chunk.boundsMin = glm::min(chunk.boundsMin, position);
            chunk.boundsMax = glm::max(chunk.boundsMax, position);
        }
    }

    return index;
}

std::uint32_t InstanceBuffer::add(
    const glm::mat4& modelMatrix,
    const glm::vec4& tintColor,
    const glm::vec4& emissiveColor,
    float ambientOverride)
{
    ToonInstanceData data;
    data.model = modelMatrix;
    data.baseColor = tintColor;
    data.emissiveColor = emissiveColor;
    data.ambientStrength = ambientOverride;
    return add(data);
}

bool InstanceBuffer::end(SDL_GPUCommandBuffer* cmdBuffer) {
    if (!cmdBuffer) {
        m_lastError = "InstanceBuffer::end: command buffer is null";
        return false;
    }

    if (!m_buffer || !m_transferBuffer) {
        m_lastError = "InstanceBuffer::end: buffer not created";
        return false;
    }

    if (m_stagingData.empty()) {
        // Nothing to upload - this is not an error
        return true;
    }

    // Map the transfer buffer
    void* mappedPtr = SDL_MapGPUTransferBuffer(
        m_device->getHandle(),
        m_transferBuffer,
        false  // Not cycling (we reset each frame)
    );

    if (!mappedPtr) {
        m_lastError = "InstanceBuffer::end: failed to map transfer buffer - ";
        m_lastError += SDL_GetError();
        return false;
    }

    // Copy staging data to transfer buffer
    std::size_t dataSize = m_stagingData.size() * sizeof(ToonInstanceData);
    std::memcpy(mappedPtr, m_stagingData.data(), dataSize);

    // Unmap the transfer buffer
    SDL_UnmapGPUTransferBuffer(m_device->getHandle(), m_transferBuffer);

    // Create copy pass
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuffer);
    if (!copyPass) {
        m_lastError = "InstanceBuffer::end: failed to begin copy pass - ";
        m_lastError += SDL_GetError();
        return false;
    }

    // Copy from transfer buffer to GPU buffer
    SDL_GPUTransferBufferLocation src = {};
    src.transfer_buffer = m_transferBuffer;
    src.offset = 0;

    SDL_GPUBufferRegion dst = {};
    dst.buffer = m_buffer;
    dst.offset = 0;
    dst.size = static_cast<std::uint32_t>(dataSize);

    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);

    SDL_EndGPUCopyPass(copyPass);

    m_uploadCount++;
    return true;
}

void InstanceBuffer::bind(SDL_GPURenderPass* renderPass, std::uint32_t slot) const {
    if (!renderPass || !m_buffer) {
        return;
    }

    SDL_GPUBuffer* buffers[] = { m_buffer };
    SDL_BindGPUVertexStorageBuffers(renderPass, slot, buffers, 1);
}

const InstanceChunk& InstanceBuffer::getChunk(std::uint32_t index) const {
    if (index < m_chunks.size()) {
        return m_chunks[index];
    }
    return s_emptyChunk;
}

void InstanceBuffer::updateChunkVisibility(const glm::vec4 frustumPlanes[6]) {
    if (!m_enableChunking || !frustumPlanes) {
        return;
    }

    for (auto& chunk : m_chunks) {
        if (chunk.count == 0) {
            chunk.visible = false;
            continue;
        }

        chunk.visible = isAABBVisible(chunk.boundsMin, chunk.boundsMax, frustumPlanes);
    }
}

std::vector<std::uint32_t> InstanceBuffer::getVisibleChunks() const {
    std::vector<std::uint32_t> result;
    result.reserve(m_chunks.size());

    for (std::uint32_t i = 0; i < m_chunks.size(); ++i) {
        if (m_chunks[i].visible && m_chunks[i].count > 0) {
            result.push_back(i);
        }
    }

    return result;
}

InstanceBufferStats InstanceBuffer::getStats() const {
    InstanceBufferStats stats;
    stats.instanceCount = static_cast<std::uint32_t>(m_stagingData.size());
    stats.capacity = m_capacity;
    stats.bytesUsed = stats.instanceCount * sizeof(ToonInstanceData);
    stats.bytesCapacity = m_capacity * sizeof(ToonInstanceData);
    stats.uploadCount = m_uploadCount;
    stats.chunkCount = static_cast<std::uint32_t>(m_chunks.size());
    return stats;
}

void InstanceBuffer::rebuildChunks() {
    if (!m_enableChunking || m_stagingData.empty()) {
        return;
    }

    // Reset all chunks
    for (auto& chunk : m_chunks) {
        chunk.count = 0;
        chunk.boundsMin = glm::vec3(std::numeric_limits<float>::max());
        chunk.boundsMax = glm::vec3(std::numeric_limits<float>::lowest());
    }

    // Rebuild chunk data from staging data
    for (std::uint32_t i = 0; i < m_stagingData.size(); ++i) {
        std::uint32_t chunkIndex = i / m_chunkSize;
        if (chunkIndex >= m_chunks.size()) {
            break;
        }

        InstanceChunk& chunk = m_chunks[chunkIndex];
        chunk.count++;

        glm::vec3 position(
            m_stagingData[i].model[3][0],
            m_stagingData[i].model[3][1],
            m_stagingData[i].model[3][2]
        );

        chunk.boundsMin = glm::min(chunk.boundsMin, position);
        chunk.boundsMax = glm::max(chunk.boundsMax, position);
    }
}

bool InstanceBuffer::isAABBVisible(
    const glm::vec3& boundsMin,
    const glm::vec3& boundsMax,
    const glm::vec4 frustumPlanes[6])
{
    // Test AABB against all 6 frustum planes
    for (int i = 0; i < 6; ++i) {
        const glm::vec4& plane = frustumPlanes[i];

        // Find the positive vertex (farthest along the plane normal)
        glm::vec3 positiveVertex;
        positiveVertex.x = (plane.x >= 0.0f) ? boundsMax.x : boundsMin.x;
        positiveVertex.y = (plane.y >= 0.0f) ? boundsMax.y : boundsMin.y;
        positiveVertex.z = (plane.z >= 0.0f) ? boundsMax.z : boundsMin.z;

        // If the positive vertex is on the negative side of the plane,
        // the AABB is completely outside the frustum
        float distance = glm::dot(glm::vec3(plane), positiveVertex) + plane.w;
        if (distance < 0.0f) {
            return false;
        }
    }

    return true;
}

// =============================================================================
// InstanceBufferPool Implementation
// =============================================================================

InstanceBufferPool::InstanceBufferPool(GPUDevice& device)
    : m_device(&device)
{
}

InstanceBufferPool::~InstanceBufferPool() {
    releaseAll();
}

InstanceBuffer* InstanceBufferPool::getOrCreate(
    std::uint64_t modelId,
    std::uint32_t initialCapacity)
{
    auto it = m_buffers.find(modelId);
    if (it != m_buffers.end()) {
        return it->second.get();
    }

    // Create new buffer
    auto buffer = std::make_unique<InstanceBuffer>(*m_device);
    if (!buffer->create(initialCapacity)) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER,
            "InstanceBufferPool: failed to create buffer for model %llu: %s",
            static_cast<unsigned long long>(modelId),
            buffer->getLastError().c_str());
        return nullptr;
    }

    InstanceBuffer* ptr = buffer.get();
    m_buffers[modelId] = std::move(buffer);
    return ptr;
}

void InstanceBufferPool::beginFrame() {
    for (auto& pair : m_buffers) {
        pair.second->begin();
    }
}

bool InstanceBufferPool::endFrame(SDL_GPUCommandBuffer* cmdBuffer) {
    bool allSucceeded = true;
    for (auto& pair : m_buffers) {
        if (pair.second->getInstanceCount() > 0) {
            if (!pair.second->end(cmdBuffer)) {
                allSucceeded = false;
            }
        }
    }
    return allSucceeded;
}

std::uint32_t InstanceBufferPool::getTotalInstanceCount() const {
    std::uint32_t total = 0;
    for (const auto& pair : m_buffers) {
        total += pair.second->getInstanceCount();
    }
    return total;
}

std::uint32_t InstanceBufferPool::getActiveBufferCount() const {
    std::uint32_t count = 0;
    for (const auto& pair : m_buffers) {
        if (pair.second->getInstanceCount() > 0) {
            count++;
        }
    }
    return count;
}

void InstanceBufferPool::releaseAll() {
    for (auto& pair : m_buffers) {
        pair.second->destroy();
    }
    m_buffers.clear();
}

} // namespace sims3000
