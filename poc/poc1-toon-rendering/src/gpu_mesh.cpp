#include "gpu_mesh.h"
#include "model_loader.h"
#include <SDL3/SDL.h>

GPUMesh::GPUMesh(SDL_GPUDevice* device, const MeshData& meshData)
    : m_device(device)
    , m_vertexCount(static_cast<uint32_t>(meshData.vertices.size()))
    , m_indexCount(static_cast<uint32_t>(meshData.indices.size()))
{
    if (!device) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GPUMesh: device is null");
        return;
    }

    if (meshData.vertices.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GPUMesh: no vertices in mesh data");
        return;
    }

    // Calculate buffer sizes
    const uint32_t vertexBufferSize = m_vertexCount * sizeof(Vertex);
    const uint32_t indexBufferSize = m_indexCount * sizeof(uint32_t);

    // Create vertex buffer
    SDL_GPUBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBufferInfo.size = vertexBufferSize;

    m_vertexBuffer = SDL_CreateGPUBuffer(device, &vertexBufferInfo);
    if (!m_vertexBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "GPUMesh: Failed to create vertex buffer: %s", SDL_GetError());
        return;
    }

    // Create index buffer if we have indices
    if (m_indexCount > 0) {
        SDL_GPUBufferCreateInfo indexBufferInfo = {};
        indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        indexBufferInfo.size = indexBufferSize;

        m_indexBuffer = SDL_CreateGPUBuffer(device, &indexBufferInfo);
        if (!m_indexBuffer) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "GPUMesh: Failed to create index buffer: %s", SDL_GetError());
            SDL_ReleaseGPUBuffer(device, m_vertexBuffer);
            m_vertexBuffer = nullptr;
            return;
        }
    }

    // Create transfer buffer for uploading data
    uint32_t transferSize = vertexBufferSize + indexBufferSize;
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = transferSize;

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transferBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "GPUMesh: Failed to create transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUBuffer(device, m_vertexBuffer);
        m_vertexBuffer = nullptr;
        if (m_indexBuffer) {
            SDL_ReleaseGPUBuffer(device, m_indexBuffer);
            m_indexBuffer = nullptr;
        }
        return;
    }

    // Map transfer buffer and copy data
    void* transferData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    if (!transferData) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "GPUMesh: Failed to map transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        SDL_ReleaseGPUBuffer(device, m_vertexBuffer);
        m_vertexBuffer = nullptr;
        if (m_indexBuffer) {
            SDL_ReleaseGPUBuffer(device, m_indexBuffer);
            m_indexBuffer = nullptr;
        }
        return;
    }

    // Copy vertex data
    SDL_memcpy(transferData, meshData.vertices.data(), vertexBufferSize);

    // Copy index data after vertex data
    if (m_indexCount > 0) {
        uint8_t* indexDest = static_cast<uint8_t*>(transferData) + vertexBufferSize;
        SDL_memcpy(indexDest, meshData.indices.data(), indexBufferSize);
    }

    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    // Acquire command buffer for copy operations
    SDL_GPUCommandBuffer* cmdBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!cmdBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "GPUMesh: Failed to acquire command buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        SDL_ReleaseGPUBuffer(device, m_vertexBuffer);
        m_vertexBuffer = nullptr;
        if (m_indexBuffer) {
            SDL_ReleaseGPUBuffer(device, m_indexBuffer);
            m_indexBuffer = nullptr;
        }
        return;
    }

    // Begin copy pass
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuffer);
    if (!copyPass) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "GPUMesh: Failed to begin copy pass: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(cmdBuffer);
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        SDL_ReleaseGPUBuffer(device, m_vertexBuffer);
        m_vertexBuffer = nullptr;
        if (m_indexBuffer) {
            SDL_ReleaseGPUBuffer(device, m_indexBuffer);
            m_indexBuffer = nullptr;
        }
        return;
    }

    // Upload vertex data
    SDL_GPUTransferBufferLocation vertexSrc = {};
    vertexSrc.transfer_buffer = transferBuffer;
    vertexSrc.offset = 0;

    SDL_GPUBufferRegion vertexDst = {};
    vertexDst.buffer = m_vertexBuffer;
    vertexDst.offset = 0;
    vertexDst.size = vertexBufferSize;

    SDL_UploadToGPUBuffer(copyPass, &vertexSrc, &vertexDst, false);

    // Upload index data if present
    if (m_indexCount > 0) {
        SDL_GPUTransferBufferLocation indexSrc = {};
        indexSrc.transfer_buffer = transferBuffer;
        indexSrc.offset = vertexBufferSize;

        SDL_GPUBufferRegion indexDst = {};
        indexDst.buffer = m_indexBuffer;
        indexDst.offset = 0;
        indexDst.size = indexBufferSize;

        SDL_UploadToGPUBuffer(copyPass, &indexSrc, &indexDst, false);
    }

    SDL_EndGPUCopyPass(copyPass);

    // Submit and wait for completion
    SDL_SubmitGPUCommandBuffer(cmdBuffer);

    // Release transfer buffer (no longer needed after submit)
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "GPUMesh: Created mesh with %u vertices and %u indices",
                m_vertexCount, m_indexCount);
}

GPUMesh::~GPUMesh() {
    if (m_device) {
        if (m_vertexBuffer) {
            SDL_ReleaseGPUBuffer(m_device, m_vertexBuffer);
            m_vertexBuffer = nullptr;
        }
        if (m_indexBuffer) {
            SDL_ReleaseGPUBuffer(m_device, m_indexBuffer);
            m_indexBuffer = nullptr;
        }
    }
}

GPUMesh::GPUMesh(GPUMesh&& other) noexcept
    : m_device(other.m_device)
    , m_vertexBuffer(other.m_vertexBuffer)
    , m_indexBuffer(other.m_indexBuffer)
    , m_vertexCount(other.m_vertexCount)
    , m_indexCount(other.m_indexCount)
{
    other.m_device = nullptr;
    other.m_vertexBuffer = nullptr;
    other.m_indexBuffer = nullptr;
    other.m_vertexCount = 0;
    other.m_indexCount = 0;
}

GPUMesh& GPUMesh::operator=(GPUMesh&& other) noexcept {
    if (this != &other) {
        // Release current resources
        if (m_device) {
            if (m_vertexBuffer) {
                SDL_ReleaseGPUBuffer(m_device, m_vertexBuffer);
            }
            if (m_indexBuffer) {
                SDL_ReleaseGPUBuffer(m_device, m_indexBuffer);
            }
        }

        // Move from other
        m_device = other.m_device;
        m_vertexBuffer = other.m_vertexBuffer;
        m_indexBuffer = other.m_indexBuffer;
        m_vertexCount = other.m_vertexCount;
        m_indexCount = other.m_indexCount;

        // Clear other
        other.m_device = nullptr;
        other.m_vertexBuffer = nullptr;
        other.m_indexBuffer = nullptr;
        other.m_vertexCount = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

SDL_GPUBuffer* GPUMesh::GetVertexBuffer() const {
    return m_vertexBuffer;
}

SDL_GPUBuffer* GPUMesh::GetIndexBuffer() const {
    return m_indexBuffer;
}

uint32_t GPUMesh::GetIndexCount() const {
    return m_indexCount;
}

uint32_t GPUMesh::GetVertexCount() const {
    return m_vertexCount;
}

bool GPUMesh::IsIndexed() const {
    return m_indexBuffer != nullptr && m_indexCount > 0;
}

bool GPUMesh::IsValid() const {
    return m_vertexBuffer != nullptr && m_vertexCount > 0;
}
