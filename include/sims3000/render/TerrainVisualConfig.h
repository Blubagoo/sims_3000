/**
 * @file TerrainVisualConfig.h
 * @brief CPU-side uniform buffer structure for terrain shader visuals
 *
 * Defines the TerrainVisualConfig struct that maps to the cbuffer TerrainVisuals
 * in the terrain fragment shader. Contains the 10-entry palette for terrain type
 * base colors and emissive colors with intensity.
 *
 * The config is populated from TerrainTypeInfo at initialization and can be
 * modified at runtime for live visual tuning.
 *
 * Memory layout matches HLSL cbuffer alignment requirements:
 * - float4[10] for base_colors (160 bytes)
 * - float4[10] for emissive_colors (160 bytes) - RGB + intensity in alpha
 * - float glow_time (4 bytes)
 * - float sea_level (4 bytes)
 * - float2 padding (8 bytes)
 * Total: 336 bytes (aligned to 16-byte boundary)
 *
 * Integration with ToonShaderConfig (Ticket 3-039):
 * - TerrainVisualConfig is managed by ToonShaderConfig singleton
 * - Changes are tracked via dirty flag for GPU uniform buffer updates
 * - Config can be loaded from JSON file for rapid iteration
 * - All changes take effect immediately (no restart required)
 *
 * @see TerrainTypeInfo for source data
 * @see ToonShaderConfig for singleton integration
 * @see assets/shaders/terrain.frag.hlsl for shader usage
 */

#ifndef SIMS3000_RENDER_TERRAIN_VISUAL_CONFIG_H
#define SIMS3000_RENDER_TERRAIN_VISUAL_CONFIG_H

#include <sims3000/terrain/TerrainTypeInfo.h>
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <cmath>
#include <string>
#include <functional>

namespace sims3000 {

/// Number of terrain types in the palette
constexpr std::size_t TERRAIN_PALETTE_SIZE = terrain::TERRAIN_TYPE_COUNT;

// Forward declarations
class ToonShaderConfig;

// =============================================================================
// Glow Animation Constants (must be defined before use)
// =============================================================================

/**
 * @namespace TerrainGlowAnimation
 * @brief Animation period constants for terrain glow behaviors.
 *
 * Each terrain type has a characteristic glow behavior:
 * - Static: No animation (Substrate, Ridge)
 * - Pulse: Smooth sine wave (Water types, BiolumeGrove, SporeFlats, EmberCrust)
 * - Shimmer: Rapid random flicker (PrismaFields)
 * - Irregular: Pseudo-random bubble pulse (BlightMires)
 */
namespace TerrainGlowAnimation {
    // Period constants in seconds
    constexpr float STATIC_PERIOD = 0.0f;           // No animation
    constexpr float WATER_PULSE_PERIOD = 7.0f;      // Slow pulse (6-8s)
    constexpr float BIOLUME_PULSE_PERIOD = 4.0f;    // Subtle pulse
    constexpr float PRISMA_SHIMMER_PERIOD = 0.2f;   // Fast shimmer base
    constexpr float SPORE_PULSE_PERIOD = 3.0f;      // Rhythmic pulse
    constexpr float BLIGHT_BUBBLE_PERIOD = 2.5f;    // Irregular base
    constexpr float EMBER_THROB_PERIOD = 5.0f;      // Slow throb

    // Amplitude constants (0.0-1.0, modulates intensity)
    constexpr float PULSE_AMPLITUDE = 0.3f;         // Standard pulse amplitude
    constexpr float SHIMMER_AMPLITUDE = 0.4f;       // Higher for shimmer
    constexpr float SUBTLE_AMPLITUDE = 0.15f;       // Subtle variation

    // Terrain type to glow behavior mapping
    // Used in shader: terrain_type -> period, amplitude
    // 0: Substrate - static
    // 1: Ridge - static (with crevice glow)
    // 2: DeepVoid - slow pulse
    // 3: FlowChannel - slow pulse
    // 4: StillBasin - slow pulse
    // 5: BiolumeGrove - subtle pulse
    // 6: PrismaFields - shimmer
    // 7: SporeFlats - rhythmic pulse
    // 8: BlightMires - irregular bubble
    // 9: EmberCrust - slow throb (with crevice glow)
} // namespace TerrainGlowAnimation

/**
 * @enum GlowBehavior
 * @brief Glow animation behavior types for terrain.
 *
 * Each terrain type has a characteristic glow behavior that provides
 * visual differentiation independent of color.
 */
enum class GlowBehavior : std::uint8_t {
    Static = 0,     ///< No animation - constant glow intensity
    Pulse,          ///< Smooth sinusoidal intensity variation
    Shimmer,        ///< Random intensity flicker (crystal refraction)
    Flow,           ///< UV-scrolling animation for directional movement
    Irregular       ///< Base pulse with random bright flares
};

/**
 * @struct GlowParameters
 * @brief Per-terrain-type glow behavior parameters.
 *
 * Configurable parameters for each terrain type's glow animation.
 * These are used by the shader to compute animated glow effects.
 */
struct GlowParameters {
    GlowBehavior behavior = GlowBehavior::Static;  ///< Animation type
    float period = 0.0f;              ///< Animation period in seconds (0 = static)
    float amplitude = 0.0f;           ///< Intensity variation range [0, 1]
    float phase_offset = 0.0f;        ///< Phase offset for variation between instances

    /// Default constructor - static behavior
    constexpr GlowParameters() = default;

    /// Parameterized constructor
    constexpr GlowParameters(GlowBehavior b, float p, float a, float o = 0.0f)
        : behavior(b), period(p), amplitude(a), phase_offset(o) {}
};

/**
 * @struct TerrainVisualConfigGPU
 * @brief GPU-only uniform buffer structure for terrain shader visuals.
 *
 * This struct contains ONLY the data that is uploaded to the GPU.
 * Exactly 336 bytes, matching the HLSL cbuffer layout.
 *
 * Memory layout:
 * - float4[10] for base_colors (160 bytes)
 * - float4[10] for emissive_colors (160 bytes)
 * - float glow_time (4 bytes)
 * - float sea_level (4 bytes)
 * - float2 padding (8 bytes)
 * Total: 336 bytes
 */
struct alignas(16) TerrainVisualConfigGPU {
    std::array<glm::vec4, TERRAIN_PALETTE_SIZE> base_colors;
    std::array<glm::vec4, TERRAIN_PALETTE_SIZE> emissive_colors;
    float glow_time;
    float sea_level;
    float _padding[2];
};

// Verify GPU struct size matches expected GPU buffer size
static_assert(sizeof(TerrainVisualConfigGPU) == 336,
    "TerrainVisualConfigGPU must be exactly 336 bytes for GPU uniform buffer");

/**
 * @struct TerrainVisualConfig
 * @brief Full terrain visual configuration including GPU and CPU-side data.
 *
 * This struct contains both the GPU uniform buffer data and additional
 * CPU-side parameters like glow animation parameters.
 *
 * For GPU upload, use getGPUData() which returns just the GPU portion.
 *
 * All colors are in linear RGB space for correct shader math.
 * The alpha channel of emissive_colors contains the intensity multiplier.
 *
 * Layout is designed to match HLSL cbuffer packing rules:
 * - Each float4 is 16-byte aligned
 * - Arrays of float4 are contiguous
 */
struct alignas(16) TerrainVisualConfig {
    /**
     * @brief Base colors for each terrain type (float4 = RGBA).
     *
     * The RGB components define the diffuse/albedo color that receives
     * toon lighting. Alpha is typically 1.0 (fully opaque).
     *
     * These are dark tones that look good when lit by the toon shader.
     */
    std::array<glm::vec4, TERRAIN_PALETTE_SIZE> base_colors;

    /**
     * @brief Emissive colors for each terrain type (float4 = RGB + intensity).
     *
     * - RGB: Emissive color in linear space
     * - Alpha: Intensity multiplier (0.05 for Substrate to 0.60 for PrismaFields)
     *
     * Emissive is added to final output unaffected by lighting bands.
     * The intensity hierarchy ensures terrain glows below building glow (0.5-1.0).
     */
    std::array<glm::vec4, TERRAIN_PALETTE_SIZE> emissive_colors;

    /**
     * @brief Animation time for glow effects (seconds).
     *
     * Updated each frame from the simulation clock.
     * Used for sin()-based pulse animations in the shader.
     */
    float glow_time;

    /**
     * @brief Sea level for water-related effects.
     *
     * Elevation level (0-31) at which water begins.
     * Used by shader for depth-based effects near water.
     */
    float sea_level;

    /**
     * @brief Padding to align to 16-byte boundary.
     */
    float _padding[2];

    // =========================================================================
    // Constructors
    // =========================================================================

    /**
     * @brief Default constructor - initializes from TerrainTypeInfo.
     */
    TerrainVisualConfig() : glow_time(0.0f), sea_level(8.0f), _padding{0.0f, 0.0f} {
        initializeFromTerrainTypeInfo();
        initializeGlowParameters();
    }

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * @brief Initialize colors from the static TerrainTypeInfo table.
     *
     * Populates base_colors and emissive_colors from TERRAIN_INFO.
     * Base colors are derived from emissive colors with reduced saturation/brightness.
     */
    void initializeFromTerrainTypeInfo() {
        for (std::size_t i = 0; i < TERRAIN_PALETTE_SIZE; ++i) {
            const auto& info = terrain::TERRAIN_INFO[i];

            // Base color: darker version of emissive for toon shading
            // We darken the emissive color to create the base diffuse color
            glm::vec3 emissive(info.emissive_color.x, info.emissive_color.y, info.emissive_color.z);

            // Base color is a much darker, desaturated version
            // This ensures the emissive "pops" against the base
            float brightness = 0.15f;  // Dark base for alien aesthetic
            glm::vec3 base = emissive * brightness;

            // Add slight purple/teal tint to base for alien aesthetic
            base.r += 0.02f;  // Subtle tint
            base.b += 0.03f;

            base_colors[i] = glm::vec4(base, 1.0f);

            // Emissive color: RGB from TerrainTypeInfo, intensity in alpha
            emissive_colors[i] = glm::vec4(
                emissive,
                info.emissive_intensity
            );
        }
    }

    // =========================================================================
    // Setters
    // =========================================================================

    /**
     * @brief Set the animation time.
     * @param time Time in seconds since start.
     */
    void setGlowTime(float time) {
        glow_time = time;
    }

    /**
     * @brief Set the sea level.
     * @param level Elevation level (0-31).
     */
    void setSeaLevel(float level) {
        sea_level = level;
    }

    /**
     * @brief Set base color for a terrain type.
     * @param type Terrain type index (0-9).
     * @param color RGBA base color.
     */
    void setBaseColor(std::size_t type, const glm::vec4& color) {
        if (type < TERRAIN_PALETTE_SIZE) {
            base_colors[type] = color;
        }
    }

    /**
     * @brief Set emissive color for a terrain type.
     * @param type Terrain type index (0-9).
     * @param color RGB emissive color.
     * @param intensity Glow intensity (0.0-1.0).
     */
    void setEmissiveColor(std::size_t type, const glm::vec3& color, float intensity) {
        if (type < TERRAIN_PALETTE_SIZE) {
            emissive_colors[type] = glm::vec4(color, intensity);
        }
    }

    // =========================================================================
    // Getters
    // =========================================================================

    /**
     * @brief Get the size of the GPU portion for upload.
     * @return Size in bytes (336).
     */
    static constexpr std::size_t getGPUSize() {
        return sizeof(TerrainVisualConfigGPU);
    }

    /**
     * @brief Get pointer to data for GPU upload.
     * @return Pointer to beginning of struct (first 336 bytes are GPU data).
     * @note Only the first 336 bytes should be uploaded to GPU.
     */
    const void* getData() const {
        return this;
    }

    /**
     * @brief Get the GPU data portion as a TerrainVisualConfigGPU struct.
     * @return Copy of the GPU-uploadable data.
     */
    TerrainVisualConfigGPU getGPUData() const {
        TerrainVisualConfigGPU gpu;
        gpu.base_colors = base_colors;
        gpu.emissive_colors = emissive_colors;
        gpu.glow_time = glow_time;
        gpu.sea_level = sea_level;
        gpu._padding[0] = 0.0f;
        gpu._padding[1] = 0.0f;
        return gpu;
    }

    // =========================================================================
    // Glow Parameters (not part of GPU buffer - separate uniform)
    // =========================================================================

    /**
     * @brief Per-terrain-type glow behavior parameters.
     *
     * These parameters control how each terrain type's glow animates.
     * Not part of the main uniform buffer - uploaded separately.
     */
    std::array<GlowParameters, TERRAIN_PALETTE_SIZE> glow_params;

    /**
     * @brief Initialize glow parameters from terrain-visuals.yaml specifications.
     */
    void initializeGlowParameters() {
        using namespace TerrainGlowAnimation;

        // Substrate (0) - Static
        glow_params[0] = GlowParameters(GlowBehavior::Static, STATIC_PERIOD, 0.0f);

        // Ridge (1) - Static with crevice glow
        glow_params[1] = GlowParameters(GlowBehavior::Static, STATIC_PERIOD, 0.0f);

        // DeepVoid (2) - Slow pulse
        glow_params[2] = GlowParameters(GlowBehavior::Pulse, WATER_PULSE_PERIOD, PULSE_AMPLITUDE);

        // FlowChannel (3) - Flow animation
        glow_params[3] = GlowParameters(GlowBehavior::Flow, WATER_PULSE_PERIOD, PULSE_AMPLITUDE);

        // StillBasin (4) - Slow pulse (slower than ocean)
        glow_params[4] = GlowParameters(GlowBehavior::Pulse, 8.0f, PULSE_AMPLITUDE);

        // BiolumeGrove (5) - Organic pulse
        glow_params[5] = GlowParameters(GlowBehavior::Pulse, BIOLUME_PULSE_PERIOD, SUBTLE_AMPLITUDE);

        // PrismaFields (6) - Shimmer
        glow_params[6] = GlowParameters(GlowBehavior::Shimmer, PRISMA_SHIMMER_PERIOD, SHIMMER_AMPLITUDE);

        // SporeFlats (7) - Rhythmic pulse
        glow_params[7] = GlowParameters(GlowBehavior::Pulse, SPORE_PULSE_PERIOD, PULSE_AMPLITUDE);

        // BlightMires (8) - Irregular bubbling
        glow_params[8] = GlowParameters(GlowBehavior::Irregular, BLIGHT_BUBBLE_PERIOD, PULSE_AMPLITUDE);

        // EmberCrust (9) - Slow throb with crevice glow
        glow_params[9] = GlowParameters(GlowBehavior::Pulse, EMBER_THROB_PERIOD, SUBTLE_AMPLITUDE);
    }

    /**
     * @brief Set glow parameters for a terrain type.
     * @param type Terrain type index (0-9).
     * @param params Glow parameters to set.
     */
    void setGlowParameters(std::size_t type, const GlowParameters& params) {
        if (type < TERRAIN_PALETTE_SIZE) {
            glow_params[type] = params;
        }
    }

    /**
     * @brief Get glow parameters for a terrain type.
     * @param type Terrain type index (0-9).
     * @return Glow parameters for the terrain type.
     */
    const GlowParameters& getGlowParameters(std::size_t type) const {
        if (type < TERRAIN_PALETTE_SIZE) {
            return glow_params[type];
        }
        return glow_params[0]; // Fallback to substrate
    }
};

// Verify alignment for GPU upload
static_assert(alignof(TerrainVisualConfig) >= 16,
    "TerrainVisualConfig must be at least 16-byte aligned for GPU uniform buffer");

// =============================================================================
// Crevice Glow Configuration
// =============================================================================

/**
 * @namespace CreviceGlow
 * @brief Configuration for normal-based crevice glow effect.
 *
 * Ridge and EmberCrust terrain types exhibit increased glow where
 * the surface normal deviates from vertical (in cracks and crevices).
 * This creates visually interesting depth on elevated terrain.
 */
namespace CreviceGlow {
    /**
     * @brief Threshold for crevice detection.
     *
     * Normal Y component below this value triggers crevice glow.
     * 1.0 = perfectly flat, 0.0 = perfectly vertical.
     */
    constexpr float NORMAL_THRESHOLD = 0.85f;

    /**
     * @brief Maximum crevice glow boost multiplier.
     *
     * Applied when normal is perpendicular to vertical.
     */
    constexpr float MAX_BOOST = 2.0f;

    /**
     * @brief Terrain types that exhibit crevice glow.
     *
     * Only Ridge (1) and EmberCrust (9) have this effect.
     */
    constexpr bool hasCreviceGlow(std::uint8_t terrain_type) {
        return terrain_type == 1 || terrain_type == 9;
    }
}

// =============================================================================
// TerrainVisualConfigManager
// =============================================================================

/**
 * @class TerrainVisualConfigManager
 * @brief Manages terrain visual configuration with change tracking and file loading.
 *
 * This class wraps TerrainVisualConfig to provide:
 * - Dirty flag tracking for GPU uniform buffer updates
 * - Integration with ToonShaderConfig singleton
 * - JSON configuration file loading for rapid iteration
 * - Change callbacks for live tuning during development
 *
 * Usage:
 *   auto& manager = TerrainVisualConfigManager::instance();
 *   manager.setBaseColor(0, glm::vec4(0.1f, 0.1f, 0.15f, 1.0f));
 *   if (manager.isDirty()) {
 *       // Upload config.getData() to GPU
 *       manager.clearDirtyFlag();
 *   }
 */
class TerrainVisualConfigManager {
public:
    /// Callback type for configuration change notifications
    using ChangeCallback = std::function<void()>;

    // =========================================================================
    // Singleton Access
    // =========================================================================

    /**
     * @brief Get the singleton instance.
     * @return Reference to the global TerrainVisualConfigManager instance.
     */
    static TerrainVisualConfigManager& instance() {
        static TerrainVisualConfigManager s_instance;
        return s_instance;
    }

    // Prevent copying and moving
    TerrainVisualConfigManager(const TerrainVisualConfigManager&) = delete;
    TerrainVisualConfigManager& operator=(const TerrainVisualConfigManager&) = delete;
    TerrainVisualConfigManager(TerrainVisualConfigManager&&) = delete;
    TerrainVisualConfigManager& operator=(TerrainVisualConfigManager&&) = delete;

    // =========================================================================
    // Configuration Access
    // =========================================================================

    /**
     * @brief Get the terrain visual configuration.
     * @return Const reference to the configuration.
     */
    const TerrainVisualConfig& getConfig() const { return m_config; }

    /**
     * @brief Get mutable access to the configuration.
     * @return Reference to the configuration.
     * @note Caller must call markDirty() after modifications.
     */
    TerrainVisualConfig& getConfigMutable() { return m_config; }

    // =========================================================================
    // Color Configuration (with automatic dirty tracking)
    // =========================================================================

    /**
     * @brief Set base color for a terrain type.
     * @param type Terrain type index (0-9).
     * @param color RGBA base color.
     */
    void setBaseColor(std::size_t type, const glm::vec4& color) {
        m_config.setBaseColor(type, color);
        markDirty();
    }

    /**
     * @brief Set emissive color for a terrain type.
     * @param type Terrain type index (0-9).
     * @param color RGB emissive color.
     * @param intensity Glow intensity (0.0-1.0).
     */
    void setEmissiveColor(std::size_t type, const glm::vec3& color, float intensity) {
        m_config.setEmissiveColor(type, color, intensity);
        markDirty();
    }

    /**
     * @brief Set glow time (animation time).
     * @param time Time in seconds.
     */
    void setGlowTime(float time) {
        m_config.setGlowTime(time);
        // Note: glow_time changes every frame, so we don't mark dirty here
        // to avoid constant uniform buffer re-uploads. The rendering system
        // should update glow_time directly in the uniform buffer.
    }

    /**
     * @brief Set sea level.
     * @param level Sea level (0-31).
     */
    void setSeaLevel(float level) {
        m_config.setSeaLevel(level);
        markDirty();
    }

    /**
     * @brief Set glow parameters for a terrain type.
     * @param type Terrain type index (0-9).
     * @param params Glow parameters.
     */
    void setGlowParameters(std::size_t type, const GlowParameters& params) {
        m_config.setGlowParameters(type, params);
        markDirty();
    }

    // =========================================================================
    // Dirty Flag Management
    // =========================================================================

    /**
     * @brief Check if configuration has changed since last clear.
     * @return True if any parameter changed since last clearDirtyFlag().
     */
    bool isDirty() const { return m_dirty; }

    /**
     * @brief Mark configuration as changed.
     *
     * Called automatically by setter methods. Can be called manually
     * after modifying config via getConfigMutable().
     */
    void markDirty() {
        m_dirty = true;
        notifyChange();
    }

    /**
     * @brief Clear the dirty flag.
     *
     * Called by the rendering system after uploading configuration to GPU.
     */
    void clearDirtyFlag() { m_dirty = false; }

    // =========================================================================
    // Change Notification
    // =========================================================================

    /**
     * @brief Set callback for configuration changes.
     * @param callback Function to call when configuration changes.
     *
     * Used for live tuning during development. The callback is invoked
     * immediately when any configuration value changes.
     */
    void setChangeCallback(ChangeCallback callback) {
        m_changeCallback = std::move(callback);
    }

    // =========================================================================
    // File Loading
    // =========================================================================

    /**
     * @brief Load configuration from JSON file.
     * @param filepath Path to JSON configuration file.
     * @return True if loading succeeded, false otherwise.
     *
     * JSON format:
     * {
     *   "base_colors": [
     *     {"r": 0.1, "g": 0.1, "b": 0.15, "a": 1.0},
     *     ...
     *   ],
     *   "emissive_colors": [
     *     {"r": 0.1, "g": 0.1, "b": 0.18, "intensity": 0.05},
     *     ...
     *   ],
     *   "glow_params": [
     *     {"behavior": "static", "period": 0.0, "amplitude": 0.0},
     *     ...
     *   ],
     *   "sea_level": 8.0
     * }
     */
    bool loadFromFile(const std::string& filepath);

    /**
     * @brief Save configuration to JSON file.
     * @param filepath Path to JSON configuration file.
     * @return True if saving succeeded, false otherwise.
     */
    bool saveToFile(const std::string& filepath) const;

    /**
     * @brief Reset to default values from TerrainTypeInfo.
     *
     * Restores all parameters to their Game Designer-specified defaults.
     */
    void resetToDefaults() {
        m_config = TerrainVisualConfig();
        markDirty();
    }

private:
    // =========================================================================
    // Private Constructor (Singleton)
    // =========================================================================

    TerrainVisualConfigManager() : m_dirty(true) {}

    // =========================================================================
    // Private Methods
    // =========================================================================

    void notifyChange() {
        if (m_changeCallback) {
            m_changeCallback();
        }
    }

    // =========================================================================
    // Member Variables
    // =========================================================================

    TerrainVisualConfig m_config;
    bool m_dirty = true;
    ChangeCallback m_changeCallback;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_TERRAIN_VISUAL_CONFIG_H
