/**
 * @file DebugGridOverlay.h
 * @brief Debug grid overlay rendering for development and debugging.
 *
 * Renders a procedural grid overlay on the terrain plane showing tile
 * boundaries at multiple scales. Supports toggle on/off, zoom-based
 * density adjustment, and camera angle-based fading for readability.
 *
 * Features:
 * - Toggle visibility via debug key
 * - Multiple grid scales (16x16, 64x64 tiles) with different colors
 * - Grid density adjusts based on camera zoom level
 * - Fading at extreme camera tilt angles to prevent visual clutter
 * - Handles configurable map sizes (128/256/512)
 *
 * Resource ownership:
 * - DebugGridOverlay owns pipeline and shader resources
 * - GPUDevice must outlive DebugGridOverlay
 * - Call release() before destroying to clean up GPU resources
 *
 * Usage:
 * @code
 *   DebugGridOverlay grid(device, swapchainFormat);
 *   grid.setMapSize(256, 256);
 *
 *   // Toggle with debug key
 *   if (debugKeyPressed) grid.toggle();
 *
 *   // Each frame, after scene rendering:
 *   if (grid.isEnabled()) {
 *       grid.render(cmdBuffer, outputTexture, width, height,
 *                   cameraUniforms, cameraState);
 *   }
 * @endcode
 */

#ifndef SIMS3000_RENDER_DEBUG_GRID_OVERLAY_H
#define SIMS3000_RENDER_DEBUG_GRID_OVERLAY_H

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <string>

namespace sims3000 {

// Forward declarations
class GPUDevice;
class CameraUniforms;
struct CameraState;

/**
 * @struct DebugGridConfig
 * @brief Configuration for debug grid overlay rendering.
 */
struct DebugGridConfig {
    /// Color for fine grid (16x16 tiles) - cyan/teal per canon bioluminescent palette
    glm::vec4 fineGridColor{0.0f, 0.8f, 0.8f, 0.4f};

    /// Color for coarse grid (64x64 tiles) - bright green per canon palette
    glm::vec4 coarseGridColor{0.2f, 1.0f, 0.3f, 0.6f};

    /// Fine grid spacing in tiles (default: 16x16)
    std::uint32_t fineGridSpacing = 16;

    /// Coarse grid spacing in tiles (default: 64x64)
    std::uint32_t coarseGridSpacing = 64;

    /// Base line thickness in screen-space pixels
    float lineThickness = 1.5f;

    /// Minimum camera pitch (degrees) where grid is fully visible
    float minPitchForFullOpacity = 25.0f;

    /// Maximum camera pitch (degrees) where grid starts fading
    float maxPitchForFade = 75.0f;

    /// Minimum opacity at extreme tilt angles
    float minOpacityAtExtremeTilt = 0.2f;

    /// Distance threshold for switching to coarse-only mode
    float coarseOnlyDistance = 150.0f;

    /// Distance at which to hide fine grid entirely
    float hideFineGridDistance = 100.0f;
};

/**
 * @struct DebugGridUBO
 * @brief Uniform buffer data for debug grid shader.
 *
 * Matches the cbuffer layout in debug_grid.frag.hlsl.
 */
struct DebugGridUBO {
    glm::mat4 viewProjection;          // 64 bytes: View-projection matrix
    glm::vec4 fineGridColor;           // 16 bytes: Fine grid color
    glm::vec4 coarseGridColor;         // 16 bytes: Coarse grid color
    glm::vec2 mapSize;                 //  8 bytes: Map dimensions (width, height)
    float fineGridSpacing;             //  4 bytes: Fine grid tile spacing
    float coarseGridSpacing;           //  4 bytes: Coarse grid tile spacing
    float lineThickness;               //  4 bytes: Line thickness in world units
    float opacity;                     //  4 bytes: Overall opacity (for tilt fading)
    float cameraDistance;              //  4 bytes: Camera distance for LOD
    float _padding;                    //  4 bytes: Align to 16 bytes
};
static_assert(sizeof(DebugGridUBO) == 128, "DebugGridUBO must be 128 bytes");

/**
 * @class DebugGridOverlay
 * @brief Renders a debug grid overlay for development.
 *
 * Renders a procedural grid on the XY plane (terrain) showing tile
 * boundaries at multiple scales. Integrates with the camera system
 * for zoom-based LOD and tilt-based opacity fading.
 */
class DebugGridOverlay {
public:
    /**
     * Create debug grid overlay.
     * @param device GPU device for resource creation
     * @param colorFormat Format of the color render target
     */
    DebugGridOverlay(GPUDevice& device, SDL_GPUTextureFormat colorFormat);

    ~DebugGridOverlay();

    // Non-copyable
    DebugGridOverlay(const DebugGridOverlay&) = delete;
    DebugGridOverlay& operator=(const DebugGridOverlay&) = delete;

    // Movable
    DebugGridOverlay(DebugGridOverlay&& other) noexcept;
    DebugGridOverlay& operator=(DebugGridOverlay&& other) noexcept;

    /**
     * Check if overlay is valid and ready to use.
     * @return true if resources were created successfully
     */
    bool isValid() const;

    /**
     * Enable or disable the grid overlay.
     * @param enabled True to enable, false to disable
     */
    void setEnabled(bool enabled);

    /**
     * Toggle the grid overlay on/off.
     */
    void toggle();

    /**
     * Check if grid overlay is currently enabled.
     * @return true if enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * Set the map size in tiles.
     * @param width Map width in tiles (128, 256, or 512)
     * @param height Map height in tiles (128, 256, or 512)
     */
    void setMapSize(std::uint32_t width, std::uint32_t height);

    /**
     * Get current grid configuration.
     * @return Current configuration
     */
    const DebugGridConfig& getConfig() const { return m_config; }

    /**
     * Set grid configuration.
     * @param config New configuration
     */
    void setConfig(const DebugGridConfig& config);

    /**
     * Set fine grid color.
     * @param color RGBA color
     */
    void setFineGridColor(const glm::vec4& color);

    /**
     * Set coarse grid color.
     * @param color RGBA color
     */
    void setCoarseGridColor(const glm::vec4& color);

    /**
     * Set line thickness.
     * @param thickness Thickness in screen-space pixels
     */
    void setLineThickness(float thickness);

    /**
     * Render the debug grid overlay.
     *
     * Should be called after the main scene render, typically in the
     * UI overlay phase. Renders a fullscreen pass that draws the grid
     * procedurally based on world position.
     *
     * @param cmdBuffer Command buffer for recording
     * @param outputTexture Output texture to render to
     * @param width Texture width
     * @param height Texture height
     * @param camera Camera uniforms for view-projection matrix
     * @param state Camera state for pitch and distance
     * @return true if rendering succeeded
     */
    bool render(
        SDL_GPUCommandBuffer* cmdBuffer,
        SDL_GPUTexture* outputTexture,
        std::uint32_t width,
        std::uint32_t height,
        const CameraUniforms& camera,
        const CameraState& state);

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

    /**
     * Calculate opacity based on camera pitch.
     * @param pitchDegrees Camera pitch in degrees
     * @return Opacity value [minOpacity, 1.0]
     */
    float calculateTiltOpacity(float pitchDegrees) const;

    GPUDevice* m_device = nullptr;
    SDL_GPUTextureFormat m_colorFormat = SDL_GPU_TEXTUREFORMAT_INVALID;

    // Configuration
    DebugGridConfig m_config;
    std::uint32_t m_mapWidth = 256;
    std::uint32_t m_mapHeight = 256;
    bool m_enabled = false;

    // Pipeline and shaders
    SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
    SDL_GPUShader* m_vertexShader = nullptr;
    SDL_GPUShader* m_fragmentShader = nullptr;

    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_DEBUG_GRID_OVERLAY_H
