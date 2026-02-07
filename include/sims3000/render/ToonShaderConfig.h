/**
 * @file ToonShaderConfig.h
 * @brief Runtime-configurable singleton resource for toon shader parameters.
 *
 * Provides a centralized configuration for toon shader rendering parameters
 * including:
 * - Band count and thresholds for toon shading
 * - Shadow color (purple shift amount)
 * - Edge line width
 * - Bloom threshold and intensity
 * - Emissive multiplier
 * - Per-terrain-type emissive color presets
 * - Ambient light level
 *
 * Changes take effect immediately without shader recompilation or restart.
 * Supports day/night palette shifts and accessibility options.
 *
 * Usage:
 *   auto& config = ToonShaderConfig::instance();
 *   config.setBandThreshold(1, 0.35f);
 *   config.setBloomIntensity(0.8f);
 */

#ifndef SIMS3000_RENDER_TOON_SHADER_CONFIG_H
#define SIMS3000_RENDER_TOON_SHADER_CONFIG_H

#include <glm/glm.hpp>
#include <array>
#include <cstdint>

namespace sims3000 {

/**
 * @enum TerrainType
 * @brief Terrain types with unique emissive color presets.
 *
 * Matches patterns.yaml terrain_types section. Each terrain type has
 * a distinct bioluminescent glow color for the alien aesthetic.
 */
enum class TerrainType : std::uint8_t {
    FlatGround = 0,    ///< Standard buildable terrain - subtle moss glow
    Hills,             ///< Elevated terrain - glowing vein patterns
    Ocean,             ///< Map-edge deep water
    River,             ///< Flowing water channels
    Lake,              ///< Inland water bodies
    Forest,            ///< Alien vegetation clusters - teal/green glow
    CrystalFields,     ///< Luminous crystal formations - magenta/cyan
    SporePlains,       ///< Bioluminescent spore flora - pulsing green/teal
    ToxicMarshes,      ///< Alien chemical pools - sickly yellow-green
    VolcanicRock,      ///< Hardened volcanic terrain - orange/red glow
    Count              ///< Number of terrain types (10)
};

/// Total number of terrain types for array sizing
constexpr std::size_t TERRAIN_TYPE_COUNT = static_cast<std::size_t>(TerrainType::Count);

/**
 * @struct BandConfig
 * @brief Configuration for a single lighting band.
 *
 * Each band has a threshold value and an intensity multiplier.
 * Thresholds define when the band activates based on lighting intensity.
 */
struct BandConfig {
    float threshold = 0.0f;    ///< Activation threshold [0.0, 1.0]
    float intensity = 1.0f;    ///< Intensity multiplier for this band
};

/**
 * @struct TerrainEmissivePreset
 * @brief Emissive color preset for a terrain type.
 *
 * Each terrain type has a default emissive color for bioluminescent rendering.
 */
struct TerrainEmissivePreset {
    glm::vec3 color{0.0f, 1.0f, 0.8f};  ///< RGB emissive color
    float intensity = 0.5f;              ///< Default intensity [0.0, 1.0]
};

/**
 * @class ToonShaderConfig
 * @brief Runtime-configurable singleton resource for toon shader parameters.
 *
 * All parameters can be modified at runtime and take effect immediately
 * on the next frame. The shader reads from this configuration during
 * the render phase.
 *
 * Thread Safety: This class is NOT thread-safe. All modifications should
 * occur on the main thread before the render phase.
 */
class ToonShaderConfig {
public:
    // =========================================================================
    // Singleton Access
    // =========================================================================

    /**
     * @brief Get the singleton instance.
     * @return Reference to the global ToonShaderConfig instance.
     *
     * Thread Safety: Safe for read access from any thread.
     * Modifications should occur on the main thread only.
     */
    static ToonShaderConfig& instance();

    // Prevent copying and moving
    ToonShaderConfig(const ToonShaderConfig&) = delete;
    ToonShaderConfig& operator=(const ToonShaderConfig&) = delete;
    ToonShaderConfig(ToonShaderConfig&&) = delete;
    ToonShaderConfig& operator=(ToonShaderConfig&&) = delete;

    // =========================================================================
    // Band Configuration
    // =========================================================================

    /// Maximum number of lighting bands supported
    static constexpr std::size_t MAX_BANDS = 4;

    /**
     * @brief Get the current number of active lighting bands.
     * @return Number of bands [1, MAX_BANDS].
     */
    std::size_t getBandCount() const { return m_bandCount; }

    /**
     * @brief Set the number of active lighting bands.
     * @param count Number of bands to use [1, MAX_BANDS].
     *
     * Bands are used in order from deepest shadow (0) to fully lit (count-1).
     */
    void setBandCount(std::size_t count);

    /**
     * @brief Get the threshold for a specific band.
     * @param bandIndex Index of the band [0, bandCount-1].
     * @return Threshold value [0.0, 1.0].
     */
    float getBandThreshold(std::size_t bandIndex) const;

    /**
     * @brief Set the threshold for a specific band.
     * @param bandIndex Index of the band [0, MAX_BANDS-1].
     * @param threshold Threshold value [0.0, 1.0].
     *
     * Thresholds should be in ascending order for correct rendering.
     */
    void setBandThreshold(std::size_t bandIndex, float threshold);

    /**
     * @brief Get the intensity multiplier for a specific band.
     * @param bandIndex Index of the band [0, bandCount-1].
     * @return Intensity multiplier [0.0, 1.0].
     */
    float getBandIntensity(std::size_t bandIndex) const;

    /**
     * @brief Set the intensity multiplier for a specific band.
     * @param bandIndex Index of the band [0, MAX_BANDS-1].
     * @param intensity Intensity multiplier [0.0, 1.0].
     */
    void setBandIntensity(std::size_t bandIndex, float intensity);

    /**
     * @brief Get all band configurations.
     * @return Array of band configurations.
     */
    const std::array<BandConfig, MAX_BANDS>& getBands() const { return m_bands; }

    // =========================================================================
    // Shadow Color Configuration
    // =========================================================================

    /**
     * @brief Get the shadow color (purple shift target).
     * @return RGB color for shadow tinting.
     */
    const glm::vec3& getShadowColor() const { return m_shadowColor; }

    /**
     * @brief Set the shadow color (purple shift target).
     * @param color RGB color to shift shadows toward.
     *
     * Default is deep purple (#2A1B3D) per alien aesthetic.
     */
    void setShadowColor(const glm::vec3& color) { m_shadowColor = color; }

    /**
     * @brief Get the shadow color shift amount.
     * @return Shift amount [0.0, 1.0], where 1.0 is full shift to shadow color.
     */
    float getShadowShiftAmount() const { return m_shadowShiftAmount; }

    /**
     * @brief Set the shadow color shift amount.
     * @param amount Shift amount [0.0, 1.0].
     */
    void setShadowShiftAmount(float amount);

    // =========================================================================
    // Edge/Outline Configuration
    // =========================================================================

    /**
     * @brief Get the edge line width in pixels.
     * @return Edge line width [0.0, 10.0].
     */
    float getEdgeLineWidth() const { return m_edgeLineWidth; }

    /**
     * @brief Set the edge line width in pixels.
     * @param width Edge line width [0.0, 10.0].
     *
     * Set to 0 to disable edge rendering.
     */
    void setEdgeLineWidth(float width);

    /**
     * @brief Get the edge outline color.
     * @return RGBA color for outlines (default: dark purple #2A1B3D).
     */
    const glm::vec4& getEdgeColor() const { return m_edgeColor; }

    /**
     * @brief Set the edge outline color.
     * @param color RGBA color for outlines.
     *
     * Default is dark purple (#2A1B3D) per alien aesthetic.
     * Alpha controls outline opacity/visibility.
     */
    void setEdgeColor(const glm::vec4& color);

    // =========================================================================
    // Bloom Configuration (Bioluminescent Rendering)
    // =========================================================================

    /**
     * @brief Get the bloom threshold.
     * @return Threshold value [0.0, 1.0].
     *
     * Pixels brighter than this threshold contribute to bloom.
     */
    float getBloomThreshold() const { return m_bloomThreshold; }

    /**
     * @brief Set the bloom threshold.
     * @param threshold Threshold value [0.0, 1.0].
     *
     * Lower values cause more bloom; higher values restrict bloom to
     * only the brightest pixels.
     */
    void setBloomThreshold(float threshold);

    /**
     * @brief Get the bloom intensity.
     * @return Intensity multiplier [0.0, 2.0].
     */
    float getBloomIntensity() const { return m_bloomIntensity; }

    /**
     * @brief Set the bloom intensity.
     * @param intensity Intensity multiplier [0.0, 2.0].
     *
     * Higher values create more prominent glow around bright areas.
     */
    void setBloomIntensity(float intensity);

    // =========================================================================
    // Emissive Configuration
    // =========================================================================

    /**
     * @brief Get the global emissive multiplier.
     * @return Multiplier [0.0, 2.0].
     */
    float getEmissiveMultiplier() const { return m_emissiveMultiplier; }

    /**
     * @brief Set the global emissive multiplier.
     * @param multiplier Multiplier [0.0, 2.0].
     *
     * Scales all emissive contributions. Set to 0 to disable emissive.
     * Values > 1.0 intensify glow effects.
     */
    void setEmissiveMultiplier(float multiplier);

    /**
     * @brief Get the emissive preset for a terrain type.
     * @param type Terrain type.
     * @return Emissive color and intensity preset.
     */
    const TerrainEmissivePreset& getTerrainEmissivePreset(TerrainType type) const;

    /**
     * @brief Set the emissive preset for a terrain type.
     * @param type Terrain type to configure.
     * @param preset Emissive color and intensity.
     */
    void setTerrainEmissivePreset(TerrainType type, const TerrainEmissivePreset& preset);

    /**
     * @brief Get all terrain emissive presets.
     * @return Array of terrain emissive presets indexed by TerrainType.
     */
    const std::array<TerrainEmissivePreset, TERRAIN_TYPE_COUNT>& getTerrainEmissivePresets() const {
        return m_terrainEmissivePresets;
    }

    // =========================================================================
    // Ambient Light Configuration
    // =========================================================================

    /**
     * @brief Get the ambient light level.
     * @return Ambient intensity [0.0, 1.0].
     */
    float getAmbientLevel() const { return m_ambientLevel; }

    /**
     * @brief Set the ambient light level.
     * @param level Ambient intensity [0.0, 1.0].
     *
     * Recommended range is 0.05-0.1 for the bioluminescent aesthetic.
     * Lower values create more contrast; higher values flatten shadows.
     */
    void setAmbientLevel(float level);

    // =========================================================================
    // Preset Application
    // =========================================================================

    /**
     * @brief Reset all parameters to Game Designer specifications (defaults).
     *
     * Restores all parameters to their canon-specified default values.
     */
    void resetToDefaults();

    /**
     * @brief Apply a day palette preset.
     *
     * Adjusts parameters for brighter daytime rendering while maintaining
     * the alien bioluminescent aesthetic.
     */
    void applyDayPalette();

    /**
     * @brief Apply a night palette preset.
     *
     * Adjusts parameters for darker nighttime rendering with enhanced
     * bioluminescent glow effects.
     */
    void applyNightPalette();

    /**
     * @brief Apply high-contrast accessibility preset.
     *
     * Increases contrast and edge visibility for accessibility.
     */
    void applyHighContrastPreset();

    // =========================================================================
    // State Query
    // =========================================================================

    /**
     * @brief Check if configuration has changed since last clear.
     * @return True if any parameter changed since last clearDirtyFlag().
     *
     * Used by the rendering system to detect when to re-upload uniforms.
     */
    bool isDirty() const { return m_dirty; }

    /**
     * @brief Clear the dirty flag.
     *
     * Called by the rendering system after uploading configuration to GPU.
     */
    void clearDirtyFlag() { m_dirty = false; }

private:
    // =========================================================================
    // Private Constructor (Singleton)
    // =========================================================================

    ToonShaderConfig();
    ~ToonShaderConfig() = default;

    // =========================================================================
    // Member Variables
    // =========================================================================

    // Band configuration
    std::size_t m_bandCount = MAX_BANDS;
    std::array<BandConfig, MAX_BANDS> m_bands;

    // Shadow configuration
    glm::vec3 m_shadowColor{0.165f, 0.106f, 0.239f};  // #2A1B3D deep purple
    float m_shadowShiftAmount = 0.7f;

    // Edge configuration
    float m_edgeLineWidth = 1.0f;
    glm::vec4 m_edgeColor{0.165f, 0.106f, 0.239f, 1.0f};  // Dark purple #2A1B3D

    // Bloom configuration
    float m_bloomThreshold = 0.7f;
    float m_bloomIntensity = 1.0f;

    // Emissive configuration
    float m_emissiveMultiplier = 1.0f;
    std::array<TerrainEmissivePreset, TERRAIN_TYPE_COUNT> m_terrainEmissivePresets;

    // Ambient configuration
    float m_ambientLevel = 0.08f;

    // Dirty flag for GPU uniform upload optimization
    bool m_dirty = true;

    // =========================================================================
    // Helper Methods
    // =========================================================================

    void initializeDefaults();
    void initializeTerrainEmissivePresets();
    void markDirty() { m_dirty = true; }
};

// =============================================================================
// Default Value Constants
// =============================================================================

/**
 * @namespace ToonShaderConfigDefaults
 * @brief Default values matching Game Designer specifications.
 */
namespace ToonShaderConfigDefaults {
    // Band thresholds (intensity values in range [0,1])
    constexpr float BAND_THRESHOLD_0 = 0.0f;   // Deep shadow starts at 0
    constexpr float BAND_THRESHOLD_1 = 0.2f;   // Shadow threshold
    constexpr float BAND_THRESHOLD_2 = 0.4f;   // Mid threshold
    constexpr float BAND_THRESHOLD_3 = 0.7f;   // Lit threshold

    // Band intensities
    constexpr float BAND_INTENSITY_0 = 0.15f;  // Deep shadow intensity
    constexpr float BAND_INTENSITY_1 = 0.35f;  // Shadow intensity
    constexpr float BAND_INTENSITY_2 = 0.65f;  // Mid intensity
    constexpr float BAND_INTENSITY_3 = 1.0f;   // Lit intensity

    // Shadow color (#2A1B3D = deep purple)
    constexpr float SHADOW_COLOR_R = 42.0f / 255.0f;   // 0.165
    constexpr float SHADOW_COLOR_G = 27.0f / 255.0f;   // 0.106
    constexpr float SHADOW_COLOR_B = 61.0f / 255.0f;   // 0.239
    constexpr float SHADOW_SHIFT_AMOUNT = 0.7f;

    // Edge rendering
    constexpr float EDGE_LINE_WIDTH = 1.0f;

    // Edge color (dark purple #2A1B3D)
    constexpr float EDGE_COLOR_R = 42.0f / 255.0f;    // 0.165
    constexpr float EDGE_COLOR_G = 27.0f / 255.0f;    // 0.106
    constexpr float EDGE_COLOR_B = 61.0f / 255.0f;    // 0.239
    constexpr float EDGE_COLOR_A = 1.0f;              // Full opacity

    // Bloom parameters
    constexpr float BLOOM_THRESHOLD = 0.7f;
    constexpr float BLOOM_INTENSITY = 1.0f;

    // Emissive parameters
    constexpr float EMISSIVE_MULTIPLIER = 1.0f;

    // Ambient light (0.05-0.1 recommended)
    constexpr float AMBIENT_LEVEL = 0.08f;
    constexpr float AMBIENT_LEVEL_MIN = 0.05f;
    constexpr float AMBIENT_LEVEL_MAX = 0.1f;
}

} // namespace sims3000

#endif // SIMS3000_RENDER_TOON_SHADER_CONFIG_H
