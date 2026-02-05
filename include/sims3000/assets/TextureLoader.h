/**
 * @file TextureLoader.h
 * @brief GPU texture loading and management.
 */

#ifndef SIMS3000_ASSETS_TEXTURELOADER_H
#define SIMS3000_ASSETS_TEXTURELOADER_H

#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace sims3000 {

// Forward declarations
class Window;

/**
 * @struct Texture
 * @brief GPU texture resource.
 */
struct Texture {
    SDL_GPUTexture* gpuTexture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    int width = 0;
    int height = 0;
    int refCount = 0;
    std::string path;
    std::uint64_t lastModified = 0;
};

/// Handle to a loaded texture
using TextureHandle = Texture*;

/**
 * @class TextureLoader
 * @brief Loads and caches GPU textures.
 *
 * Supports PNG, JPG, and other common image formats.
 * Creates SDL_GPU textures with appropriate formats.
 */
class TextureLoader {
public:
    /**
     * Create texture loader.
     * @param window Window for GPU device access
     */
    explicit TextureLoader(Window& window);
    ~TextureLoader();

    // Non-copyable
    TextureLoader(const TextureLoader&) = delete;
    TextureLoader& operator=(const TextureLoader&) = delete;

    /**
     * Load texture from file.
     * @param path Full path to image file
     * @return Texture handle, or nullptr on failure
     */
    TextureHandle load(const std::string& path);

    /**
     * Create fallback texture (magenta checkerboard).
     * @return Fallback texture handle
     */
    TextureHandle createFallback();

    /**
     * Increment reference count.
     * @param handle Texture handle
     */
    void addRef(TextureHandle handle);

    /**
     * Decrement reference count.
     * @param handle Texture handle
     */
    void release(TextureHandle handle);

    /**
     * Clear textures with zero references.
     */
    void clearUnused();

    /**
     * Clear all textures.
     */
    void clearAll();

    /**
     * Get cache statistics.
     * @param outCount Number of cached textures
     * @param outBytes Total memory used
     */
    void getStats(size_t& outCount, size_t& outBytes) const;

    /**
     * Reload a texture if the file was modified.
     * @param handle Texture to check
     * @return true if reloaded
     */
    bool reloadIfModified(TextureHandle handle);

private:
    SDL_GPUTexture* createGPUTexture(SDL_Surface* surface);
    SDL_GPUSampler* createSampler();

    Window& m_window;
    std::unordered_map<std::string, Texture> m_cache;
    SDL_GPUSampler* m_defaultSampler = nullptr;
};

} // namespace sims3000

#endif // SIMS3000_ASSETS_TEXTURELOADER_H
