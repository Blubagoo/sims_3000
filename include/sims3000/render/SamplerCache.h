/**
 * @file SamplerCache.h
 * @brief GPU sampler creation and caching.
 *
 * Provides a cache for SDL_GPUSampler objects with different filter
 * and address mode configurations. Samplers are cached by configuration
 * to avoid redundant GPU resource creation.
 *
 * Resource ownership:
 * - SamplerCache owns all SDL_GPUSampler instances it creates
 * - Destruction releases all samplers
 *
 * Thread safety:
 * - Not thread-safe. Call from render thread only.
 */

#ifndef SIMS3000_RENDER_SAMPLER_CACHE_H
#define SIMS3000_RENDER_SAMPLER_CACHE_H

#include <SDL3/SDL.h>
#include <cstdint>
#include <unordered_map>
#include <string>

namespace sims3000 {

/**
 * @enum SamplerFilter
 * @brief Texture filtering modes.
 */
enum class SamplerFilter {
    Nearest,  ///< Point sampling (pixelated)
    Linear    ///< Bilinear interpolation (smooth)
};

/**
 * @enum SamplerAddressMode
 * @brief Texture coordinate wrapping modes.
 */
enum class SamplerAddressMode {
    Repeat,         ///< Tile the texture
    ClampToEdge,    ///< Clamp to edge pixels
    MirroredRepeat  ///< Tile with mirroring
};

/**
 * @struct SamplerConfig
 * @brief Configuration for creating a sampler.
 */
struct SamplerConfig {
    SamplerFilter minFilter = SamplerFilter::Linear;
    SamplerFilter magFilter = SamplerFilter::Linear;
    SamplerFilter mipFilter = SamplerFilter::Linear;
    SamplerAddressMode addressModeU = SamplerAddressMode::Repeat;
    SamplerAddressMode addressModeV = SamplerAddressMode::Repeat;
    SamplerAddressMode addressModeW = SamplerAddressMode::Repeat;
    float maxAnisotropy = 1.0f;  ///< 1.0 = disabled, up to 16.0 for anisotropic filtering

    bool operator==(const SamplerConfig& other) const {
        return minFilter == other.minFilter &&
               magFilter == other.magFilter &&
               mipFilter == other.mipFilter &&
               addressModeU == other.addressModeU &&
               addressModeV == other.addressModeV &&
               addressModeW == other.addressModeW &&
               maxAnisotropy == other.maxAnisotropy;
    }
};

/**
 * @class SamplerCache
 * @brief Caches GPU samplers by configuration.
 *
 * Provides commonly-used sampler presets and custom sampler creation.
 * All samplers are cached for reuse.
 *
 * Usage:
 * @code
 *   SamplerCache cache(device);
 *
 *   // Get a standard sampler
 *   SDL_GPUSampler* linear = cache.getLinear();
 *   SDL_GPUSampler* nearest = cache.getNearest();
 *
 *   // Or create a custom configuration
 *   SamplerConfig config;
 *   config.minFilter = SamplerFilter::Linear;
 *   config.magFilter = SamplerFilter::Nearest;
 *   config.maxAnisotropy = 4.0f;
 *   SDL_GPUSampler* custom = cache.getSampler(config);
 * @endcode
 */
class SamplerCache {
public:
    /**
     * Create a sampler cache.
     * @param device SDL GPU device for sampler creation
     */
    explicit SamplerCache(SDL_GPUDevice* device);

    ~SamplerCache();

    // Non-copyable
    SamplerCache(const SamplerCache&) = delete;
    SamplerCache& operator=(const SamplerCache&) = delete;

    // Movable
    SamplerCache(SamplerCache&& other) noexcept;
    SamplerCache& operator=(SamplerCache&& other) noexcept;

    /**
     * Get sampler for a given configuration.
     * Creates the sampler if it doesn't exist in cache.
     *
     * @param config Sampler configuration
     * @return Sampler handle, or nullptr on failure
     */
    SDL_GPUSampler* getSampler(const SamplerConfig& config);

    /**
     * Get linear filtering sampler (bilinear + repeat).
     * Common for most 3D textures.
     * @return Sampler handle
     */
    SDL_GPUSampler* getLinear();

    /**
     * Get nearest filtering sampler (point + repeat).
     * Good for pixel art or when you want sharp texels.
     * @return Sampler handle
     */
    SDL_GPUSampler* getNearest();

    /**
     * Get linear sampler with clamp-to-edge.
     * Good for UI elements and non-tiling textures.
     * @return Sampler handle
     */
    SDL_GPUSampler* getLinearClamp();

    /**
     * Get nearest sampler with clamp-to-edge.
     * Good for pixel art UI elements.
     * @return Sampler handle
     */
    SDL_GPUSampler* getNearestClamp();

    /**
     * Get anisotropic filtering sampler.
     * High-quality filtering for surfaces at oblique angles.
     * @param maxAnisotropy Maximum anisotropy level (1-16)
     * @return Sampler handle
     */
    SDL_GPUSampler* getAnisotropic(float maxAnisotropy = 4.0f);

    /**
     * Clear all cached samplers.
     * Releases all GPU resources.
     */
    void clear();

    /**
     * Get number of cached samplers.
     * @return Cache size
     */
    std::size_t size() const { return m_cache.size(); }

    /**
     * Get the last error message.
     * @return Error string
     */
    const std::string& getLastError() const { return m_lastError; }

    /**
     * Check if cache is valid (has a device).
     * @return true if cache can create samplers
     */
    bool isValid() const { return m_device != nullptr; }

private:
    /**
     * Hash function for SamplerConfig.
     */
    struct ConfigHash {
        std::size_t operator()(const SamplerConfig& config) const;
    };

    /**
     * Create SDL sampler from config.
     * @param config Sampler configuration
     * @return Sampler handle, or nullptr on failure
     */
    SDL_GPUSampler* createSampler(const SamplerConfig& config);

    /**
     * Convert SamplerFilter to SDL_GPUFilter.
     */
    static SDL_GPUFilter toSDLFilter(SamplerFilter filter);

    /**
     * Convert SamplerAddressMode to SDL_GPUSamplerAddressMode.
     */
    static SDL_GPUSamplerAddressMode toSDLAddressMode(SamplerAddressMode mode);

    SDL_GPUDevice* m_device = nullptr;
    std::unordered_map<SamplerConfig, SDL_GPUSampler*, ConfigHash> m_cache;
    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_SAMPLER_CACHE_H
