/**
 * @file DepthBuffer.cpp
 * @brief Depth buffer implementation for SDL_GPU automatic occlusion handling.
 */

#include "sims3000/render/DepthBuffer.h"
#include "sims3000/render/GPUDevice.h"
#include <SDL3/SDL_log.h>

namespace sims3000 {

// =============================================================================
// Free Functions
// =============================================================================

const char* getDepthFormatName(DepthFormat format) {
    switch (format) {
        case DepthFormat::D32_FLOAT:        return "D32_FLOAT";
        case DepthFormat::D24_UNORM_S8_UINT: return "D24_UNORM_S8_UINT";
        default:                             return "Unknown";
    }
}

// =============================================================================
// Construction / Destruction
// =============================================================================

DepthBuffer::DepthBuffer(GPUDevice& device, uint32_t width, uint32_t height)
    : DepthBuffer(device, width, height, DepthFormat::D32_FLOAT)
{
}

DepthBuffer::DepthBuffer(GPUDevice& device, uint32_t width, uint32_t height, DepthFormat format)
    : m_device(device.getHandle())
    , m_width(width)
    , m_height(height)
    , m_format(format)
{
    if (!m_device) {
        m_lastError = "Cannot create depth buffer: GPU device is not valid";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return;
    }

    if (width == 0 || height == 0) {
        m_lastError = "Cannot create depth buffer: dimensions must be non-zero";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return;
    }

    if (!createTexture()) {
        // Error already logged in createTexture()
        return;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU,
                "DepthBuffer: Created %ux%u depth buffer (format: %s)",
                width, height, getDepthFormatName(format));
}

DepthBuffer::~DepthBuffer() {
    releaseTexture();
}

DepthBuffer::DepthBuffer(DepthBuffer&& other) noexcept
    : m_device(other.m_device)
    , m_texture(other.m_texture)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_format(other.m_format)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_texture = nullptr;
    other.m_width = 0;
    other.m_height = 0;
}

DepthBuffer& DepthBuffer::operator=(DepthBuffer&& other) noexcept {
    if (this != &other) {
        releaseTexture();

        m_device = other.m_device;
        m_texture = other.m_texture;
        m_width = other.m_width;
        m_height = other.m_height;
        m_format = other.m_format;
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

bool DepthBuffer::isValid() const {
    return m_texture != nullptr;
}

SDL_GPUTexture* DepthBuffer::getHandle() const {
    return m_texture;
}

uint32_t DepthBuffer::getWidth() const {
    return m_width;
}

uint32_t DepthBuffer::getHeight() const {
    return m_height;
}

DepthFormat DepthBuffer::getFormat() const {
    return m_format;
}

SDL_GPUTextureFormat DepthBuffer::getSDLFormat() const {
    return toSDLFormat(m_format);
}

bool DepthBuffer::hasStencil() const {
    return m_format == DepthFormat::D24_UNORM_S8_UINT;
}

const std::string& DepthBuffer::getLastError() const {
    return m_lastError;
}

// =============================================================================
// Resize Operations
// =============================================================================

bool DepthBuffer::resize(uint32_t newWidth, uint32_t newHeight) {
    if (newWidth == 0 || newHeight == 0) {
        m_lastError = "Cannot resize depth buffer: dimensions must be non-zero";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    // Skip if dimensions haven't changed
    if (newWidth == m_width && newHeight == m_height) {
        return true;
    }

    if (!m_device) {
        m_lastError = "Cannot resize depth buffer: GPU device is not valid";
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
                "DepthBuffer: Resized to %ux%u", newWidth, newHeight);

    return true;
}

// =============================================================================
// Render Pass Configuration
// =============================================================================

SDL_GPUDepthStencilTargetInfo DepthBuffer::getDepthStencilTargetInfo() const {
    return getDepthStencilTargetInfo(1.0f, 0);
}

SDL_GPUDepthStencilTargetInfo DepthBuffer::getDepthStencilTargetInfo(float clearDepth) const {
    return getDepthStencilTargetInfo(clearDepth, 0);
}

SDL_GPUDepthStencilTargetInfo DepthBuffer::getDepthStencilTargetInfo(float clearDepth, uint8_t clearStencil) const {
    SDL_GPUDepthStencilTargetInfo info = {};
    info.texture = m_texture;
    info.load_op = SDL_GPU_LOADOP_CLEAR;
    info.store_op = SDL_GPU_STOREOP_DONT_CARE;
    info.stencil_load_op = hasStencil() ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_DONT_CARE;
    info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    info.clear_depth = clearDepth;
    info.clear_stencil = clearStencil;
    info.cycle = true;  // Allow texture cycling for performance
    return info;
}

SDL_GPUDepthStencilTargetInfo DepthBuffer::getDepthStencilTargetInfoPreserve() const {
    SDL_GPUDepthStencilTargetInfo info = {};
    info.texture = m_texture;
    info.load_op = SDL_GPU_LOADOP_LOAD;
    info.store_op = SDL_GPU_STOREOP_STORE;
    info.stencil_load_op = hasStencil() ? SDL_GPU_LOADOP_LOAD : SDL_GPU_LOADOP_DONT_CARE;
    info.stencil_store_op = hasStencil() ? SDL_GPU_STOREOP_STORE : SDL_GPU_STOREOP_DONT_CARE;
    info.clear_depth = 1.0f;
    info.clear_stencil = 0;
    info.cycle = false;  // Don't cycle when preserving
    return info;
}

SDL_GPUDepthStencilTargetInfo DepthBuffer::getDepthStencilTargetInfoSampleable(float clearDepth) const {
    SDL_GPUDepthStencilTargetInfo info = {};
    info.texture = m_texture;
    info.load_op = SDL_GPU_LOADOP_CLEAR;
    info.store_op = SDL_GPU_STOREOP_STORE;  // Store for sampling in post-process
    info.stencil_load_op = hasStencil() ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_DONT_CARE;
    info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    info.clear_depth = clearDepth;
    info.clear_stencil = 0;
    info.cycle = true;  // Allow texture cycling for performance
    return info;
}

// =============================================================================
// Private Implementation
// =============================================================================

bool DepthBuffer::createTexture() {
    SDL_GPUTextureCreateInfo createInfo = {};
    createInfo.type = SDL_GPU_TEXTURETYPE_2D;
    createInfo.format = toSDLFormat(m_format);
    createInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET |
                       SDL_GPU_TEXTUREUSAGE_SAMPLER;  // Sampleable for post-process edge detection
    createInfo.width = m_width;
    createInfo.height = m_height;
    createInfo.layer_count_or_depth = 1;
    createInfo.num_levels = 1;
    createInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

    m_texture = SDL_CreateGPUTexture(m_device, &createInfo);

    if (!m_texture) {
        m_lastError = "Failed to create depth texture: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    return true;
}

void DepthBuffer::releaseTexture() {
    if (m_texture && m_device) {
        SDL_ReleaseGPUTexture(m_device, m_texture);
        SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "DepthBuffer: Released depth texture");
    }
    m_texture = nullptr;
}

SDL_GPUTextureFormat DepthBuffer::toSDLFormat(DepthFormat format) {
    switch (format) {
        case DepthFormat::D32_FLOAT:
            return SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        case DepthFormat::D24_UNORM_S8_UINT:
            return SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
        default:
            return SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    }
}

} // namespace sims3000
