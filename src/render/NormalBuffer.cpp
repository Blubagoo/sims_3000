/**
 * @file NormalBuffer.cpp
 * @brief Normal G-buffer implementation for screen-space edge detection.
 */

#include "sims3000/render/NormalBuffer.h"
#include "sims3000/render/GPUDevice.h"
#include <SDL3/SDL_log.h>

namespace sims3000 {

// =============================================================================
// Construction / Destruction
// =============================================================================

NormalBuffer::NormalBuffer(GPUDevice& device, std::uint32_t width, std::uint32_t height)
    : m_device(device.getHandle())
    , m_width(width)
    , m_height(height)
{
    if (!m_device) {
        m_lastError = "Cannot create normal buffer: GPU device is not valid";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return;
    }

    if (width == 0 || height == 0) {
        m_lastError = "Cannot create normal buffer: dimensions must be non-zero";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return;
    }

    if (!createTexture()) {
        // Error already logged in createTexture()
        return;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU,
                "NormalBuffer: Created %ux%u normal buffer (format: RGBA16F)",
                width, height);
}

NormalBuffer::~NormalBuffer() {
    releaseTexture();
}

NormalBuffer::NormalBuffer(NormalBuffer&& other) noexcept
    : m_device(other.m_device)
    , m_texture(other.m_texture)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_texture = nullptr;
    other.m_width = 0;
    other.m_height = 0;
}

NormalBuffer& NormalBuffer::operator=(NormalBuffer&& other) noexcept {
    if (this != &other) {
        releaseTexture();

        m_device = other.m_device;
        m_texture = other.m_texture;
        m_width = other.m_width;
        m_height = other.m_height;
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
        other.m_texture = nullptr;
        other.m_width = 0;
        other.m_height = 0;
    }
    return *this;
}

// =============================================================================
// State Queries
// =============================================================================

bool NormalBuffer::isValid() const {
    return m_texture != nullptr;
}

SDL_GPUTexture* NormalBuffer::getHandle() const {
    return m_texture;
}

std::uint32_t NormalBuffer::getWidth() const {
    return m_width;
}

std::uint32_t NormalBuffer::getHeight() const {
    return m_height;
}

SDL_GPUTextureFormat NormalBuffer::getFormat() const {
    return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
}

const std::string& NormalBuffer::getLastError() const {
    return m_lastError;
}

// =============================================================================
// Resize Operations
// =============================================================================

bool NormalBuffer::resize(std::uint32_t newWidth, std::uint32_t newHeight) {
    if (newWidth == 0 || newHeight == 0) {
        m_lastError = "Cannot resize normal buffer: dimensions must be non-zero";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    // Skip if dimensions haven't changed
    if (newWidth == m_width && newHeight == m_height) {
        return true;
    }

    if (!m_device) {
        m_lastError = "Cannot resize normal buffer: GPU device is not valid";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    // Release old texture
    releaseTexture();

    // Update dimensions
    m_width = newWidth;
    m_height = newHeight;

    // Create new texture at new size
    if (!createTexture()) {
        return false;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU,
                "NormalBuffer: Resized to %ux%u", newWidth, newHeight);

    return true;
}

// =============================================================================
// Render Pass Configuration
// =============================================================================

SDL_GPUColorTargetInfo NormalBuffer::getColorTargetInfo() const {
    // Default clear to neutral normal (0.5, 0.5, 1.0) encoded as up-facing
    return getColorTargetInfo(0.5f, 0.5f, 1.0f, 1.0f);
}

SDL_GPUColorTargetInfo NormalBuffer::getColorTargetInfo(float clearR, float clearG,
                                                         float clearB, float clearA) const {
    SDL_GPUColorTargetInfo info = {};
    info.texture = m_texture;
    info.load_op = SDL_GPU_LOADOP_CLEAR;
    info.store_op = SDL_GPU_STOREOP_STORE;  // Must store for sampling in edge pass
    info.clear_color = {clearR, clearG, clearB, clearA};
    info.cycle = true;  // Allow texture cycling for performance
    return info;
}

SDL_GPUColorTargetInfo NormalBuffer::getColorTargetInfoPreserve() const {
    SDL_GPUColorTargetInfo info = {};
    info.texture = m_texture;
    info.load_op = SDL_GPU_LOADOP_LOAD;
    info.store_op = SDL_GPU_STOREOP_STORE;
    info.clear_color = {0.5f, 0.5f, 1.0f, 1.0f};
    info.cycle = false;  // Don't cycle when preserving
    return info;
}

// =============================================================================
// Private Implementation
// =============================================================================

bool NormalBuffer::createTexture() {
    SDL_GPUTextureCreateInfo createInfo = {};
    createInfo.type = SDL_GPU_TEXTURETYPE_2D;
    createInfo.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    createInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                       SDL_GPU_TEXTUREUSAGE_SAMPLER;  // Sampleable for edge detection
    createInfo.width = m_width;
    createInfo.height = m_height;
    createInfo.layer_count_or_depth = 1;
    createInfo.num_levels = 1;
    createInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

    m_texture = SDL_CreateGPUTexture(m_device, &createInfo);

    if (!m_texture) {
        m_lastError = "Failed to create normal texture: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    return true;
}

void NormalBuffer::releaseTexture() {
    if (m_texture && m_device) {
        SDL_ReleaseGPUTexture(m_device, m_texture);
        SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "NormalBuffer: Released normal texture");
    }
    m_texture = nullptr;
}

} // namespace sims3000
