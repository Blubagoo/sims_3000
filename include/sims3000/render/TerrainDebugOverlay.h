/**
 * @file TerrainDebugOverlay.h
 * @brief Terrain-specific debug visualization overlay for development.
 *
 * Extends the debug grid overlay (Epic 2 ticket 2-040) with terrain-specific
 * visualization modes for tuning generation parameters, LOD distances, and
 * chunk boundaries. All debug modes are independently toggled via key bindings.
 *
 * Visualization modes:
 * - Elevation heatmap: Blue (low) to red (high) for 0-31 elevation
 * - Terrain type colormap: Distinct false color per terrain type
 * - Chunk boundary visualization: Lines at 32-tile chunk edges
 * - LOD level visualization: Color per chunk based on current LOD level
 * - Normals visualization: Per-vertex normal direction arrows
 * - Water body ID visualization: Unique color per water body
 * - Buildability overlay: Green (buildable) vs red (unbuildable) tiles
 *
 * Performance target: Debug overlays add < 0.5ms when active.
 *
 * Resource ownership:
 * - TerrainDebugOverlay owns pipeline and shader resources
 * - GPUDevice must outlive TerrainDebugOverlay
 * - ITerrainRenderData pointer must remain valid during render
 *
 * Usage:
 * @code
 *   TerrainDebugOverlay debugOverlay(device, swapchainFormat);
 *   debugOverlay.setTerrainData(&terrainRenderData);
 *
 *   // Toggle modes with debug keys
 *   if (key1Pressed) debugOverlay.toggleMode(TerrainDebugMode::ElevationHeatmap);
 *   if (key2Pressed) debugOverlay.toggleMode(TerrainDebugMode::TerrainType);
 *
 *   // Each frame, after scene rendering:
 *   debugOverlay.render(cmdBuffer, outputTexture, width, height,
 *                       cameraUniforms, cameraState, chunkLODLevels);
 * @endcode
 */

#ifndef SIMS3000_RENDER_TERRAIN_DEBUG_OVERLAY_H
#define SIMS3000_RENDER_TERRAIN_DEBUG_OVERLAY_H

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <array>
#include <vector>

namespace sims3000 {

// Forward declarations
class GPUDevice;
class CameraUniforms;
struct CameraState;

namespace terrain {
class ITerrainRenderData;
class ITerrainQueryable;
} // namespace terrain

/**
 * @enum TerrainDebugMode
 * @brief Available terrain debug visualization modes.
 *
 * Multiple modes can be active simultaneously. Each mode renders
 * as a semi-transparent overlay on top of the terrain.
 */
enum class TerrainDebugMode : std::uint8_t {
    ElevationHeatmap = 0,  ///< Blue-to-red elevation color ramp (0-31)
    TerrainType = 1,       ///< Distinct color per terrain type
    ChunkBoundary = 2,     ///< Lines at 32-tile chunk edges
    LODLevel = 3,          ///< Color per chunk based on LOD level
    Normals = 4,           ///< Per-vertex normal direction arrows
    WaterBodyID = 5,       ///< Unique color per water body
    Buildability = 6,      ///< Green/red buildable/unbuildable overlay

    Count = 7              ///< Number of debug modes
};

/**
 * @struct TerrainDebugConfig
 * @brief Configuration for terrain debug overlay rendering.
 */
struct TerrainDebugConfig {
    /// Overlay opacity (0.0 = invisible, 1.0 = opaque)
    float overlayOpacity = 0.5f;

    /// Line thickness for chunk boundaries
    float chunkLineThickness = 2.0f;

    /// Arrow length for normal visualization (world units)
    float normalArrowLength = 0.5f;

    /// Grid spacing for normal arrows (sample every N tiles)
    std::uint32_t normalGridSpacing = 2;

    /// Colors for elevation heatmap (low to high)
    glm::vec4 elevationColorLow{0.0f, 0.0f, 1.0f, 1.0f};   // Blue
    glm::vec4 elevationColorMid{1.0f, 1.0f, 0.0f, 1.0f};   // Yellow
    glm::vec4 elevationColorHigh{1.0f, 0.0f, 0.0f, 1.0f};  // Red

    /// Colors for buildability overlay
    glm::vec4 buildableColor{0.0f, 1.0f, 0.0f, 0.5f};      // Green
    glm::vec4 unbuildableColor{1.0f, 0.0f, 0.0f, 0.5f};    // Red

    /// Chunk boundary color
    glm::vec4 chunkBoundaryColor{1.0f, 1.0f, 1.0f, 0.7f};  // White

    /// LOD level colors (Level 0, 1, 2)
    glm::vec4 lodColors[3] = {
        {0.0f, 1.0f, 0.0f, 0.4f},  // Green for LOD 0
        {1.0f, 1.0f, 0.0f, 0.4f},  // Yellow for LOD 1
        {1.0f, 0.0f, 0.0f, 0.4f}   // Red for LOD 2
    };

    /// Terrain type colors (indexed by TerrainType enum)
    std::array<glm::vec4, 10> terrainTypeColors = {{
        {0.6f, 0.5f, 0.4f, 0.6f},  // Substrate - Brown
        {0.5f, 0.4f, 0.3f, 0.6f},  // Ridge - Dark brown
        {0.0f, 0.0f, 0.3f, 0.6f},  // DeepVoid - Dark blue
        {0.0f, 0.5f, 0.8f, 0.6f},  // FlowChannel - Light blue
        {0.0f, 0.3f, 0.6f, 0.6f},  // StillBasin - Medium blue
        {0.0f, 0.6f, 0.0f, 0.6f},  // BiolumeGrove - Green
        {0.8f, 0.2f, 0.8f, 0.6f},  // PrismaFields - Magenta
        {0.2f, 0.8f, 0.5f, 0.6f},  // SporeFlats - Teal
        {0.4f, 0.2f, 0.0f, 0.6f},  // BlightMires - Dark orange
        {0.8f, 0.4f, 0.0f, 0.6f}   // EmberCrust - Orange
    }};
};

/**
 * @struct TerrainDebugUBO
 * @brief Uniform buffer data for terrain debug shader.
 *
 * Matches the cbuffer layout in terrain_debug.frag.hlsl.
 * Contains all parameters needed for rendering active debug modes.
 */
struct TerrainDebugUBO {
    glm::mat4 viewProjection;          // 64 bytes: View-projection matrix
    glm::vec4 elevationColorLow;       // 16 bytes: Low elevation color
    glm::vec4 elevationColorMid;       // 16 bytes: Mid elevation color
    glm::vec4 elevationColorHigh;      // 16 bytes: High elevation color
    glm::vec4 buildableColor;          // 16 bytes: Buildable tile color
    glm::vec4 unbuildableColor;        // 16 bytes: Unbuildable tile color
    glm::vec4 chunkBoundaryColor;      // 16 bytes: Chunk boundary line color
    glm::vec2 mapSize;                 //  8 bytes: Map dimensions
    float chunkSize;                   //  4 bytes: Chunk size (32)
    float lineThickness;               //  4 bytes: Line thickness
    float opacity;                     //  4 bytes: Overall overlay opacity
    std::uint32_t activeModeMask;      //  4 bytes: Bit mask of active modes
    float cameraDistance;              //  4 bytes: Camera distance for LOD fade
    float _padding;                    //  4 bytes: Padding to 16-byte alignment
};
static_assert(sizeof(TerrainDebugUBO) == 192, "TerrainDebugUBO must be 192 bytes");

/**
 * @struct ChunkLODInfo
 * @brief LOD level information for a single chunk.
 *
 * Passed to the overlay for LOD level visualization.
 */
struct ChunkLODInfo {
    std::uint8_t lodLevel;  ///< Current LOD level (0, 1, or 2)
};

/**
 * @class TerrainDebugOverlay
 * @brief Renders terrain-specific debug visualizations.
 *
 * Provides multiple independently-toggled debug visualization modes
 * for development and tuning. Renders as semi-transparent overlays
 * on top of the terrain.
 */
class TerrainDebugOverlay {
public:
    /**
     * Create terrain debug overlay.
     * @param device GPU device for resource creation
     * @param colorFormat Format of the color render target
     */
    TerrainDebugOverlay(GPUDevice& device, SDL_GPUTextureFormat colorFormat);

    ~TerrainDebugOverlay();

    // Non-copyable
    TerrainDebugOverlay(const TerrainDebugOverlay&) = delete;
    TerrainDebugOverlay& operator=(const TerrainDebugOverlay&) = delete;

    // Movable
    TerrainDebugOverlay(TerrainDebugOverlay&& other) noexcept;
    TerrainDebugOverlay& operator=(TerrainDebugOverlay&& other) noexcept;

    /**
     * Check if overlay is valid and ready to use.
     * @return true if resources were created successfully
     */
    bool isValid() const;

    // =========================================================================
    // Mode Control
    // =========================================================================

    /**
     * Enable or disable a specific debug mode.
     * @param mode The debug mode to set
     * @param enabled True to enable, false to disable
     */
    void setModeEnabled(TerrainDebugMode mode, bool enabled);

    /**
     * Toggle a specific debug mode on/off.
     * @param mode The debug mode to toggle
     */
    void toggleMode(TerrainDebugMode mode);

    /**
     * Check if a specific debug mode is enabled.
     * @param mode The debug mode to check
     * @return true if enabled
     */
    bool isModeEnabled(TerrainDebugMode mode) const;

    /**
     * Check if any debug mode is currently enabled.
     * @return true if at least one mode is active
     */
    bool hasActiveMode() const;

    /**
     * Disable all debug modes.
     */
    void disableAllModes();

    /**
     * Get the bitmask of currently active modes.
     * @return Bitmask where bit N is set if mode N is active
     */
    std::uint32_t getActiveModeMask() const { return m_activeModeMask; }

    // =========================================================================
    // Data Sources
    // =========================================================================

    /**
     * Set the terrain render data source.
     * @param data Pointer to terrain render data (must outlive render calls)
     */
    void setTerrainRenderData(const terrain::ITerrainRenderData* data);

    /**
     * Set the terrain queryable interface for buildability checks.
     * @param queryable Pointer to terrain queryable (must outlive render calls)
     */
    void setTerrainQueryable(const terrain::ITerrainQueryable* queryable);

    /**
     * Set chunk LOD level information for LOD visualization.
     * @param lodLevels Vector of LOD levels indexed by (chunk_y * chunks_x + chunk_x)
     * @param chunksX Number of chunks in X direction
     * @param chunksY Number of chunks in Y direction
     */
    void setChunkLODLevels(const std::vector<ChunkLODInfo>& lodLevels,
                           std::uint32_t chunksX, std::uint32_t chunksY);

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * Get current configuration.
     * @return Current configuration
     */
    const TerrainDebugConfig& getConfig() const { return m_config; }

    /**
     * Set configuration.
     * @param config New configuration
     */
    void setConfig(const TerrainDebugConfig& config);

    /**
     * Set overlay opacity.
     * @param opacity Opacity value [0.0, 1.0]
     */
    void setOpacity(float opacity);

    // =========================================================================
    // Rendering
    // =========================================================================

    /**
     * Render the terrain debug overlay.
     *
     * Renders all active debug modes as semi-transparent overlays.
     * Should be called after the main terrain render pass.
     *
     * @param cmdBuffer Command buffer for recording
     * @param outputTexture Output texture to render to
     * @param width Texture width
     * @param height Texture height
     * @param camera Camera uniforms for view-projection matrix
     * @param state Camera state for distance calculations
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
     * Update the data texture with current terrain/LOD data.
     * @return true if update succeeded
     */
    bool updateDataTexture();

    /**
     * Create the data texture for terrain visualization.
     * @return true if creation succeeded
     */
    bool createDataTexture();

    GPUDevice* m_device = nullptr;
    SDL_GPUTextureFormat m_colorFormat = SDL_GPU_TEXTUREFORMAT_INVALID;

    // Configuration
    TerrainDebugConfig m_config;
    std::uint32_t m_activeModeMask = 0;

    // Data sources
    const terrain::ITerrainRenderData* m_terrainRenderData = nullptr;
    const terrain::ITerrainQueryable* m_terrainQueryable = nullptr;
    std::vector<ChunkLODInfo> m_chunkLODLevels;
    std::uint32_t m_chunksX = 0;
    std::uint32_t m_chunksY = 0;

    // Map dimensions cache
    std::uint32_t m_mapWidth = 256;
    std::uint32_t m_mapHeight = 256;

    // GPU resources
    SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
    SDL_GPUShader* m_vertexShader = nullptr;
    SDL_GPUShader* m_fragmentShader = nullptr;
    SDL_GPUSampler* m_sampler = nullptr;

    // Data texture for terrain info lookup (elevation, type, buildability, LOD)
    // R = elevation (0-31), G = terrain type (0-9), B = flags (buildable, etc.), A = LOD level
    SDL_GPUTexture* m_dataTexture = nullptr;
    bool m_dataTextureDirty = true;

    // Normals texture for normal visualization
    // R = nx * 0.5 + 0.5, G = ny * 0.5 + 0.5, B = nz * 0.5 + 0.5, A = reserved
    SDL_GPUTexture* m_normalsTexture = nullptr;
    bool m_normalsTextureDirty = true;

    /**
     * Create the normals texture for normal visualization.
     * @return true if creation succeeded
     */
    bool createNormalsTexture();

    /**
     * Update the normals texture with computed terrain normals.
     * @return true if update succeeded
     */
    bool updateNormalsTexture();

    std::string m_lastError;
};

/**
 * @brief Get a deterministic color for a water body ID.
 *
 * Uses a hash-based color generation to assign distinct colors
 * to different water body IDs.
 *
 * @param bodyId Water body ID (0 = no water)
 * @return RGBA color for visualization
 */
inline glm::vec4 getWaterBodyColor(std::uint16_t bodyId) {
    if (bodyId == 0) {
        return glm::vec4(0.0f);  // No water body - transparent
    }

    // Hash-based color generation for distinct colors per body ID
    // Using a simple hash to spread IDs across the color space
    std::uint32_t hash = static_cast<std::uint32_t>(bodyId) * 2654435761u;  // Knuth multiplicative hash

    // Extract RGB components from hash
    float r = static_cast<float>((hash >> 0) & 0xFF) / 255.0f;
    float g = static_cast<float>((hash >> 8) & 0xFF) / 255.0f;
    float b = static_cast<float>((hash >> 16) & 0xFF) / 255.0f;

    // Ensure colors are bright enough to be visible
    float minBrightness = 0.4f;
    float brightness = (r + g + b) / 3.0f;
    if (brightness < minBrightness) {
        float scale = minBrightness / (brightness + 0.001f);
        r = (r * scale < 1.0f) ? r * scale : 1.0f;
        g = (g * scale < 1.0f) ? g * scale : 1.0f;
        b = (b * scale < 1.0f) ? b * scale : 1.0f;
    }

    return glm::vec4(r, g, b, 0.6f);
}

/**
 * @brief Get the debug mode name for display.
 *
 * @param mode The debug mode
 * @return Human-readable name string
 */
inline const char* getDebugModeName(TerrainDebugMode mode) {
    switch (mode) {
        case TerrainDebugMode::ElevationHeatmap: return "Elevation Heatmap";
        case TerrainDebugMode::TerrainType: return "Terrain Type";
        case TerrainDebugMode::ChunkBoundary: return "Chunk Boundaries";
        case TerrainDebugMode::LODLevel: return "LOD Level";
        case TerrainDebugMode::Normals: return "Normals";
        case TerrainDebugMode::WaterBodyID: return "Water Body ID";
        case TerrainDebugMode::Buildability: return "Buildability";
        default: return "Unknown";
    }
}

} // namespace sims3000

#endif // SIMS3000_RENDER_TERRAIN_DEBUG_OVERLAY_H
