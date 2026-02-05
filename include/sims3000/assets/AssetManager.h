/**
 * @file AssetManager.h
 * @brief Central asset loading and caching.
 */

#ifndef SIMS3000_ASSETS_ASSETMANAGER_H
#define SIMS3000_ASSETS_ASSETMANAGER_H

#include "sims3000/assets/TextureLoader.h"
#include "sims3000/assets/ModelLoader.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace sims3000 {

// Forward declarations
class Window;

/**
 * @class AssetManager
 * @brief Manages loading and caching of game assets.
 *
 * Provides unified access to textures, models, and other
 * resources. Handles reference counting and cache eviction.
 */
class AssetManager {
public:
    /**
     * Create asset manager.
     * @param window Window for GPU resource creation
     * @param assetBasePath Base path for asset files
     */
    AssetManager(Window& window, const std::string& assetBasePath = "assets");
    ~AssetManager();

    // Non-copyable
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    /**
     * Load a texture.
     * @param path Relative path from asset base
     * @return Texture handle, or null on failure
     */
    TextureHandle loadTexture(const std::string& path);

    /**
     * Load a model.
     * @param path Relative path from asset base
     * @return Model handle, or null on failure
     */
    ModelHandle loadModel(const std::string& path);

    /**
     * Get the fallback texture (magenta checkerboard).
     */
    TextureHandle getFallbackTexture();

    /**
     * Get the fallback model (unit cube).
     */
    ModelHandle getFallbackModel();

    /**
     * Release a texture.
     * @param handle Texture to release
     */
    void releaseTexture(TextureHandle handle);

    /**
     * Release a model.
     * @param handle Model to release
     */
    void releaseModel(ModelHandle handle);

    /**
     * Clear unused cached assets.
     * Removes assets with zero references.
     */
    void clearUnused();

    /**
     * Clear all cached assets.
     * Warning: invalidates all handles.
     */
    void clearAll();

    /**
     * Get cache statistics.
     * @param outTextureCount Number of cached textures
     * @param outModelCount Number of cached models
     * @param outTextureBytes Total texture memory
     * @param outModelBytes Total model memory
     */
    void getCacheStats(size_t& outTextureCount, size_t& outModelCount,
                       size_t& outTextureBytes, size_t& outModelBytes) const;

    /**
     * Enable/disable hot reload in debug builds.
     * When enabled, modified files are automatically reloaded.
     */
    void setHotReloadEnabled(bool enabled);

    /**
     * Check for modified files and reload.
     * Call once per frame when hot reload is enabled.
     */
    void checkHotReload();

    /**
     * Get the asset base path.
     */
    const std::string& getBasePath() const;

    /**
     * Get the texture loader.
     */
    TextureLoader& getTextureLoader();

    /**
     * Get the model loader.
     */
    ModelLoader& getModelLoader();

private:
    Window& m_window;
    std::string m_basePath;
    bool m_hotReloadEnabled = false;

    std::unique_ptr<TextureLoader> m_textureLoader;
    std::unique_ptr<ModelLoader> m_modelLoader;

    TextureHandle m_fallbackTexture;
    ModelHandle m_fallbackModel;
};

} // namespace sims3000

#endif // SIMS3000_ASSETS_ASSETMANAGER_H
