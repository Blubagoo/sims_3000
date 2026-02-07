/**
 * @file ShaderCompiler.h
 * @brief Shader compilation pipeline with hot-reload and fallback support.
 *
 * Provides HLSL shader loading, compilation to SPIR-V/DXIL, caching,
 * hot-reload during development, and embedded fallback shaders for
 * graceful degradation when shader loading fails.
 *
 * Loading Priority:
 * 1. User cache (compiled bytecode with hash validation)
 * 2. Pre-compiled assets (shipped with game)
 * 3. Embedded fallback (compiled into executable)
 *
 * Resource ownership:
 * - ShaderCompiler owns cached bytecode and file watchers
 * - Created SDL_GPUShader objects are owned by caller
 * - GPUDevice must outlive ShaderCompiler
 */

#ifndef SIMS3000_RENDER_SHADER_COMPILER_H
#define SIMS3000_RENDER_SHADER_COMPILER_H

#include <SDL3/SDL.h>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace sims3000 {

// Forward declaration
class GPUDevice;

/**
 * @enum ShaderStage
 * @brief Shader pipeline stage.
 */
enum class ShaderStage {
    Vertex,
    Fragment
};

/**
 * @struct ShaderResources
 * @brief Resource bindings declared by a shader.
 */
struct ShaderResources {
    uint32_t numSamplers = 0;
    uint32_t numStorageTextures = 0;
    uint32_t numStorageBuffers = 0;
    uint32_t numUniformBuffers = 0;
};

/**
 * @struct ShaderCompileError
 * @brief Detailed shader compilation error information.
 */
struct ShaderCompileError {
    std::string filename;
    int line = 0;
    int column = 0;
    std::string message;
    std::string fullText;  // Complete error output
};

/**
 * @struct ShaderLoadResult
 * @brief Result of shader loading operation.
 */
struct ShaderLoadResult {
    SDL_GPUShader* shader = nullptr;
    bool usedFallback = false;
    bool fromCache = false;
    std::string loadedPath;
    ShaderCompileError error;

    bool isValid() const { return shader != nullptr; }
    bool hasError() const { return !error.message.empty(); }
};

/**
 * @struct ShaderCacheEntry
 * @brief Cached shader bytecode with validation metadata.
 */
struct ShaderCacheEntry {
    std::vector<uint8_t> bytecode;
    uint32_t sourceHash = 0;
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    uint64_t timestamp = 0;
};

/**
 * @class ShaderCompiler
 * @brief Shader compilation and loading system.
 *
 * Manages the complete shader lifecycle:
 * - Loading pre-compiled shaders from disk
 * - Detecting backend-appropriate format (SPIR-V or DXIL)
 * - Validating cached shaders against source hashes
 * - Hot-reloading shaders when source files change (debug builds)
 * - Falling back to embedded shaders when loading fails
 *
 * Usage:
 * @code
 *   ShaderCompiler compiler(gpuDevice);
 *
 *   // Load a shader with fallback
 *   auto result = compiler.loadShader(
 *       "assets/shaders/toon.vert",
 *       ShaderStage::Vertex,
 *       "main",
 *       {.numUniformBuffers = 1}
 *   );
 *
 *   if (result.usedFallback) {
 *       SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Using fallback shader");
 *   }
 *
 *   // In debug builds, check for hot-reload
 *   if (compiler.checkForReload()) {
 *       // Shaders changed - rebuild pipelines
 *   }
 * @endcode
 */
class ShaderCompiler {
public:
    /**
     * Callback for shader reload notifications.
     * @param shaderPath Base path of the shader that changed
     */
    using ReloadCallback = std::function<void(const std::string& shaderPath)>;

    /**
     * Create shader compiler.
     * @param device GPUDevice for shader format detection and creation
     */
    explicit ShaderCompiler(GPUDevice& device);

    ~ShaderCompiler();

    // Non-copyable
    ShaderCompiler(const ShaderCompiler&) = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;

    // Movable
    ShaderCompiler(ShaderCompiler&& other) noexcept;
    ShaderCompiler& operator=(ShaderCompiler&& other) noexcept;

    /**
     * Set the shader asset directory.
     * Default is "assets/shaders".
     * @param path Directory containing pre-compiled shaders
     */
    void setAssetPath(const std::string& path);

    /**
     * Set the shader cache directory.
     * Default is "cache/shaders".
     * @param path Directory for cached compiled shaders
     */
    void setCachePath(const std::string& path);

    /**
     * Enable or disable hot-reload monitoring.
     * Only has effect in debug builds.
     * @param enable True to enable hot-reload
     */
    void setHotReloadEnabled(bool enable);

    /**
     * Register callback for shader reload notifications.
     * @param callback Function called when a shader is reloaded
     */
    void setReloadCallback(ReloadCallback callback);

    /**
     * Load a shader from disk with fallback support.
     *
     * Loading priority:
     * 1. Cache (if valid hash match)
     * 2. Pre-compiled assets
     * 3. Embedded fallback shader
     *
     * @param basePath Path without extension (e.g., "shaders/toon.vert")
     * @param stage Vertex or Fragment shader
     * @param entryPoint Shader entry point function name
     * @param resources Resource binding counts
     * @return ShaderLoadResult with shader or error info
     */
    ShaderLoadResult loadShader(
        const std::string& basePath,
        ShaderStage stage,
        const char* entryPoint = "main",
        const ShaderResources& resources = {}
    );

    /**
     * Check for shader file changes and trigger reloads.
     * Only effective when hot-reload is enabled.
     * @return True if any shaders were reloaded
     */
    bool checkForReload();

    /**
     * Invalidate cached shader, forcing reload on next load.
     * @param basePath Path to the shader to invalidate
     */
    void invalidateCache(const std::string& basePath);

    /**
     * Clear all cached shaders.
     */
    void clearCache();

    /**
     * Get the preferred shader format for the current backend.
     * @return DXIL for D3D12, SPIR-V for Vulkan/Metal
     */
    SDL_GPUShaderFormat getPreferredFormat() const;

    /**
     * Get file extension for the preferred format.
     * @return ".dxil" or ".spv"
     */
    const char* getFormatExtension() const;

    /**
     * Get human-readable format name.
     * @return "DXIL" or "SPIRV"
     */
    const char* getFormatName() const;

    /**
     * Check if hot-reload is currently enabled.
     * @return True if hot-reload monitoring is active
     */
    bool isHotReloadEnabled() const;

    /**
     * Get embedded fallback vertex shader bytecode.
     * @param outSize Size of returned bytecode
     * @return Pointer to embedded bytecode, or nullptr if not available
     */
    static const uint8_t* getFallbackVertexShader(size_t& outSize);

    /**
     * Get embedded fallback fragment shader bytecode.
     * @param outSize Size of returned bytecode
     * @return Pointer to embedded bytecode, or nullptr if not available
     */
    static const uint8_t* getFallbackFragmentShader(size_t& outSize);

private:
    /**
     * Load bytecode from a file.
     * @param path Full path to shader bytecode file
     * @param outBytecode Output vector for bytecode
     * @return True if successful
     */
    bool loadBytecodeFromFile(const std::string& path, std::vector<uint8_t>& outBytecode);

    /**
     * Create GPU shader from bytecode.
     * @param bytecode Compiled shader bytecode
     * @param stage Shader stage
     * @param entryPoint Entry point name
     * @param resources Resource bindings
     * @param format Shader format
     * @return Created shader or nullptr on failure
     */
    SDL_GPUShader* createShaderFromBytecode(
        const std::vector<uint8_t>& bytecode,
        ShaderStage stage,
        const char* entryPoint,
        const ShaderResources& resources,
        SDL_GPUShaderFormat format
    );

    /**
     * Load shader from pre-compiled assets.
     * @param basePath Base path without extension
     * @param outBytecode Output bytecode
     * @return True if found and loaded
     */
    bool loadFromAssets(const std::string& basePath, std::vector<uint8_t>& outBytecode);

    /**
     * Load shader from cache with validation.
     * @param basePath Base path without extension
     * @param outEntry Output cache entry
     * @return True if valid cache entry found
     */
    bool loadFromCache(const std::string& basePath, ShaderCacheEntry& outEntry);

    /**
     * Save shader to cache.
     * @param basePath Base path without extension
     * @param entry Cache entry to save
     * @return True if saved successfully
     */
    bool saveToCache(const std::string& basePath, const ShaderCacheEntry& entry);

    /**
     * Validate cache entry against source file hash.
     * @param basePath Base path without extension
     * @param entry Cache entry to validate
     * @return True if cache is valid
     */
    bool validateCacheEntry(const std::string& basePath, const ShaderCacheEntry& entry);

    /**
     * Calculate hash of source file for cache validation.
     * @param path Path to source file
     * @return Hash value, or 0 if file not found
     */
    uint32_t calculateSourceHash(const std::string& path);

    /**
     * Get file modification timestamp.
     * @param path Path to file
     * @return Timestamp or 0 if not found
     */
    uint64_t getFileTimestamp(const std::string& path);

    /**
     * Watch a shader file for changes.
     * @param basePath Base path of shader to watch
     */
    void watchShader(const std::string& basePath);

    GPUDevice* m_device = nullptr;
    std::string m_assetPath = "assets/shaders";
    std::string m_cachePath = "cache/shaders";
    bool m_hotReloadEnabled = false;
    ReloadCallback m_reloadCallback;

    // File watching for hot-reload
    struct WatchedShader {
        std::string basePath;
        uint64_t lastTimestamp = 0;
        ShaderStage stage = ShaderStage::Vertex;
    };
    std::unordered_map<std::string, WatchedShader> m_watchedShaders;

    // In-memory cache
    std::unordered_map<std::string, ShaderCacheEntry> m_memoryCache;
};

/**
 * Convert ShaderStage to SDL shader stage.
 * @param stage ShaderStage enum value
 * @return Corresponding SDL_GPUShaderStage
 */
SDL_GPUShaderStage toSDLShaderStage(ShaderStage stage);

/**
 * Get shader profile string for DXC compilation.
 * @param stage ShaderStage enum value
 * @return Profile string like "vs_6_0" or "ps_6_0"
 */
const char* getShaderProfile(ShaderStage stage);

} // namespace sims3000

#endif // SIMS3000_RENDER_SHADER_COMPILER_H
