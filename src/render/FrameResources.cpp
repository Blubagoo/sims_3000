/**
 * @file FrameResources.cpp
 * @brief FrameResources implementation.
 */

#include "sims3000/render/FrameResources.h"
#include <SDL3/SDL_log.h>
#include <utility>
#include <cstring>

namespace sims3000 {

FrameResources::FrameResources(SDL_GPUDevice* device)
    : FrameResources(device, FrameResourcesConfig{})
{
}

FrameResources::FrameResources(SDL_GPUDevice* device, const FrameResourcesConfig& config)
    : m_device(device)
    , m_config(config)
{
    if (!device) {
        m_lastError = "Cannot create FrameResources: device is null";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return;
    }

    if (m_config.frameCount < 2) {
        m_config.frameCount = 2;  // Minimum double buffering
    }
    if (m_config.frameCount > 4) {
        m_config.frameCount = 4;  // Cap at quad buffering
    }

    if (!initialize()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "FrameResources: Failed to initialize frame sets");
    }
}

FrameResources::~FrameResources() {
    releaseAll();
}

FrameResources::FrameResources(FrameResources&& other) noexcept
    : m_device(other.m_device)
    , m_config(other.m_config)
    , m_frameSets(std::move(other.m_frameSets))
    , m_currentFrame(other.m_currentFrame)
    , m_totalFrames(other.m_totalFrames)
    , m_inFrame(other.m_inFrame)
    , m_uniformMappedPtr(other.m_uniformMappedPtr)
    , m_textureMappedPtr(other.m_textureMappedPtr)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_frameSets.clear();
    other.m_uniformMappedPtr = nullptr;
    other.m_textureMappedPtr = nullptr;
}

FrameResources& FrameResources::operator=(FrameResources&& other) noexcept {
    if (this != &other) {
        releaseAll();

        m_device = other.m_device;
        m_config = other.m_config;
        m_frameSets = std::move(other.m_frameSets);
        m_currentFrame = other.m_currentFrame;
        m_totalFrames = other.m_totalFrames;
        m_inFrame = other.m_inFrame;
        m_uniformMappedPtr = other.m_uniformMappedPtr;
        m_textureMappedPtr = other.m_textureMappedPtr;
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
        other.m_frameSets.clear();
        other.m_uniformMappedPtr = nullptr;
        other.m_textureMappedPtr = nullptr;
    }
    return *this;
}

void FrameResources::beginFrame() {
    if (!m_device || m_frameSets.empty()) {
        return;
    }

    if (m_inFrame) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                    "FrameResources: beginFrame() called while already in frame");
        endFrame();
    }

    // Advance to next frame
    m_currentFrame = (m_currentFrame + 1) % m_config.frameCount;
    m_totalFrames++;

    FrameResourceSet& set = m_frameSets[m_currentFrame];

    // Reset allocation offsets
    set.uniformTransferOffset = 0;
    set.textureTransferOffset = 0;
    set.lastUsedFrame = m_totalFrames;

    // Map transfer buffers for writing
    if (set.uniformTransfer) {
        m_uniformMappedPtr = SDL_MapGPUTransferBuffer(m_device, set.uniformTransfer, false);
        if (!m_uniformMappedPtr) {
            SDL_LogError(SDL_LOG_CATEGORY_GPU,
                         "FrameResources: Failed to map uniform transfer buffer: %s",
                         SDL_GetError());
        }
    }

    if (set.textureTransfer) {
        m_textureMappedPtr = SDL_MapGPUTransferBuffer(m_device, set.textureTransfer, false);
        if (!m_textureMappedPtr) {
            SDL_LogError(SDL_LOG_CATEGORY_GPU,
                         "FrameResources: Failed to map texture transfer buffer: %s",
                         SDL_GetError());
        }
    }

    m_inFrame = true;
}

void FrameResources::endFrame() {
    if (!m_device || !m_inFrame) {
        return;
    }

    FrameResourceSet& set = m_frameSets[m_currentFrame];

    // Unmap transfer buffers
    if (set.uniformTransfer && m_uniformMappedPtr) {
        SDL_UnmapGPUTransferBuffer(m_device, set.uniformTransfer);
        m_uniformMappedPtr = nullptr;
    }

    if (set.textureTransfer && m_textureMappedPtr) {
        SDL_UnmapGPUTransferBuffer(m_device, set.textureTransfer);
        m_textureMappedPtr = nullptr;
    }

    m_inFrame = false;
}

bool FrameResources::allocateUniformData(std::uint32_t size, void** outPtr, std::uint32_t* outOffset) {
    if (!m_inFrame || !m_uniformMappedPtr) {
        m_lastError = "Cannot allocate: not in frame or buffer not mapped";
        return false;
    }

    if (size == 0) {
        m_lastError = "Cannot allocate: size is zero";
        return false;
    }

    FrameResourceSet& set = m_frameSets[m_currentFrame];

    // Check if allocation fits
    if (set.uniformTransferOffset + size > m_config.uniformTransferSize) {
        m_lastError = "Uniform transfer buffer exhausted";
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    *outOffset = set.uniformTransferOffset;
    *outPtr = static_cast<char*>(m_uniformMappedPtr) + set.uniformTransferOffset;

    set.uniformTransferOffset += size;

    return true;
}

bool FrameResources::allocateTextureData(std::uint32_t size, void** outPtr, std::uint32_t* outOffset) {
    if (!m_inFrame || !m_textureMappedPtr) {
        m_lastError = "Cannot allocate: not in frame or buffer not mapped";
        return false;
    }

    if (size == 0) {
        m_lastError = "Cannot allocate: size is zero";
        return false;
    }

    FrameResourceSet& set = m_frameSets[m_currentFrame];

    // Check if allocation fits
    if (set.textureTransferOffset + size > m_config.textureTransferSize) {
        m_lastError = "Texture transfer buffer exhausted";
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    *outOffset = set.textureTransferOffset;
    *outPtr = static_cast<char*>(m_textureMappedPtr) + set.textureTransferOffset;

    set.textureTransferOffset += size;

    return true;
}

SDL_GPUTransferBuffer* FrameResources::getUniformTransferBuffer() const {
    if (m_frameSets.empty()) {
        return nullptr;
    }
    return m_frameSets[m_currentFrame].uniformTransfer;
}

SDL_GPUTransferBuffer* FrameResources::getTextureTransferBuffer() const {
    if (m_frameSets.empty()) {
        return nullptr;
    }
    return m_frameSets[m_currentFrame].textureTransfer;
}

FrameResourcesStats FrameResources::getStats() const {
    FrameResourcesStats stats;
    stats.frameCount = m_config.frameCount;
    stats.currentFrame = m_currentFrame;
    stats.totalFramesRendered = m_totalFrames;
    stats.uniformTransferCapacity = m_config.uniformTransferSize;
    stats.textureTransferCapacity = m_config.textureTransferSize;

    if (!m_frameSets.empty()) {
        const auto& set = m_frameSets[m_currentFrame];
        stats.uniformBytesUsed = set.uniformTransferOffset;
        stats.textureBytesUsed = set.textureTransferOffset;
    }

    return stats;
}

void FrameResources::releaseAll() {
    // End any in-progress frame
    if (m_inFrame) {
        endFrame();
    }

    if (m_device) {
        for (auto& set : m_frameSets) {
            destroyFrameSet(set);
        }
    }

    m_frameSets.clear();
    m_currentFrame = 0;
    m_uniformMappedPtr = nullptr;
    m_textureMappedPtr = nullptr;
}

bool FrameResources::initialize() {
    m_frameSets.resize(m_config.frameCount);

    for (std::uint32_t i = 0; i < m_config.frameCount; ++i) {
        if (!createFrameSet(m_frameSets[i])) {
            // Clean up any created sets
            for (std::uint32_t j = 0; j < i; ++j) {
                destroyFrameSet(m_frameSets[j]);
            }
            m_frameSets.clear();
            return false;
        }
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU,
                "FrameResources: Created %u frame sets (uniform=%uKB, texture=%uKB each)",
                m_config.frameCount,
                m_config.uniformTransferSize / 1024,
                m_config.textureTransferSize / 1024);

    // Start at frame -1 so first beginFrame() goes to frame 0
    m_currentFrame = m_config.frameCount - 1;

    return true;
}

bool FrameResources::createFrameSet(FrameResourceSet& set) {
    // Create uniform transfer buffer
    SDL_GPUTransferBufferCreateInfo uniformInfo = {};
    uniformInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    uniformInfo.size = m_config.uniformTransferSize;

    set.uniformTransfer = SDL_CreateGPUTransferBuffer(m_device, &uniformInfo);
    if (!set.uniformTransfer) {
        m_lastError = "Failed to create uniform transfer buffer: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    // Create texture transfer buffer
    SDL_GPUTransferBufferCreateInfo textureInfo = {};
    textureInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    textureInfo.size = m_config.textureTransferSize;

    set.textureTransfer = SDL_CreateGPUTransferBuffer(m_device, &textureInfo);
    if (!set.textureTransfer) {
        m_lastError = "Failed to create texture transfer buffer: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        SDL_ReleaseGPUTransferBuffer(m_device, set.uniformTransfer);
        set.uniformTransfer = nullptr;
        return false;
    }

    set.uniformTransferOffset = 0;
    set.textureTransferOffset = 0;
    set.lastUsedFrame = 0;
    set.pendingRelease = false;

    return true;
}

void FrameResources::destroyFrameSet(FrameResourceSet& set) {
    if (set.uniformTransfer) {
        SDL_ReleaseGPUTransferBuffer(m_device, set.uniformTransfer);
        set.uniformTransfer = nullptr;
    }
    if (set.textureTransfer) {
        SDL_ReleaseGPUTransferBuffer(m_device, set.textureTransfer);
        set.textureTransfer = nullptr;
    }
}

} // namespace sims3000
