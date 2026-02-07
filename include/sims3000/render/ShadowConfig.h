/**
 * @file ShadowConfig.h
 * @brief Shadow rendering configuration and quality settings.
 *
 * Provides configuration for basic directional light shadow mapping:
 * - Shadow map resolution (quality tiers)
 * - Shadow color and intensity (tuned for dark bioluminescent environment)
 * - Depth bias settings for shadow acne prevention
 * - Enable/disable toggle for low-end systems
 *
 * The alien sun direction is world-space fixed (not camera-relative).
 * Shadow frustum adapts to camera orientation via orthographic projection
 * fitted to the camera's view frustum.
 *
 * Usage:
 * @code
 *   ShadowConfig config;
 *   config.setQuality(ShadowQuality::High);
 *   config.setEnabled(true);
 *
 *   // Access settings
 *   uint32_t resolution = config.getShadowMapResolution();
 *   float bias = config.getDepthBias();
 * @endcode
 */

#ifndef SIMS3000_RENDER_SHADOW_CONFIG_H
#define SIMS3000_RENDER_SHADOW_CONFIG_H

#include <glm/glm.hpp>
#include <cstdint>

namespace sims3000 {

/**
 * @enum ShadowQuality
 * @brief Shadow quality presets for different hardware capabilities.
 *
 * Each tier adjusts shadow map resolution and filtering for
 * performance vs quality tradeoffs.
 */
enum class ShadowQuality : std::uint8_t {
    Disabled = 0,  ///< Shadows completely disabled (fastest)
    Low,           ///< 512x512 shadow map, basic filtering
    Medium,        ///< 1024x1024 shadow map, bilinear filtering
    High,          ///< 2048x2048 shadow map, PCF filtering
    Ultra          ///< 4096x4096 shadow map, enhanced PCF
};

/**
 * @struct ShadowConfig
 * @brief Configuration for directional light shadow mapping.
 *
 * All values have sensible defaults tuned for the dark bioluminescent
 * environment. Shadow color shifts toward purple per the alien aesthetic.
 */
struct ShadowConfig {
    // =========================================================================
    // Quality Settings
    // =========================================================================

    /// Shadow quality tier (affects resolution and filtering)
    ShadowQuality quality = ShadowQuality::High;

    /// Master enable/disable toggle
    bool enabled = true;

    // =========================================================================
    // Light Direction (World-Space Fixed Alien Sun)
    // =========================================================================

    /// Directional light direction (normalized, points toward light source)
    /// Default: (0.408, 0.816, 0.408) = normalized (1, 2, 1)
    /// This is the alien sun direction, fixed in world-space.
    glm::vec3 lightDirection{0.408248f, 0.816497f, 0.408248f};

    // =========================================================================
    // Shadow Color/Intensity (Dark Environment Tuning)
    // =========================================================================

    /// Shadow color tint (applied to shadowed areas)
    /// Default: Deep purple (#2A1B3D) per alien bioluminescent aesthetic
    glm::vec3 shadowColor{0.165f, 0.106f, 0.239f};

    /// Shadow intensity multiplier [0.0, 1.0]
    /// 0.0 = no shadow darkening, 1.0 = full shadow
    /// Default: 0.6 for visible but not harsh shadows in dark environment
    float shadowIntensity = 0.6f;

    /// Shadow softness for edge blending [0.0, 1.0]
    /// Lower values = harder edges (toon-appropriate)
    /// Default: 0.2 for relatively clean toon-style edges
    float shadowSoftness = 0.2f;

    // =========================================================================
    // Depth Bias (Shadow Acne Prevention)
    // =========================================================================

    /// Constant depth bias offset
    /// Prevents shadow acne by pushing shadow comparison slightly
    float depthBias = 0.0005f;

    /// Slope-scaled depth bias
    /// Additional bias based on surface slope relative to light
    float slopeBias = 0.002f;

    /// Normal offset bias (world units)
    /// Offsets sample position along surface normal
    float normalBias = 0.02f;

    // =========================================================================
    // Frustum Settings
    // =========================================================================

    /// Shadow frustum padding (world units)
    /// Extra margin around camera frustum to prevent shadow pop-in
    float frustumPadding = 5.0f;

    /// Minimum shadow frustum size (world units)
    /// Prevents overly tight frustum when zoomed in
    float minFrustumSize = 50.0f;

    /// Maximum shadow frustum size (world units)
    /// Limits shadow map coverage when zoomed out for quality
    float maxFrustumSize = 500.0f;

    // =========================================================================
    // Texel Snapping (Camera Movement Stability)
    // =========================================================================

    /// Enable shadow map texel snapping
    /// When enabled, light frustum snaps to shadow map texels to prevent
    /// shimmering during camera movement.
    bool texelSnapping = true;

    // =========================================================================
    // Methods
    // =========================================================================

    /**
     * @brief Get shadow map resolution based on quality tier.
     * @return Shadow map width/height in pixels (always square).
     */
    std::uint32_t getShadowMapResolution() const;

    /**
     * @brief Check if shadows are effectively enabled.
     * @return true if enabled and quality is not Disabled.
     */
    bool isEnabled() const { return enabled && quality != ShadowQuality::Disabled; }

    /**
     * @brief Set quality tier and update resolution.
     * @param q Quality tier to set.
     */
    void setQuality(ShadowQuality q) { quality = q; }

    /**
     * @brief Get the number of PCF samples based on quality.
     * @return Number of samples for percentage-closer filtering.
     *
     * Higher sample counts produce softer shadow edges but cost more.
     */
    std::uint32_t getPCFSampleCount() const;

    /**
     * @brief Apply a quality preset.
     * @param q Quality tier to apply.
     *
     * Adjusts multiple settings for the specified quality level.
     */
    void applyQualityPreset(ShadowQuality q);

    /**
     * @brief Reset to default values.
     */
    void resetToDefaults();
};

// =============================================================================
// Default Constants
// =============================================================================

/**
 * @namespace ShadowConfigDefaults
 * @brief Default values for shadow configuration.
 */
namespace ShadowConfigDefaults {
    // Light direction (normalized (1, 2, 1))
    constexpr float LIGHT_DIR_X = 0.408248f;
    constexpr float LIGHT_DIR_Y = 0.816497f;
    constexpr float LIGHT_DIR_Z = 0.408248f;

    // Shadow color (#2A1B3D deep purple)
    constexpr float SHADOW_COLOR_R = 42.0f / 255.0f;   // 0.165
    constexpr float SHADOW_COLOR_G = 27.0f / 255.0f;   // 0.106
    constexpr float SHADOW_COLOR_B = 61.0f / 255.0f;   // 0.239

    // Shadow intensity and softness
    constexpr float SHADOW_INTENSITY = 0.6f;
    constexpr float SHADOW_SOFTNESS = 0.2f;

    // Depth bias values
    constexpr float DEPTH_BIAS = 0.0005f;
    constexpr float SLOPE_BIAS = 0.002f;
    constexpr float NORMAL_BIAS = 0.02f;

    // Frustum settings
    constexpr float FRUSTUM_PADDING = 5.0f;
    constexpr float MIN_FRUSTUM_SIZE = 50.0f;
    constexpr float MAX_FRUSTUM_SIZE = 500.0f;

    // Resolution per quality tier
    constexpr std::uint32_t RESOLUTION_LOW = 512;
    constexpr std::uint32_t RESOLUTION_MEDIUM = 1024;
    constexpr std::uint32_t RESOLUTION_HIGH = 2048;
    constexpr std::uint32_t RESOLUTION_ULTRA = 4096;

    // PCF samples per quality tier
    constexpr std::uint32_t PCF_SAMPLES_LOW = 1;      // No PCF
    constexpr std::uint32_t PCF_SAMPLES_MEDIUM = 4;   // 2x2
    constexpr std::uint32_t PCF_SAMPLES_HIGH = 9;     // 3x3
    constexpr std::uint32_t PCF_SAMPLES_ULTRA = 16;   // 4x4
}

/**
 * @brief Get string name for quality tier.
 * @param quality Quality tier enum value.
 * @return Human-readable string.
 */
const char* getShadowQualityName(ShadowQuality quality);

} // namespace sims3000

#endif // SIMS3000_RENDER_SHADOW_CONFIG_H
