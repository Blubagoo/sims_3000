#include "instance_buffer.h"
#include <cstring>

namespace poc1 {

InstanceBuffer::InstanceBuffer(SDL_GPUDevice* device, size_t maxInstances)
    : m_device(device)
    , m_maxInstances(maxInstances)
    , m_bufferSize(maxInstances * sizeof(InstanceData))
{
    if (!m_device || m_maxInstances == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "InstanceBuffer: Invalid device or maxInstances");
        return;
    }

    // Create the GPU storage buffer for reading in shaders
    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    bufferInfo.size = static_cast<Uint32>(m_bufferSize);
    bufferInfo.props = 0;

    m_buffer = SDL_CreateGPUBuffer(m_device, &bufferInfo);
    if (!m_buffer) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "InstanceBuffer: Failed to create GPU buffer: %s", SDL_GetError());
        return;
    }

    // Create the transfer buffer for CPU -> GPU uploads
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<Uint32>(m_bufferSize);
    transferInfo.props = 0;

    m_transferBuffer = SDL_CreateGPUTransferBuffer(m_device, &transferInfo);
    if (!m_transferBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "InstanceBuffer: Failed to create transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUBuffer(m_device, m_buffer);
        m_buffer = nullptr;
        return;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "InstanceBuffer: Created buffer for %zu instances (%zu bytes)",
                m_maxInstances, m_bufferSize);
}

InstanceBuffer::~InstanceBuffer()
{
    Release();
}

InstanceBuffer::InstanceBuffer(InstanceBuffer&& other) noexcept
    : m_device(other.m_device)
    , m_buffer(other.m_buffer)
    , m_transferBuffer(other.m_transferBuffer)
    , m_maxInstances(other.m_maxInstances)
    , m_instanceCount(other.m_instanceCount)
    , m_bufferSize(other.m_bufferSize)
{
    other.m_device = nullptr;
    other.m_buffer = nullptr;
    other.m_transferBuffer = nullptr;
    other.m_maxInstances = 0;
    other.m_instanceCount = 0;
    other.m_bufferSize = 0;
}

InstanceBuffer& InstanceBuffer::operator=(InstanceBuffer&& other) noexcept
{
    if (this != &other) {
        Release();

        m_device = other.m_device;
        m_buffer = other.m_buffer;
        m_transferBuffer = other.m_transferBuffer;
        m_maxInstances = other.m_maxInstances;
        m_instanceCount = other.m_instanceCount;
        m_bufferSize = other.m_bufferSize;

        other.m_device = nullptr;
        other.m_buffer = nullptr;
        other.m_transferBuffer = nullptr;
        other.m_maxInstances = 0;
        other.m_instanceCount = 0;
        other.m_bufferSize = 0;
    }
    return *this;
}

bool InstanceBuffer::Update(SDL_GPUCommandBuffer* commandBuffer, const std::vector<InstanceData>& instances)
{
    if (!m_buffer || !m_transferBuffer || !commandBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "InstanceBuffer::Update: Invalid state or null command buffer");
        return false;
    }

    if (instances.empty()) {
        m_instanceCount = 0;
        return true;
    }

    if (instances.size() > m_maxInstances) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "InstanceBuffer::Update: Too many instances (%zu > %zu)",
                     instances.size(), m_maxInstances);
        return false;
    }

    // Map the transfer buffer
    void* mappedData = SDL_MapGPUTransferBuffer(m_device, m_transferBuffer, false);
    if (!mappedData) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "InstanceBuffer::Update: Failed to map transfer buffer: %s", SDL_GetError());
        return false;
    }

    // Copy instance data to the transfer buffer
    size_t dataSize = instances.size() * sizeof(InstanceData);
    std::memcpy(mappedData, instances.data(), dataSize);

    // Unmap the transfer buffer
    SDL_UnmapGPUTransferBuffer(m_device, m_transferBuffer);

    // Create a copy pass to upload data to the GPU buffer
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (!copyPass) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "InstanceBuffer::Update: Failed to begin copy pass: %s", SDL_GetError());
        return false;
    }

    // Set up the transfer source
    SDL_GPUTransferBufferLocation source = {};
    source.transfer_buffer = m_transferBuffer;
    source.offset = 0;

    // Set up the destination
    SDL_GPUBufferRegion destination = {};
    destination.buffer = m_buffer;
    destination.offset = 0;
    destination.size = static_cast<Uint32>(dataSize);

    // Perform the upload
    SDL_UploadToGPUBuffer(copyPass, &source, &destination, false);

    SDL_EndGPUCopyPass(copyPass);

    m_instanceCount = static_cast<uint32_t>(instances.size());

    return true;
}

void InstanceBuffer::Release()
{
    if (m_device) {
        if (m_transferBuffer) {
            SDL_ReleaseGPUTransferBuffer(m_device, m_transferBuffer);
            m_transferBuffer = nullptr;
        }
        if (m_buffer) {
            SDL_ReleaseGPUBuffer(m_device, m_buffer);
            m_buffer = nullptr;
        }
    }
    m_device = nullptr;
    m_instanceCount = 0;
}

} // namespace poc1
