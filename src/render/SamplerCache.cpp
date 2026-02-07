/**
 * @file SamplerCache.cpp
 * @brief SamplerCache implementation.
 */

#include "sims3000/render/SamplerCache.h"
#include <SDL3/SDL_log.h>
#include <utility>

namespace sims3000 {

SamplerCache::SamplerCache(SDL_GPUDevice* device)
    : m_device(device)
{
    if (!device) {
        m_lastError = "Cannot create SamplerCache: device is null";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
    }
}

SamplerCache::~SamplerCache() {
    clear();
}

SamplerCache::SamplerCache(SamplerCache&& other) noexcept
    : m_device(other.m_device)
    , m_cache(std::move(other.m_cache))
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_cache.clear();
}

SamplerCache& SamplerCache::operator=(SamplerCache&& other) noexcept {
    if (this != &other) {
        clear();

        m_device = other.m_device;
        m_cache = std::move(other.m_cache);
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
        other.m_cache.clear();
    }
    return *this;
}

SDL_GPUSampler* SamplerCache::getSampler(const SamplerConfig& config) {
    if (!m_device) {
        return nullptr;
    }

    // Check cache first
    auto it = m_cache.find(config);
    if (it != m_cache.end()) {
        return it->second;
    }

    // Create new sampler
    SDL_GPUSampler* sampler = createSampler(config);
    if (sampler) {
        m_cache[config] = sampler;
    }

    return sampler;
}

SDL_GPUSampler* SamplerCache::getLinear() {
    SamplerConfig config;
    config.minFilter = SamplerFilter::Linear;
    config.magFilter = SamplerFilter::Linear;
    config.mipFilter = SamplerFilter::Linear;
    config.addressModeU = SamplerAddressMode::Repeat;
    config.addressModeV = SamplerAddressMode::Repeat;
    config.addressModeW = SamplerAddressMode::Repeat;
    return getSampler(config);
}

SDL_GPUSampler* SamplerCache::getNearest() {
    SamplerConfig config;
    config.minFilter = SamplerFilter::Nearest;
    config.magFilter = SamplerFilter::Nearest;
    config.mipFilter = SamplerFilter::Nearest;
    config.addressModeU = SamplerAddressMode::Repeat;
    config.addressModeV = SamplerAddressMode::Repeat;
    config.addressModeW = SamplerAddressMode::Repeat;
    return getSampler(config);
}

SDL_GPUSampler* SamplerCache::getLinearClamp() {
    SamplerConfig config;
    config.minFilter = SamplerFilter::Linear;
    config.magFilter = SamplerFilter::Linear;
    config.mipFilter = SamplerFilter::Linear;
    config.addressModeU = SamplerAddressMode::ClampToEdge;
    config.addressModeV = SamplerAddressMode::ClampToEdge;
    config.addressModeW = SamplerAddressMode::ClampToEdge;
    return getSampler(config);
}

SDL_GPUSampler* SamplerCache::getNearestClamp() {
    SamplerConfig config;
    config.minFilter = SamplerFilter::Nearest;
    config.magFilter = SamplerFilter::Nearest;
    config.mipFilter = SamplerFilter::Nearest;
    config.addressModeU = SamplerAddressMode::ClampToEdge;
    config.addressModeV = SamplerAddressMode::ClampToEdge;
    config.addressModeW = SamplerAddressMode::ClampToEdge;
    return getSampler(config);
}

SDL_GPUSampler* SamplerCache::getAnisotropic(float maxAnisotropy) {
    SamplerConfig config;
    config.minFilter = SamplerFilter::Linear;
    config.magFilter = SamplerFilter::Linear;
    config.mipFilter = SamplerFilter::Linear;
    config.addressModeU = SamplerAddressMode::Repeat;
    config.addressModeV = SamplerAddressMode::Repeat;
    config.addressModeW = SamplerAddressMode::Repeat;
    config.maxAnisotropy = maxAnisotropy;
    return getSampler(config);
}

void SamplerCache::clear() {
    if (m_device) {
        for (auto& [config, sampler] : m_cache) {
            if (sampler) {
                SDL_ReleaseGPUSampler(m_device, sampler);
            }
        }
    }
    m_cache.clear();
}

SDL_GPUSampler* SamplerCache::createSampler(const SamplerConfig& config) {
    if (!m_device) {
        return nullptr;
    }

    SDL_GPUSamplerCreateInfo createInfo = {};
    createInfo.min_filter = toSDLFilter(config.minFilter);
    createInfo.mag_filter = toSDLFilter(config.magFilter);
    createInfo.mipmap_mode = config.mipFilter == SamplerFilter::Linear
        ? SDL_GPU_SAMPLERMIPMAPMODE_LINEAR
        : SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    createInfo.address_mode_u = toSDLAddressMode(config.addressModeU);
    createInfo.address_mode_v = toSDLAddressMode(config.addressModeV);
    createInfo.address_mode_w = toSDLAddressMode(config.addressModeW);

    // Anisotropic filtering
    if (config.maxAnisotropy > 1.0f) {
        createInfo.enable_anisotropy = true;
        createInfo.max_anisotropy = config.maxAnisotropy;
    }

    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(m_device, &createInfo);
    if (!sampler) {
        m_lastError = "Failed to create sampler: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
    }

    return sampler;
}

std::size_t SamplerCache::ConfigHash::operator()(const SamplerConfig& config) const {
    // Simple hash combining
    std::size_t h = 0;
    h ^= static_cast<std::size_t>(config.minFilter) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= static_cast<std::size_t>(config.magFilter) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= static_cast<std::size_t>(config.mipFilter) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= static_cast<std::size_t>(config.addressModeU) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= static_cast<std::size_t>(config.addressModeV) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= static_cast<std::size_t>(config.addressModeW) + 0x9e3779b9 + (h << 6) + (h >> 2);

    // Hash float by converting to int bits
    union { float f; std::uint32_t i; } u;
    u.f = config.maxAnisotropy;
    h ^= u.i + 0x9e3779b9 + (h << 6) + (h >> 2);

    return h;
}

SDL_GPUFilter SamplerCache::toSDLFilter(SamplerFilter filter) {
    switch (filter) {
        case SamplerFilter::Nearest:
            return SDL_GPU_FILTER_NEAREST;
        case SamplerFilter::Linear:
        default:
            return SDL_GPU_FILTER_LINEAR;
    }
}

SDL_GPUSamplerAddressMode SamplerCache::toSDLAddressMode(SamplerAddressMode mode) {
    switch (mode) {
        case SamplerAddressMode::ClampToEdge:
            return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        case SamplerAddressMode::MirroredRepeat:
            return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
        case SamplerAddressMode::Repeat:
        default:
            return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    }
}

} // namespace sims3000
