/**
 * @file EmissiveMaterial.h
 * @brief Emissive material system for bioluminescent art direction.
 *
 * Core rendering feature for the bioluminescent alien aesthetic. Provides:
 * - Canonical emissive color palette (cyan, green, amber, magenta)
 * - Glow intensity hierarchy (player structures > terrain > background)
 * - Per-instance emissive control with smooth transitions
 * - Integration with ToonShaderConfig terrain presets
 *
 * Emissive colors are added to final output unaffected by lighting bands.
 *
 * @see Ticket 2-037: Emissive Material Support
 */

#ifndef SIMS3000_RENDER_EMISSIVE_MATERIAL_H
#define SIMS3000_RENDER_EMISSIVE_MATERIAL_H

#include "sims3000/core/Interpolatable.h"
#include "sims3000/render/ToonShaderConfig.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace sims3000 {

// =============================================================================
// Emissive Palette - Canonical Bioluminescent Colors
// =============================================================================

/**
 * @namespace EmissivePalette
 * @brief Canonical emissive color palette for bioluminescent art direction.
 *
 * All emissive colors in the game should use these palette colors to maintain
 * visual consistency. Colors are in linear RGB space for shader use.
 */
namespace EmissivePalette {
    /// Primary cyan/teal (#00D4AA) - Main bioluminescent color
    /// Used for: UI highlights, active structures, healthy zones
    constexpr glm::vec3 CYAN{0.0f, 0.831f, 0.667f};  // #00D4AA in linear RGB

    /// Bright green (#00FF88) - Growth and activity
    /// Used for: SporePlains, active vegetation, positive feedback
    constexpr glm::vec3 GREEN{0.0f, 1.0f, 0.533f};

    /// Warm amber/orange (#FFA500) - Energy and warnings
    /// Used for: Energy systems, VolcanicRock, caution states
    constexpr glm::vec3 AMBER{1.0f, 0.647f, 0.0f};

    /// Magenta/pink (#FF00FF) - Special and rare
    /// Used for: CrystalFields, landmarks, special structures
    constexpr glm::vec3 MAGENTA{1.0f, 0.0f, 1.0f};

    /// Deep purple (#8B00FF) - Mysterious and alien
    /// Used for: Deep shadow tints, alien artifacts
    constexpr glm::vec3 PURPLE{0.545f, 0.0f, 1.0f};

    /// Soft white/blue (#AADDFF) - Water and fluid systems
    /// Used for: Rivers, lakes, fluid conduits
    constexpr glm::vec3 WATER_BLUE{0.667f, 0.867f, 1.0f};

    /// Toxic yellow-green (#CCFF00) - Hazard and contamination
    /// Used for: ToxicMarshes, contamination, warnings
    constexpr glm::vec3 TOXIC_GREEN{0.8f, 1.0f, 0.0f};
}

// =============================================================================
// Glow Intensity Hierarchy
// =============================================================================

/**
 * @enum EmissiveCategory
 * @brief Categories for glow intensity hierarchy.
 *
 * Glow intensity follows this hierarchy to ensure visual clarity:
 * 1. Player structures (highest) - Clear feedback for player-built items
 * 2. Terrain features (medium) - Environmental glow without overwhelming
 * 3. Background elements (lowest) - Subtle atmospheric glow
 */
enum class EmissiveCategory : std::uint8_t {
    Background = 0,      ///< Lowest intensity - ambient environmental glow
    TerrainFeature = 1,  ///< Medium intensity - terrain-based bioluminescence
    PlayerStructure = 2, ///< Highest intensity - player-built structures
    Count = 3
};

/**
 * @namespace GlowHierarchy
 * @brief Intensity multipliers for each emissive category.
 *
 * These multipliers are applied on top of the base emissive intensity
 * to enforce the visual hierarchy: player structures > terrain > background.
 */
namespace GlowHierarchy {
    /// Background elements: subtle ambient glow (30% of base)
    constexpr float BACKGROUND_MULTIPLIER = 0.3f;

    /// Terrain features: moderate bioluminescence (60% of base)
    constexpr float TERRAIN_MULTIPLIER = 0.6f;

    /// Player structures: full glow intensity (100% of base)
    constexpr float PLAYER_STRUCTURE_MULTIPLIER = 1.0f;

    /**
     * @brief Get the intensity multiplier for an emissive category.
     * @param category The emissive category.
     * @return Intensity multiplier [0.0, 1.0].
     */
    constexpr float getMultiplier(EmissiveCategory category) {
        switch (category) {
            case EmissiveCategory::Background:
                return BACKGROUND_MULTIPLIER;
            case EmissiveCategory::TerrainFeature:
                return TERRAIN_MULTIPLIER;
            case EmissiveCategory::PlayerStructure:
                return PLAYER_STRUCTURE_MULTIPLIER;
            default:
                return 1.0f;
        }
    }
}

// =============================================================================
// Emissive State - Interpolated Glow for Smooth Transitions
// =============================================================================

/**
 * @class EmissiveState
 * @brief Per-instance emissive state with interpolated transitions.
 *
 * Manages emissive intensity and color with smooth ~0.5s transitions
 * when state changes (e.g., building powered/unpowered).
 *
 * Uses Interpolatable<float> for intensity and stores target color.
 * The interpolation rate is set to complete a full transition in
 * approximately 0.5 seconds at the 20Hz simulation tick rate.
 *
 * Usage:
 * @code
 *     EmissiveState state;
 *
 *     // Set powered state (starts transition)
 *     state.setPowered(true, EmissivePalette::CYAN, EmissiveCategory::PlayerStructure);
 *
 *     // Each simulation tick
 *     state.tick(deltaTime);
 *
 *     // Get interpolated values for rendering
 *     float intensity = state.getInterpolatedIntensity(alpha);
 *     glm::vec3 color = state.getColor();
 * @endcode
 */
class EmissiveState {
public:
    /**
     * @brief Default constructor - unpowered state.
     */
    EmissiveState()
        : m_intensity(0.0f)
        , m_targetIntensity(0.0f)
        , m_color(EmissivePalette::CYAN)
        , m_category(EmissiveCategory::PlayerStructure)
        , m_powered(false)
    {
    }

    /**
     * @brief Construct with initial state.
     * @param powered Initial powered state.
     * @param color Emissive color.
     * @param category Glow hierarchy category.
     * @param intensity Base intensity [0.0, 1.0].
     */
    EmissiveState(bool powered, const glm::vec3& color,
                  EmissiveCategory category, float intensity = 1.0f)
        : m_intensity(powered ? intensity * GlowHierarchy::getMultiplier(category) : 0.0f)
        , m_targetIntensity(powered ? intensity * GlowHierarchy::getMultiplier(category) : 0.0f)
        , m_color(color)
        , m_category(category)
        , m_powered(powered)
    {
    }

    // =========================================================================
    // State Modification
    // =========================================================================

    /**
     * @brief Set the powered state, initiating a smooth transition.
     * @param powered New powered state.
     * @param color Emissive color to use.
     * @param category Glow hierarchy category.
     * @param baseIntensity Base intensity before hierarchy multiplier [0.0, 1.0].
     */
    void setPowered(bool powered, const glm::vec3& color,
                    EmissiveCategory category, float baseIntensity = 1.0f) {
        m_powered = powered;
        m_color = color;
        m_category = category;

        // Apply hierarchy multiplier to get target intensity
        float multiplier = GlowHierarchy::getMultiplier(category);
        m_targetIntensity = powered ? baseIntensity * multiplier : 0.0f;
    }

    /**
     * @brief Set powered state using terrain emissive preset.
     * @param powered New powered state.
     * @param terrainType Terrain type to get color/intensity from.
     */
    void setPoweredForTerrain(bool powered, TerrainType terrainType) {
        const auto& preset = ToonShaderConfig::instance().getTerrainEmissivePreset(terrainType);
        setPowered(powered, preset.color, EmissiveCategory::TerrainFeature, preset.intensity);
    }

    /**
     * @brief Immediately set intensity without transition.
     * @param intensity Target intensity.
     *
     * Use for initialization or teleporting to avoid transition artifacts.
     */
    void setIntensityImmediate(float intensity) {
        float multiplier = GlowHierarchy::getMultiplier(m_category);
        float adjusted = intensity * multiplier;
        m_intensity.setBoth(adjusted);
        m_targetIntensity = adjusted;
    }

    /**
     * @brief Set emissive color.
     * @param color New RGB emissive color.
     */
    void setColor(const glm::vec3& color) {
        m_color = color;
    }

    /**
     * @brief Set the glow hierarchy category.
     * @param category New category.
     *
     * Recalculates target intensity with new hierarchy multiplier.
     */
    void setCategory(EmissiveCategory category) {
        if (m_category != category) {
            float oldMultiplier = GlowHierarchy::getMultiplier(m_category);
            float newMultiplier = GlowHierarchy::getMultiplier(category);

            // Preserve the base intensity by scaling
            if (oldMultiplier > 0.0f) {
                float baseIntensity = m_targetIntensity / oldMultiplier;
                m_targetIntensity = baseIntensity * newMultiplier;
            }

            m_category = category;
        }
    }

    // =========================================================================
    // Tick Update
    // =========================================================================

    /**
     * @brief Update interpolation state for a simulation tick.
     *
     * Call once per simulation tick (20Hz). Smoothly transitions
     * intensity toward target over approximately 0.5 seconds.
     */
    void tick() {
        m_intensity.rotateTick();

        // Smoothly transition toward target (lerp factor for ~0.5s at 20Hz = 10 ticks)
        // Using 0.2 lerp factor: after 10 ticks, ~87% of transition complete
        constexpr float TRANSITION_RATE = 0.2f;  // Per-tick interpolation factor

        float current = m_intensity.current();
        float delta = m_targetIntensity - current;

        if (glm::abs(delta) < 0.001f) {
            // Close enough, snap to target
            m_intensity.set(m_targetIntensity);
        } else {
            // Smooth transition
            m_intensity.set(current + delta * TRANSITION_RATE);
        }
    }

    // =========================================================================
    // Queries
    // =========================================================================

    /**
     * @brief Get current intensity (for logic, not rendering).
     * @return Current intensity value.
     */
    float getCurrentIntensity() const {
        return m_intensity.current();
    }

    /**
     * @brief Get interpolated intensity for rendering.
     * @param alpha Interpolation factor from SimulationClock [0.0, 1.0].
     * @return Interpolated intensity value.
     */
    float getInterpolatedIntensity(float alpha) const {
        return m_intensity.lerp(alpha);
    }

    /**
     * @brief Get the target intensity (after transition completes).
     * @return Target intensity value.
     */
    float getTargetIntensity() const {
        return m_targetIntensity;
    }

    /**
     * @brief Get the emissive color.
     * @return RGB emissive color.
     */
    const glm::vec3& getColor() const {
        return m_color;
    }

    /**
     * @brief Get the emissive color with intensity for shader use.
     * @param alpha Interpolation factor for intensity.
     * @return RGBA where RGB is color and A is interpolated intensity.
     */
    glm::vec4 getColorWithIntensity(float alpha) const {
        return glm::vec4(m_color, getInterpolatedIntensity(alpha));
    }

    /**
     * @brief Get the glow hierarchy category.
     * @return Current category.
     */
    EmissiveCategory getCategory() const {
        return m_category;
    }

    /**
     * @brief Check if currently powered.
     * @return True if powered (target intensity > 0).
     */
    bool isPowered() const {
        return m_powered;
    }

    /**
     * @brief Check if transition is complete.
     * @return True if current intensity equals target intensity.
     */
    bool isTransitionComplete() const {
        return glm::abs(m_intensity.current() - m_targetIntensity) < 0.001f;
    }

private:
    Interpolatable<float> m_intensity;  ///< Interpolated intensity for smooth transitions
    float m_targetIntensity;            ///< Target intensity to transition toward
    glm::vec3 m_color;                  ///< Emissive RGB color
    EmissiveCategory m_category;        ///< Glow hierarchy category
    bool m_powered;                     ///< Current powered state
};

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * @brief Get appropriate emissive color for a building based on power state.
 * @param isPowered Whether the building is currently powered.
 * @param baseColor Base emissive color when powered.
 * @param intensity Intensity multiplier [0.0, 1.0].
 * @return RGBA color for shader (RGB = color, A = intensity).
 */
inline glm::vec4 getEmissiveColorForBuilding(bool isPowered,
                                              const glm::vec3& baseColor,
                                              float intensity = 1.0f) {
    if (!isPowered) {
        return glm::vec4(baseColor, 0.0f);  // Color preserved but no glow
    }
    float adjustedIntensity = intensity * GlowHierarchy::PLAYER_STRUCTURE_MULTIPLIER;
    return glm::vec4(baseColor, adjustedIntensity);
}

/**
 * @brief Get emissive color for terrain based on terrain type.
 * @param terrainType The terrain type.
 * @return RGBA color for shader (RGB = preset color, A = preset intensity).
 */
inline glm::vec4 getEmissiveColorForTerrain(TerrainType terrainType) {
    const auto& preset = ToonShaderConfig::instance().getTerrainEmissivePreset(terrainType);
    float adjustedIntensity = preset.intensity * GlowHierarchy::TERRAIN_MULTIPLIER;
    return glm::vec4(preset.color, adjustedIntensity);
}

/**
 * @brief Get emissive color for background elements.
 * @param baseColor Base emissive color.
 * @param intensity Base intensity [0.0, 1.0].
 * @return RGBA color for shader with background multiplier applied.
 */
inline glm::vec4 getEmissiveColorForBackground(const glm::vec3& baseColor,
                                                float intensity = 0.5f) {
    float adjustedIntensity = intensity * GlowHierarchy::BACKGROUND_MULTIPLIER;
    return glm::vec4(baseColor, adjustedIntensity);
}

} // namespace sims3000

#endif // SIMS3000_RENDER_EMISSIVE_MATERIAL_H
