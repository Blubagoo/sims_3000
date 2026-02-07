/**
 * @file EdgeDetectionPass.h
 * @brief Screen-space Sobel edge detection post-process for cartoon outlines.
 *
 * Implements edge detection using normal-based edges as the primary signal
 * and linearized depth as a secondary signal. Works correctly with
 * perspective projection by linearizing the non-linear depth buffer.
 *
 * Pipeline stages:
 * 1. Sample normal buffer (primary edge signal)
 * 2. Sample and linearize depth buffer (secondary edge signal)
 * 3. Apply Sobel filter to detect edges
 * 4. Blend outline color with scene color
 *
 * Performance: <1ms at 1080p (target: 0.5-1ms)
 *
 * Resource ownership:
 * - EdgeDetectionPass owns pipeline and sampler resources
 * - EdgeDetectionPass does NOT own input textures (scene, normal, depth)
 * - GPUDevice must outlive EdgeDetectionPass
 *
 * Usage:
 * @code
 *   EdgeDetectionPass edgePass(device, swapchainFormat);
 *
 *   // Configure (optional)
 *   EdgeDetectionConfig config;
 *   config.outlineColor = glm::vec4(0.165f, 0.106f, 0.239f, 1.0f); // Dark purple
 *   config.edgeThickness = 1.5f;
 *   edgePass.setConfig(config);
 *
 *   // In render loop after scene render, before bloom:
 *   edgePass.execute(cmdBuffer, sceneTexture, normalBuffer, depthBuffer, outputTexture);
 * @endcode
 */

#ifndef SIMS3000_RENDER_EDGE_DETECTION_PASS_H
#define SIMS3000_RENDER_EDGE_DETECTION_PASS_H

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <string>

namespace sims3000 {

// Forward declarations
class GPUDevice;

/**
 * @struct EdgeDetectionConfig
 * @brief Configuration parameters for edge detection.
 */
struct EdgeDetectionConfig {
    /// Outline color (default: dark purple #2A1B3D per canon)
    glm::vec4 outlineColor{0.165f, 0.106f, 0.239f, 1.0f};

    /// Threshold for normal discontinuity detection [0.0, 1.0]
    /// Lower values = more sensitive to normal changes
    float normalThreshold = 0.3f;

    /// Threshold for depth discontinuity detection [0.0, 1.0]
    /// Lower values = more sensitive to depth changes
    float depthThreshold = 0.1f;

    /// Edge thickness in screen-space pixels [0.5, 3.0]
    float edgeThickness = 1.0f;

    /// Near plane distance (for depth linearization)
    float nearPlane = 0.1f;

    /// Far plane distance (for depth linearization)
    float farPlane = 1000.0f;
};

/**
 * @struct EdgeDetectionStats
 * @brief Statistics about edge detection pass execution.
 */
struct EdgeDetectionStats {
    float executionTimeMs = 0.0f;   ///< Time for edge detection pass
    std::uint32_t width = 0;        ///< Width of processed texture
    std::uint32_t height = 0;       ///< Height of processed texture
};

/**
 * @struct EdgeDetectionUBO
 * @brief Uniform buffer data for edge detection shader.
 *
 * Matches the cbuffer layout in edge_detect.frag.hlsl.
 */
struct EdgeDetectionUBO {
    glm::vec4 outlineColor;      // 16 bytes: Outline color (RGB) + alpha multiplier
    glm::vec2 texelSize;         //  8 bytes: 1.0 / textureSize
    float normalThreshold;       //  4 bytes: Threshold for normal discontinuities
    float depthThreshold;        //  4 bytes: Threshold for depth discontinuities
    float nearPlane;             //  4 bytes: Camera near plane
    float farPlane;              //  4 bytes: Camera far plane
    float edgeThickness;         //  4 bytes: Edge thickness in pixels
    float _padding;              //  4 bytes: Align to 16 bytes
};
static_assert(sizeof(EdgeDetectionUBO) == 48, "EdgeDetectionUBO must be 48 bytes");

/**
 * @class EdgeDetectionPass
 * @brief Screen-space Sobel edge detection for cartoon outlines.
 *
 * Detects edges using normal and depth buffers, applies configurable
 * outline color. Executes as a fullscreen post-process pass.
 */
class EdgeDetectionPass {
public:
    /**
     * Create edge detection pass.
     * @param device GPU device for resource creation
     * @param colorFormat Format of the color render target
     */
    EdgeDetectionPass(GPUDevice& device, SDL_GPUTextureFormat colorFormat);

    ~EdgeDetectionPass();

    // Non-copyable
    EdgeDetectionPass(const EdgeDetectionPass&) = delete;
    EdgeDetectionPass& operator=(const EdgeDetectionPass&) = delete;

    // Movable
    EdgeDetectionPass(EdgeDetectionPass&& other) noexcept;
    EdgeDetectionPass& operator=(EdgeDetectionPass&& other) noexcept;

    /**
     * Check if edge detection pass is valid and ready to use.
     * @return true if resources were created successfully
     */
    bool isValid() const;

    /**
     * Execute the edge detection pass.
     *
     * Reads from scene color, normal buffer, and depth buffer.
     * Writes edge-detected result to output texture.
     *
     * @param cmdBuffer Command buffer for recording
     * @param sceneTexture Scene color texture to read
     * @param normalTexture View-space normal buffer
     * @param depthTexture Depth buffer
     * @param outputTexture Output texture (may be same as scene for in-place)
     * @param width Texture width
     * @param height Texture height
     * @return true if execution succeeded
     */
    bool execute(
        SDL_GPUCommandBuffer* cmdBuffer,
        SDL_GPUTexture* sceneTexture,
        SDL_GPUTexture* normalTexture,
        SDL_GPUTexture* depthTexture,
        SDL_GPUTexture* outputTexture,
        std::uint32_t width,
        std::uint32_t height);

    /**
     * Get current edge detection configuration.
     * @return Current configuration
     */
    const EdgeDetectionConfig& getConfig() const { return m_config; }

    /**
     * Set edge detection configuration.
     * Takes effect on next execute().
     *
     * @param config New configuration
     */
    void setConfig(const EdgeDetectionConfig& config);

    /**
     * Set outline color.
     * @param color New outline color (RGBA)
     */
    void setOutlineColor(const glm::vec4& color);

    /**
     * Set edge thickness.
     * @param thickness Thickness in screen-space pixels [0.5, 3.0]
     */
    void setEdgeThickness(float thickness);

    /**
     * Set camera near/far planes for depth linearization.
     * Must match the camera projection settings.
     *
     * @param nearPlane Near clipping plane distance
     * @param farPlane Far clipping plane distance
     */
    void setCameraPlanes(float nearPlane, float farPlane);

    /**
     * Set normal edge detection threshold.
     * @param threshold Threshold [0.0, 1.0] - lower = more sensitive
     */
    void setNormalThreshold(float threshold);

    /**
     * Set depth edge detection threshold.
     * @param threshold Threshold [0.0, 1.0] - lower = more sensitive
     */
    void setDepthThreshold(float threshold);

    /**
     * Get execution statistics from last execute() call.
     * @return Edge detection statistics
     */
    const EdgeDetectionStats& getStats() const { return m_stats; }

    /**
     * Get the last error message.
     * @return Error string
     */
    const std::string& getLastError() const { return m_lastError; }

private:
    /**
     * Create pipeline and resources.
     * @return true if creation succeeded
     */
    bool createResources();

    /**
     * Release all resources.
     */
    void releaseResources();

    /**
     * Load and compile shaders.
     * @return true if shaders loaded successfully
     */
    bool loadShaders();

    GPUDevice* m_device = nullptr;
    SDL_GPUTextureFormat m_colorFormat = SDL_GPU_TEXTUREFORMAT_INVALID;

    // Configuration
    EdgeDetectionConfig m_config;

    // Pipeline and shaders
    SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
    SDL_GPUShader* m_vertexShader = nullptr;
    SDL_GPUShader* m_fragmentShader = nullptr;

    // Sampler for texture reads (point sampling for accurate edge detection)
    SDL_GPUSampler* m_pointSampler = nullptr;

    // Statistics
    EdgeDetectionStats m_stats;

    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_EDGE_DETECTION_PASS_H
