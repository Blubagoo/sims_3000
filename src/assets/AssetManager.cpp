/**
 * @file AssetManager.cpp
 * @brief AssetManager implementation.
 */

#include "sims3000/assets/AssetManager.h"
#include "sims3000/render/Window.h"
#include <SDL3/SDL_log.h>

namespace sims3000 {

AssetManager::AssetManager(Window& window, const std::string& assetBasePath)
    : m_window(window)
    , m_basePath(assetBasePath)
{
    m_textureLoader = std::make_unique<TextureLoader>(window);
    m_modelLoader = std::make_unique<ModelLoader>(window);

    // Create fallback assets
    m_fallbackTexture = m_textureLoader->createFallback();
    m_fallbackModel = m_modelLoader->createFallback();

    SDL_Log("AssetManager initialized with base path: %s", m_basePath.c_str());
}

AssetManager::~AssetManager() {
    clearAll();
}

TextureHandle AssetManager::loadTexture(const std::string& path) {
    std::string fullPath = m_basePath + "/" + path;
    TextureHandle handle = m_textureLoader->load(fullPath);

    if (!handle) {
        SDL_Log("Using fallback texture for: %s", path.c_str());
        return m_fallbackTexture;
    }

    return handle;
}

ModelHandle AssetManager::loadModel(const std::string& path) {
    std::string fullPath = m_basePath + "/" + path;
    ModelHandle handle = m_modelLoader->load(fullPath);

    if (!handle) {
        SDL_Log("Using fallback model for: %s", path.c_str());
        return m_fallbackModel;
    }

    return handle;
}

TextureHandle AssetManager::getFallbackTexture() {
    return m_fallbackTexture;
}

ModelHandle AssetManager::getFallbackModel() {
    return m_fallbackModel;
}

void AssetManager::releaseTexture(TextureHandle handle) {
    if (handle && handle != m_fallbackTexture) {
        m_textureLoader->release(handle);
    }
}

void AssetManager::releaseModel(ModelHandle handle) {
    if (handle && handle != m_fallbackModel) {
        m_modelLoader->release(handle);
    }
}

void AssetManager::clearUnused() {
    m_textureLoader->clearUnused();
    m_modelLoader->clearUnused();
}

void AssetManager::clearAll() {
    m_textureLoader->clearAll();
    m_modelLoader->clearAll();
    m_fallbackTexture = nullptr;
    m_fallbackModel = nullptr;
}

void AssetManager::getCacheStats(size_t& outTextureCount, size_t& outModelCount,
                                  size_t& outTextureBytes, size_t& outModelBytes) const {
    m_textureLoader->getStats(outTextureCount, outTextureBytes);
    m_modelLoader->getStats(outModelCount, outModelBytes);
}

void AssetManager::setHotReloadEnabled(bool enabled) {
#ifdef SIMS3000_DEBUG
    m_hotReloadEnabled = enabled;
    SDL_Log("Hot reload %s", enabled ? "enabled" : "disabled");
#else
    (void)enabled;
#endif
}

void AssetManager::checkHotReload() {
#ifdef SIMS3000_DEBUG
    if (!m_hotReloadEnabled) return;

    // Check textures and models for modifications
    // This is called once per frame, so we could rate-limit this
    // For now, we leave it to the loaders to handle efficiently
#endif
}

const std::string& AssetManager::getBasePath() const {
    return m_basePath;
}

TextureLoader& AssetManager::getTextureLoader() {
    return *m_textureLoader;
}

ModelLoader& AssetManager::getModelLoader() {
    return *m_modelLoader;
}

} // namespace sims3000
