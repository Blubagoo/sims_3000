/**
 * @file TextureLoader.h
 * @brief GPU texture loading and management.
 *
 * Loads textures from various image formats (PNG, JPG, BMP, etc.) using
 * stb_image and creates SDL_GPU textures. Provides caching, reference
 * counting, and hot-reload support.
 *
 * Resource ownership:
 * - TextureLoader owns all SDL_GPUTexture and SDL_GPUSampler instances
 * - Reference counting tracks usage; clearUnused() releases zero-ref textures
 * - Hot-reload watches file modification times
 *
 * Supported formats:
 * - PNG (recommended for assets with alpha)
 * - JPEG (for large photographic textures)
 * - BMP (for compatibility)
 * - TGA, PSD, GIF, HDR, PIC (via stb_image)
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
class SamplerCache;

/**
 * @enum TextureFilter
 * @brief Texture filtering mode for sampling.
 */
enum class TextureFilter {
    Linear,   ///< Bilinear filtering (smooth)
    Nearest   ///< Point filtering (pixelated)
};

/**
 * @struct TextureLoadOptions
 * @brief Options for texture loading.
 */
struct TextureLoadOptions {
    TextureFilter filter = TextureFilter::Linear;  ///< Filtering mode
    bool generateMipmaps = false;                   ///< Generate mipmaps (not yet implemented)
    bool sRGB = true;                               ///< Treat as sRGB color space
};

/**
 * @struct Texture
 * @brief GPU texture resource.
 */
struct Texture {
    SDL_GPUTexture* gpuTexture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    int width = 0;
    int height = 0;
    int channels = 0;              ///< Original channel count (3=RGB, 4=RGBA)
    int refCount = 0;
    std::string path;
    std::uint64_t lastModified = 0;
    TextureFilter filter = TextureFilter::Linear;
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
     * Load texture from file with default options.
     * Supports PNG, JPG, BMP, TGA, PSD, GIF, HDR, PIC formats.
     *
     * @param path Full path to image file
     * @return Texture handle, or nullptr on failure
     */
    TextureHandle load(const std::string& path);

    /**
     * Load texture from file with custom options.
     *
     * @param path Full path to image file
     * @param options Loading options (filter, mipmaps, etc.)
     * @return Texture handle, or nullptr on failure
     */
    TextureHandle load(const std::string& path, const TextureLoadOptions& options);

    /**
     * Load texture from memory buffer.
     * Useful for procedurally generated textures or embedded resources.
     *
     * @param name Unique name for caching
     * @param data Pointer to image file data (PNG, JPG, etc.)
     * @param size Size of data in bytes
     * @param options Loading options
     * @return Texture handle, or nullptr on failure
     */
    TextureHandle loadFromMemory(const std::string& name,
                                  const void* data, std::size_t size,
                                  const TextureLoadOptions& options = {});

    /**
     * Create texture from raw RGBA pixel data.
     *
     * @param name Unique name for caching
     * @param pixels RGBA8 pixel data
     * @param width Texture width
     * @param height Texture height
     * @param options Loading options
     * @return Texture handle, or nullptr on failure
     */
    TextureHandle createFromPixels(const std::string& name,
                                    const void* pixels, int width, int height,
                                    const TextureLoadOptions& options = {});

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

    /**
     * Get the last error message.
     * @return Error string from last failed operation
     */
    const std::string& getLastError() const { return m_lastError; }

private:
    /**
     * Load image data using stb_image.
     * @param path File path
     * @param outWidth Receives width
     * @param outHeight Receives height
     * @param outChannels Receives channel count
     * @return Pointer to RGBA data (caller must free with stbi_image_free)
     */
    unsigned char* loadImageFile(const std::string& path,
                                  int* outWidth, int* outHeight, int* outChannels);

    /**
     * Load image from memory using stb_image.
     * @param data Pointer to image file data
     * @param size Size of data in bytes
     * @param outWidth Receives width
     * @param outHeight Receives height
     * @param outChannels Receives channel count
     * @return Pointer to RGBA data (caller must free with stbi_image_free)
     */
    unsigned char* loadImageMemory(const void* data, std::size_t size,
                                    int* outWidth, int* outHeight, int* outChannels);

    /**
     * Create GPU texture from raw RGBA pixel data.
     */
    SDL_GPUTexture* createGPUTextureFromPixels(const void* pixels,
                                                int width, int height);

    SDL_GPUTexture* createGPUTexture(SDL_Surface* surface);
    SDL_GPUSampler* createSampler(TextureFilter filter);

    Window& m_window;
    std::unordered_map<std::string, Texture> m_cache;
    SDL_GPUSampler* m_linearSampler = nullptr;
    SDL_GPUSampler* m_nearestSampler = nullptr;
    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_ASSETS_TEXTURELOADER_H
