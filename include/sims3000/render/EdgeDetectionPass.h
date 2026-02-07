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
 * @struct TerrainEdgeConfig
 * @brief Terrain-specific edge detection parameters.
 *
 * Tuned parameters for terrain rendering:
 * - Cliff edges: Bold outlines from strong normal discontinuities
 * - Water shorelines: Visible outlines at water/land transitions
 * - Gentle slopes: No excessive edge noise
 * - Terrain type boundaries: Visual separation via color (edge detection bonus)
 *
 * These values are tuned for terrain distances and camera angles typical
 * of city builder games (35-80 degree pitch, distance 5-250 units).
 */
struct TerrainEdgeConfig {
    /**
     * @brief Normal threshold for terrain.
     *
     * Lower than building threshold (0.3) to catch more subtle terrain features.
     * Cliffs naturally produce strong normal discontinuities (>0.5 gradient).
     * Water shorelines have moderate discontinuity where water meets land.
     *
     * Value: 0.15 catches cliffs and shorelines without noise on gentle slopes.
     */
    static constexpr float NORMAL_THRESHOLD = 0.15f;

    /**
     * @brief Depth threshold for terrain.
     *
     * Higher than building threshold (0.1) to avoid artifacts on gentle slopes.
     * Terrain has gradual depth changes; we only want silhouette edges.
     * At terrain distances (50-250 units), depth gradients are smaller.
     *
     * Value: 0.25 avoids slope noise while catching terrain silhouettes.
     */
    static constexpr float DEPTH_THRESHOLD = 0.25f;

    /**
     * @brief Edge thickness for terrain outlines.
     *
     * Slightly thicker than buildings (1.0) for visibility at distance.
     * Cliffs should have bold outlines; shorelines should be visible.
     *
     * Value: 1.5 provides good visibility without overwhelming detail.
     */
    static constexpr float EDGE_THICKNESS = 1.5f;

    /**
     * @brief Cliff normal discontinuity threshold.
     *
     * Cliffs are defined by steep normal changes (horizontal vs vertical).
     * A normal.y < 0.5 indicates a cliff face (>60 degree slope).
     * The edge detection naturally catches this as a normal discontinuity.
     *
     * Value: 0.5 - used by shader to identify cliff regions for bold edges.
     */
    static constexpr float CLIFF_NORMAL_Y_THRESHOLD = 0.5f;

    /**
     * @brief Minimum slope angle (radians) for edge detection suppression.
     *
     * Gentle slopes below this angle should not produce edge lines.
     * 0.35 radians = ~20 degrees, which covers typical rolling terrain.
     *
     * Value: Used to suppress depth edges on gentle slopes.
     */
    static constexpr float GENTLE_SLOPE_ANGLE = 0.35f;

    /**
     * @brief Edge weight multiplier for cliff edges.
     *
     * Cliff edges are multiplied by this factor for bolder appearance.
     * Applied when normal.y is below CLIFF_NORMAL_Y_THRESHOLD.
     *
     * Value: 1.5 makes cliff edges ~50% bolder than standard edges.
     */
    static constexpr float CLIFF_EDGE_WEIGHT = 1.5f;

    /**
     * @brief Edge weight multiplier for shoreline edges.
     *
     * Water/land boundaries receive this weight multiplier.
     * Detected by checking for water terrain type transitions.
     *
     * Value: 1.25 makes shorelines visible but not overwhelming.
     */
    static constexpr float SHORELINE_EDGE_WEIGHT = 1.25f;

    /**
     * @brief Depth linearization scale factor for terrain distances.
     *
     * Terrain is viewed at greater distances than buildings.
     * This scales the depth threshold based on camera distance.
     *
     * Value: 0.8 reduces depth sensitivity at far distances.
     */
    static constexpr float DISTANCE_SCALE_FACTOR = 0.8f;

    /**
     * @brief Create EdgeDetectionConfig with terrain-tuned values.
     *
     * @param outlineColor Edge outline color (default: dark purple)
     * @param nearPlane Camera near plane distance
     * @param farPlane Camera far plane distance
     * @return EdgeDetectionConfig tuned for terrain
     */
    static EdgeDetectionConfig createConfig(
        const glm::vec4& outlineColor = glm::vec4(0.165f, 0.106f, 0.239f, 1.0f),
        float nearPlane = 0.1f,
        float farPlane = 1000.0f)
    {
        EdgeDetectionConfig config;
        config.outlineColor = outlineColor;
        config.normalThreshold = NORMAL_THRESHOLD;
        config.depthThreshold = DEPTH_THRESHOLD;
        config.edgeThickness = EDGE_THICKNESS;
        config.nearPlane = nearPlane;
        config.farPlane = farPlane;
        return config;
    }

    /**
     * @brief Get terrain config values as a struct for reference.
     * @return EdgeDetectionConfig with terrain values
     */
    static EdgeDetectionConfig getDefaults()
    {
        return createConfig();
    }
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
     * Apply terrain-specific edge detection configuration.
     *
     * Configures edge detection for terrain rendering:
     * - Lower normal threshold for cliffs and shorelines
     * - Higher depth threshold to avoid gentle slope noise
     * - Thicker edges for visibility at distance
     *
     * Call this before rendering terrain, or use setConfig() to switch
     * back to building configuration.
     */
    void applyTerrainConfig();

    /**
     * Apply building/default edge detection configuration.
     *
     * Restores edge detection to default values suitable for buildings
     * and other non-terrain geometry.
     */
    void applyBuildingConfig();

    /**
     * Check if currently using terrain configuration.
     * @return true if terrain config is active
     */
    bool isTerrainConfigActive() const { return m_terrainConfigActive; }

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

    // Configuration mode tracking
    bool m_terrainConfigActive = false;

    // Stored building config for switching back
    EdgeDetectionConfig m_buildingConfig;

    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_EDGE_DETECTION_PASS_H
