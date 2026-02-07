/**
 * @file ShaderCompiler.cpp
 * @brief Shader compilation pipeline implementation.
 */

#include "sims3000/render/ShaderCompiler.h"
#include "sims3000/render/GPUDevice.h"
#include <SDL3/SDL_log.h>
#include <algorithm>
#include <fstream>

namespace sims3000 {

// =============================================================================
// External declarations for embedded shaders (generated at build time)
// =============================================================================

// These functions are defined in the generated EmbeddedShaders.cpp
extern const uint8_t* getEmbeddedFallbackVertexDXIL(size_t& outSize);
extern const uint8_t* getEmbeddedFallbackVertexSPIRV(size_t& outSize);
extern const uint8_t* getEmbeddedFallbackFragmentDXIL(size_t& outSize);
extern const uint8_t* getEmbeddedFallbackFragmentSPIRV(size_t& outSize);

// =============================================================================
// Helper Functions
// =============================================================================

SDL_GPUShaderStage toSDLShaderStage(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Vertex:   return SDL_GPU_SHADERSTAGE_VERTEX;
        case ShaderStage::Fragment: return SDL_GPU_SHADERSTAGE_FRAGMENT;
        default:                    return SDL_GPU_SHADERSTAGE_VERTEX;
    }
}

const char* getShaderProfile(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Vertex:   return "vs_6_0";
        case ShaderStage::Fragment: return "ps_6_0";
        default:                    return "vs_6_0";
    }
}

static const char* getShaderStageName(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Vertex:   return "vertex";
        case ShaderStage::Fragment: return "fragment";
        default:                    return "unknown";
    }
}

// Simple FNV-1a hash for cache validation
static uint32_t fnv1aHash(const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

// =============================================================================
// ShaderCompiler Implementation
// =============================================================================

ShaderCompiler::ShaderCompiler(GPUDevice& device)
    : m_device(&device)
{
#ifdef SIMS3000_DEBUG
    m_hotReloadEnabled = true;
#endif
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: Initialized (hot-reload=%s)",
                m_hotReloadEnabled ? "enabled" : "disabled");
}

ShaderCompiler::~ShaderCompiler() {
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: Destroyed");
}

ShaderCompiler::ShaderCompiler(ShaderCompiler&& other) noexcept
    : m_device(other.m_device)
    , m_assetPath(std::move(other.m_assetPath))
    , m_cachePath(std::move(other.m_cachePath))
    , m_hotReloadEnabled(other.m_hotReloadEnabled)
    , m_reloadCallback(std::move(other.m_reloadCallback))
    , m_watchedShaders(std::move(other.m_watchedShaders))
    , m_memoryCache(std::move(other.m_memoryCache))
{
    other.m_device = nullptr;
}

ShaderCompiler& ShaderCompiler::operator=(ShaderCompiler&& other) noexcept {
    if (this != &other) {
        m_device = other.m_device;
        m_assetPath = std::move(other.m_assetPath);
        m_cachePath = std::move(other.m_cachePath);
        m_hotReloadEnabled = other.m_hotReloadEnabled;
        m_reloadCallback = std::move(other.m_reloadCallback);
        m_watchedShaders = std::move(other.m_watchedShaders);
        m_memoryCache = std::move(other.m_memoryCache);
        other.m_device = nullptr;
    }
    return *this;
}

void ShaderCompiler::setAssetPath(const std::string& path) {
    m_assetPath = path;
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: Asset path set to '%s'", path.c_str());
}

void ShaderCompiler::setCachePath(const std::string& path) {
    m_cachePath = path;
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: Cache path set to '%s'", path.c_str());
}

void ShaderCompiler::setHotReloadEnabled(bool enable) {
#ifdef SIMS3000_DEBUG
    m_hotReloadEnabled = enable;
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: Hot-reload %s",
                enable ? "enabled" : "disabled");
#else
    (void)enable;
    SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                "ShaderCompiler: Hot-reload only available in debug builds");
#endif
}

void ShaderCompiler::setReloadCallback(ReloadCallback callback) {
    m_reloadCallback = std::move(callback);
}

SDL_GPUShaderFormat ShaderCompiler::getPreferredFormat() const {
    if (!m_device || !m_device->isValid()) {
        return SDL_GPU_SHADERFORMAT_INVALID;
    }

    const auto& caps = m_device->getCapabilities();

    // Prefer DXIL for D3D12, SPIR-V for Vulkan/Metal
    if (caps.backend == GPUBackend::D3D12 && caps.supportsDXIL) {
        return SDL_GPU_SHADERFORMAT_DXIL;
    }
    if (caps.supportsSpirV) {
        return SDL_GPU_SHADERFORMAT_SPIRV;
    }
    if (caps.supportsDXIL) {
        return SDL_GPU_SHADERFORMAT_DXIL;
    }

    return SDL_GPU_SHADERFORMAT_INVALID;
}

const char* ShaderCompiler::getFormatExtension() const {
    SDL_GPUShaderFormat format = getPreferredFormat();
    switch (format) {
        case SDL_GPU_SHADERFORMAT_DXIL:  return ".dxil";
        case SDL_GPU_SHADERFORMAT_SPIRV: return ".spv";
        default:                         return "";
    }
}

const char* ShaderCompiler::getFormatName() const {
    SDL_GPUShaderFormat format = getPreferredFormat();
    switch (format) {
        case SDL_GPU_SHADERFORMAT_DXIL:  return "DXIL";
        case SDL_GPU_SHADERFORMAT_SPIRV: return "SPIRV";
        default:                         return "Unknown";
    }
}

bool ShaderCompiler::isHotReloadEnabled() const {
    return m_hotReloadEnabled;
}

ShaderLoadResult ShaderCompiler::loadShader(
    const std::string& basePath,
    ShaderStage stage,
    const char* entryPoint,
    const ShaderResources& resources)
{
    ShaderLoadResult result;
    result.loadedPath = basePath;

    if (!m_device || !m_device->isValid()) {
        result.error.message = "GPU device not valid";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: %s", result.error.message.c_str());
        return result;
    }

    SDL_GPUShaderFormat format = getPreferredFormat();
    if (format == SDL_GPU_SHADERFORMAT_INVALID) {
        result.error.message = "No supported shader format available";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: %s", result.error.message.c_str());
        return result;
    }

    std::vector<uint8_t> bytecode;

    // Priority 1: Try cache
    ShaderCacheEntry cacheEntry;
    if (loadFromCache(basePath, cacheEntry)) {
        if (validateCacheEntry(basePath, cacheEntry)) {
            result.shader = createShaderFromBytecode(
                cacheEntry.bytecode, stage, entryPoint, resources, format);
            if (result.shader) {
                result.fromCache = true;
                SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: Loaded %s from cache",
                            basePath.c_str());

                if (m_hotReloadEnabled) {
                    watchShader(basePath);
                }
                return result;
            }
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                        "ShaderCompiler: Cache invalid for %s, reloading", basePath.c_str());
            invalidateCache(basePath);
        }
    }

    // Priority 2: Try pre-compiled assets
    if (loadFromAssets(basePath, bytecode)) {
        result.shader = createShaderFromBytecode(
            bytecode, stage, entryPoint, resources, format);
        if (result.shader) {
            SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: Loaded %s from assets (%s)",
                        basePath.c_str(), getFormatName());

            // Cache the loaded shader
            ShaderCacheEntry newEntry;
            newEntry.bytecode = bytecode;
            newEntry.format = format;
            newEntry.sourceHash = calculateSourceHash(basePath + ".hlsl");
            newEntry.timestamp = getFileTimestamp(basePath + getFormatExtension());
            saveToCache(basePath, newEntry);

            if (m_hotReloadEnabled) {
                watchShader(basePath);
            }
            return result;
        }
    }

    // Priority 3: Use embedded fallback
    SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                "ShaderCompiler: Failed to load %s, using fallback shader", basePath.c_str());

    size_t fallbackSize = 0;
    const uint8_t* fallbackData = nullptr;

    if (stage == ShaderStage::Vertex) {
        if (format == SDL_GPU_SHADERFORMAT_DXIL) {
            fallbackData = getEmbeddedFallbackVertexDXIL(fallbackSize);
        } else {
            fallbackData = getEmbeddedFallbackVertexSPIRV(fallbackSize);
        }
    } else {
        if (format == SDL_GPU_SHADERFORMAT_DXIL) {
            fallbackData = getEmbeddedFallbackFragmentDXIL(fallbackSize);
        } else {
            fallbackData = getEmbeddedFallbackFragmentSPIRV(fallbackSize);
        }
    }

    if (fallbackData && fallbackSize > 0) {
        std::vector<uint8_t> fallbackBytecode(fallbackData, fallbackData + fallbackSize);
        result.shader = createShaderFromBytecode(
            fallbackBytecode, stage, entryPoint, resources, format);
        if (result.shader) {
            result.usedFallback = true;
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                        "ShaderCompiler: Using embedded fallback for %s %s shader",
                        basePath.c_str(), getShaderStageName(stage));
            return result;
        }
    }

    result.error.message = "Failed to load shader and fallback not available";
    result.error.filename = basePath;
    SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: %s", result.error.message.c_str());
    return result;
}

bool ShaderCompiler::checkForReload() {
    if (!m_hotReloadEnabled) {
        return false;
    }

    bool anyReloaded = false;

    for (auto& pair : m_watchedShaders) {
        WatchedShader& watched = pair.second;
        std::string shaderPath = m_assetPath + "/" + watched.basePath + getFormatExtension();
        uint64_t currentTimestamp = getFileTimestamp(shaderPath);

        if (currentTimestamp > watched.lastTimestamp && watched.lastTimestamp > 0) {
            SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: Detected change in %s",
                        watched.basePath.c_str());

            // Invalidate cache for this shader
            invalidateCache(watched.basePath);

            // Update timestamp
            watched.lastTimestamp = currentTimestamp;

            // Notify callback
            if (m_reloadCallback) {
                m_reloadCallback(watched.basePath);
            }

            anyReloaded = true;
        }
    }

    return anyReloaded;
}

void ShaderCompiler::invalidateCache(const std::string& basePath) {
    m_memoryCache.erase(basePath);
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: Invalidated cache for %s",
                basePath.c_str());
}

void ShaderCompiler::clearCache() {
    m_memoryCache.clear();
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: Cleared all cached shaders");
}

// =============================================================================
// Private Implementation
// =============================================================================

bool ShaderCompiler::loadBytecodeFromFile(const std::string& path, std::vector<uint8_t>& outBytecode) {
    SDL_IOStream* file = SDL_IOFromFile(path.c_str(), "rb");
    if (!file) {
        return false;
    }

    Sint64 size = SDL_GetIOSize(file);
    if (size <= 0) {
        SDL_CloseIO(file);
        return false;
    }

    outBytecode.resize(static_cast<size_t>(size));
    size_t bytesRead = SDL_ReadIO(file, outBytecode.data(), static_cast<size_t>(size));
    SDL_CloseIO(file);

    if (bytesRead != static_cast<size_t>(size)) {
        outBytecode.clear();
        return false;
    }

    return true;
}

SDL_GPUShader* ShaderCompiler::createShaderFromBytecode(
    const std::vector<uint8_t>& bytecode,
    ShaderStage stage,
    const char* entryPoint,
    const ShaderResources& resources,
    SDL_GPUShaderFormat format)
{
    if (bytecode.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: Empty bytecode");
        return nullptr;
    }

    SDL_GPUShaderCreateInfo shaderInfo = {};
    shaderInfo.code = bytecode.data();
    shaderInfo.code_size = bytecode.size();
    shaderInfo.entrypoint = entryPoint;
    shaderInfo.format = format;
    shaderInfo.stage = toSDLShaderStage(stage);
    shaderInfo.num_samplers = resources.numSamplers;
    shaderInfo.num_storage_textures = resources.numStorageTextures;
    shaderInfo.num_storage_buffers = resources.numStorageBuffers;
    shaderInfo.num_uniform_buffers = resources.numUniformBuffers;

    SDL_GPUShader* shader = SDL_CreateGPUShader(m_device->getHandle(), &shaderInfo);
    if (!shader) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderCompiler: Failed to create shader: %s",
                     SDL_GetError());
        return nullptr;
    }

    return shader;
}

bool ShaderCompiler::loadFromAssets(const std::string& basePath, std::vector<uint8_t>& outBytecode) {
    std::string fullPath = m_assetPath + "/" + basePath + getFormatExtension();
    return loadBytecodeFromFile(fullPath, outBytecode);
}

bool ShaderCompiler::loadFromCache(const std::string& basePath, ShaderCacheEntry& outEntry) {
    auto it = m_memoryCache.find(basePath);
    if (it != m_memoryCache.end()) {
        outEntry = it->second;
        return true;
    }
    return false;
}

bool ShaderCompiler::saveToCache(const std::string& basePath, const ShaderCacheEntry& entry) {
    m_memoryCache[basePath] = entry;
    return true;
}

bool ShaderCompiler::validateCacheEntry(const std::string& basePath, const ShaderCacheEntry& entry) {
    // Check if format still matches
    if (entry.format != getPreferredFormat()) {
        return false;
    }

    // Check if bytecode is non-empty
    if (entry.bytecode.empty()) {
        return false;
    }

    // In debug builds, also check source hash if HLSL file exists
#ifdef SIMS3000_DEBUG
    std::string hlslPath = m_assetPath + "/" + basePath + ".hlsl";
    uint32_t currentHash = calculateSourceHash(hlslPath);
    if (currentHash != 0 && entry.sourceHash != 0 && currentHash != entry.sourceHash) {
        return false;
    }
#endif

    return true;
}

uint32_t ShaderCompiler::calculateSourceHash(const std::string& path) {
    std::vector<uint8_t> content;
    if (!loadBytecodeFromFile(path, content)) {
        return 0;
    }
    return fnv1aHash(content.data(), content.size());
}

uint64_t ShaderCompiler::getFileTimestamp(const std::string& path) {
    SDL_IOStream* file = SDL_IOFromFile(path.c_str(), "rb");
    if (!file) {
        return 0;
    }

    // Get file modification time via SDL
    // Note: SDL3 doesn't have direct file timestamp API, so we use file size as proxy
    // In a production system, you'd use platform-specific APIs
    Sint64 size = SDL_GetIOSize(file);
    SDL_CloseIO(file);

    // Use file size as a simple change indicator (not ideal but works for hot-reload)
    // A proper implementation would use std::filesystem or platform APIs
    return static_cast<uint64_t>(size);
}

void ShaderCompiler::watchShader(const std::string& basePath) {
    if (!m_hotReloadEnabled) {
        return;
    }

    std::string shaderPath = m_assetPath + "/" + basePath + getFormatExtension();
    uint64_t timestamp = getFileTimestamp(shaderPath);

    WatchedShader watched;
    watched.basePath = basePath;
    watched.lastTimestamp = timestamp;

    m_watchedShaders[basePath] = watched;
}

// =============================================================================
// Fallback Shader Accessors
// =============================================================================

const uint8_t* ShaderCompiler::getFallbackVertexShader(size_t& outSize) {
    // Prefer DXIL on Windows, SPIR-V elsewhere
#ifdef _WIN32
    return getEmbeddedFallbackVertexDXIL(outSize);
#else
    return getEmbeddedFallbackVertexSPIRV(outSize);
#endif
}

const uint8_t* ShaderCompiler::getFallbackFragmentShader(size_t& outSize) {
#ifdef _WIN32
    return getEmbeddedFallbackFragmentDXIL(outSize);
#else
    return getEmbeddedFallbackFragmentSPIRV(outSize);
#endif
}

} // namespace sims3000
