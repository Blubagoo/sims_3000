/**
 * @file WaterVisualConfig.h
 * @brief CPU-side uniform buffer structure for water shader visuals.
 *
 * Defines the WaterVisualConfig struct that maps to the cbuffer WaterVisualConfig
 * in the water fragment shader. Contains colors, emissive properties, and animation
 * parameters for semi-transparent water rendering.
 *
 * Key features:
 * - Base color with alpha 0.7-0.8 for semi-transparency
 * - Per-water-type emissive colors (ocean, river, lake)
 * - Glow time for animation
 * - Flow direction for river UV scrolling
 * - Sun direction for surface highlights
 *
 * Memory layout matches HLSL cbuffer alignment requirements:
 * - float4 base_color (16 bytes)
 * - float4 ocean_emissive (16 bytes)
 * - float4 river_emissive (16 bytes)
 * - float4 lake_emissive (16 bytes)
 * - float glow_time (4 bytes)
 * - float flow_dx (4 bytes)
 * - float flow_dy (4 bytes)
 * - uint water_body_type (4 bytes)
 * - float3 sun_direction + padding (16 bytes)
 * - float ambient_strength + padding (16 bytes)
 * Total: 112 bytes (aligned to 16-byte boundary)
 *
 * @see assets/shaders/water.frag.hlsl for shader usage
 * @see WaterMesh.h for WaterBodyType enum
 */

#ifndef SIMS3000_RENDER_WATER_VISUAL_CONFIG_H
#define SIMS3000_RENDER_WATER_VISUAL_CONFIG_H

#include <sims3000/terrain/WaterMesh.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <cmath>

namespace sims3000 {

// =============================================================================
// Water Visual Constants
// =============================================================================

/**
 * @namespace WaterVisualConstants
 * @brief Visual configuration constants for water rendering.
 */
namespace WaterVisualConstants {
    // Semi-transparent alpha range (depth test ON, depth write OFF)
    constexpr float WATER_ALPHA_MIN = 0.70f;
    constexpr float WATER_ALPHA_MAX = 0.80f;
    constexpr float WATER_ALPHA_DEFAULT = 0.75f;

    // Very dark blue/teal base color (barely visible without glow)
    // sRGB values, converted to linear in shader
    constexpr float BASE_COLOR_R = 0.02f;
    constexpr float BASE_COLOR_G = 0.04f;
    constexpr float BASE_COLOR_B = 0.08f;

    // Ocean emissive: blue-white glow
    constexpr float OCEAN_EMISSIVE_R = 0.3f;
    constexpr float OCEAN_EMISSIVE_G = 0.5f;
    constexpr float OCEAN_EMISSIVE_B = 0.9f;
    constexpr float OCEAN_EMISSIVE_INTENSITY = 0.15f;

    // River emissive: teal glow (matches FlowChannel)
    constexpr float RIVER_EMISSIVE_R = 0.2f;
    constexpr float RIVER_EMISSIVE_G = 0.7f;
    constexpr float RIVER_EMISSIVE_B = 0.6f;
    constexpr float RIVER_EMISSIVE_INTENSITY = 0.18f;

    // Lake emissive: blue-white (similar to DeepVoid but calmer)
    constexpr float LAKE_EMISSIVE_R = 0.25f;
    constexpr float LAKE_EMISSIVE_G = 0.55f;
    constexpr float LAKE_EMISSIVE_B = 0.85f;
    constexpr float LAKE_EMISSIVE_INTENSITY = 0.12f;

    // Default sun direction (pointing down-right for typical isometric view)
    constexpr float SUN_DIR_X = 0.577f;   // Normalized (1, 1, 1) / sqrt(3)
    constexpr float SUN_DIR_Y = 0.577f;
    constexpr float SUN_DIR_Z = 0.577f;

    // Default ambient strength
    constexpr float AMBIENT_STRENGTH = 0.4f;

    // Animation periods (for reference - actual logic is in shader)
    constexpr float OCEAN_PULSE_PERIOD = 6.0f;   // Slow pulse
    constexpr float LAKE_PULSE_PERIOD = 8.0f;    // Gentle pulse
    // Rivers: no pulse (flow handles visual interest)
}

/**
 * @struct WaterVisualConfig
 * @brief GPU uniform buffer structure for water shader visuals.
 *
 * This struct is uploaded to the GPU as a uniform buffer and read by
 * the water fragment shader. It contains colors, emissive properties,
 * and animation parameters.
 *
 * Layout is designed to match HLSL cbuffer packing rules:
 * - Each float4 is 16-byte aligned
 * - Structs are padded to 16-byte boundaries
 */
struct alignas(16) WaterVisualConfig {
    // =========================================================================
    // Base Color (16 bytes)
    // =========================================================================

    /**
     * @brief Base water color with alpha for transparency.
     *
     * Very dark blue/teal that is barely visible without glow.
     * Alpha should be 0.70-0.80 for semi-transparency.
     */
    glm::vec4 base_color;

    // =========================================================================
    // Per-Water-Type Emissive Colors (48 bytes)
    // =========================================================================

    /**
     * @brief Ocean (DeepVoid) emissive color.
     *
     * Blue-white glow, slow pulse (6s period).
     * Alpha contains base emissive intensity.
     */
    glm::vec4 ocean_emissive;

    /**
     * @brief River (FlowChannel) emissive color.
     *
     * Teal glow, no pulse (flow animation provides visual interest).
     * Alpha contains base emissive intensity.
     */
    glm::vec4 river_emissive;

    /**
     * @brief Lake (StillBasin) emissive color.
     *
     * Blue-white glow (calmer than ocean), gentle pulse (8s period).
     * Alpha contains base emissive intensity.
     */
    glm::vec4 lake_emissive;

    // =========================================================================
    // Animation Parameters (16 bytes)
    // =========================================================================

    /**
     * @brief Animation time for glow effects (seconds).
     *
     * Updated each frame from the simulation clock.
     * Used for pulse animations and UV scrolling.
     */
    float glow_time;

    /**
     * @brief Flow direction X component (-1, 0, 1).
     *
     * Used for river UV scrolling. Set per-draw-call based on
     * the flow direction of the river being rendered.
     */
    float flow_dx;

    /**
     * @brief Flow direction Y component (-1, 0, 1).
     *
     * Used for river UV scrolling. Maps to FlowDirection enum.
     */
    float flow_dy;

    /**
     * @brief Water body type (0=Ocean, 1=River, 2=Lake).
     *
     * Set per-draw-call to select appropriate visual treatment.
     */
    std::uint32_t water_body_type;

    // =========================================================================
    // Lighting Parameters (16 bytes)
    // =========================================================================

    /**
     * @brief Sun direction for surface highlights.
     *
     * Normalized world-space light direction vector.
     */
    glm::vec3 sun_direction;
    float _padding1;

    // =========================================================================
    // Ambient Parameters (16 bytes)
    // =========================================================================

    /**
     * @brief Ambient lighting strength (0.0-1.0).
     *
     * Controls how much ambient light affects the water surface.
     */
    float ambient_strength;
    glm::vec3 _padding2;

    // =========================================================================
    // Constructors
    // =========================================================================

    /**
     * @brief Default constructor - initializes to default water colors.
     */
    WaterVisualConfig()
        : base_color(
            WaterVisualConstants::BASE_COLOR_R,
            WaterVisualConstants::BASE_COLOR_G,
            WaterVisualConstants::BASE_COLOR_B,
            WaterVisualConstants::WATER_ALPHA_DEFAULT
        )
        , ocean_emissive(
            WaterVisualConstants::OCEAN_EMISSIVE_R,
            WaterVisualConstants::OCEAN_EMISSIVE_G,
            WaterVisualConstants::OCEAN_EMISSIVE_B,
            WaterVisualConstants::OCEAN_EMISSIVE_INTENSITY
        )
        , river_emissive(
            WaterVisualConstants::RIVER_EMISSIVE_R,
            WaterVisualConstants::RIVER_EMISSIVE_G,
            WaterVisualConstants::RIVER_EMISSIVE_B,
            WaterVisualConstants::RIVER_EMISSIVE_INTENSITY
        )
        , lake_emissive(
            WaterVisualConstants::LAKE_EMISSIVE_R,
            WaterVisualConstants::LAKE_EMISSIVE_G,
            WaterVisualConstants::LAKE_EMISSIVE_B,
            WaterVisualConstants::LAKE_EMISSIVE_INTENSITY
        )
        , glow_time(0.0f)
        , flow_dx(0.0f)
        , flow_dy(0.0f)
        , water_body_type(0)
        , sun_direction(
            WaterVisualConstants::SUN_DIR_X,
            WaterVisualConstants::SUN_DIR_Y,
            WaterVisualConstants::SUN_DIR_Z
        )
        , _padding1(0.0f)
        , ambient_strength(WaterVisualConstants::AMBIENT_STRENGTH)
        , _padding2(0.0f)
    {}

    // =========================================================================
    // Setters - Animation
    // =========================================================================

    /**
     * @brief Set the animation time.
     * @param time Time in seconds since start.
     */
    void setGlowTime(float time) {
        glow_time = time;
    }

    /**
     * @brief Set the flow direction for river rendering.
     *
     * Uses FlowDirection enum to determine UV scroll direction.
     *
     * @param dir Flow direction enum value.
     */
    void setFlowDirection(terrain::FlowDirection dir) {
        flow_dx = static_cast<float>(terrain::getFlowDirectionDX(dir));
        flow_dy = static_cast<float>(terrain::getFlowDirectionDY(dir));
    }

    /**
     * @brief Set the water body type for the current draw call.
     * @param type Water body type (Ocean=0, River=1, Lake=2).
     */
    void setWaterBodyType(terrain::WaterBodyType type) {
        water_body_type = static_cast<std::uint32_t>(type);
    }

    // =========================================================================
    // Setters - Lighting
    // =========================================================================

    /**
     * @brief Set the sun direction.
     * @param direction Normalized world-space light direction.
     */
    void setSunDirection(const glm::vec3& direction) {
        sun_direction = glm::normalize(direction);
    }

    /**
     * @brief Set the ambient lighting strength.
     * @param strength Ambient strength (0.0-1.0).
     */
    void setAmbientStrength(float strength) {
        ambient_strength = glm::clamp(strength, 0.0f, 1.0f);
    }

    // =========================================================================
    // Setters - Colors
    // =========================================================================

    /**
     * @brief Set the base water color.
     * @param color RGB color (very dark blue/teal recommended).
     */
    void setBaseColor(const glm::vec3& color) {
        base_color = glm::vec4(color, base_color.a);
    }

    /**
     * @brief Set the water transparency alpha.
     * @param alpha Alpha value (0.70-0.80 recommended).
     */
    void setAlpha(float alpha) {
        base_color.a = glm::clamp(alpha, 0.0f, 1.0f);
    }

    /**
     * @brief Set the ocean emissive color and intensity.
     * @param color RGB emissive color.
     * @param intensity Glow intensity (0.0-1.0).
     */
    void setOceanEmissive(const glm::vec3& color, float intensity) {
        ocean_emissive = glm::vec4(color, intensity);
    }

    /**
     * @brief Set the river emissive color and intensity.
     * @param color RGB emissive color.
     * @param intensity Glow intensity (0.0-1.0).
     */
    void setRiverEmissive(const glm::vec3& color, float intensity) {
        river_emissive = glm::vec4(color, intensity);
    }

    /**
     * @brief Set the lake emissive color and intensity.
     * @param color RGB emissive color.
     * @param intensity Glow intensity (0.0-1.0).
     */
    void setLakeEmissive(const glm::vec3& color, float intensity) {
        lake_emissive = glm::vec4(color, intensity);
    }

    // =========================================================================
    // Getters
    // =========================================================================

    /**
     * @brief Get the size of this struct for GPU upload.
     * @return Size in bytes.
     */
    static constexpr std::size_t getGPUSize() {
        return sizeof(WaterVisualConfig);
    }

    /**
     * @brief Get pointer to data for GPU upload.
     * @return Pointer to beginning of struct.
     */
    const void* getData() const {
        return this;
    }

    /**
     * @brief Get the current water alpha.
     * @return Alpha value.
     */
    float getAlpha() const {
        return base_color.a;
    }
};

// Verify size matches expected GPU buffer size
// 4 * float4 (colors) + float4 (animation) + float4 (sun) + float4 (ambient)
// = 16 * 4 + 16 + 16 + 16 = 112 bytes
static_assert(sizeof(WaterVisualConfig) == 112,
    "WaterVisualConfig must be exactly 112 bytes for GPU uniform buffer");

// Verify alignment for GPU upload
static_assert(alignof(WaterVisualConfig) >= 16,
    "WaterVisualConfig must be at least 16-byte aligned for GPU uniform buffer");

// =============================================================================
// Flow Direction to UV Scroll Velocity Helpers
// =============================================================================

/**
 * @brief Get UV scroll velocity for a flow direction.
 *
 * Maps FlowDirection enum to 2D velocity vector for UV scrolling.
 * Used to update WaterVisualConfig before rendering rivers.
 *
 * @param dir Flow direction.
 * @return vec2 with (dx, dy) components.
 */
inline glm::vec2 getFlowVelocity(terrain::FlowDirection dir) {
    return glm::vec2(
        static_cast<float>(terrain::getFlowDirectionDX(dir)),
        static_cast<float>(terrain::getFlowDirectionDY(dir))
    );
}

// =============================================================================
// Water Rendering State Configuration
// =============================================================================

/**
 * @namespace WaterRenderState
 * @brief Recommended render state for water rendering.
 *
 * Water should be rendered with:
 * - Depth test: ON (compare less)
 * - Depth write: OFF (terrain beneath visible)
 * - Blend mode: Standard alpha blend
 * - Cull mode: Back-face culling
 *
 * Use BlendState::transparent() and DepthState::transparent() for water.
 */
namespace WaterRenderState {
    // Depth test ON, depth write OFF
    constexpr bool DEPTH_TEST_ENABLED = true;
    constexpr bool DEPTH_WRITE_ENABLED = false;

    // Standard alpha blending
    constexpr bool BLEND_ENABLED = true;

    // Back-face culling (water surface is single-sided)
    constexpr bool CULL_BACK_FACE = true;
}

} // namespace sims3000

#endif // SIMS3000_RENDER_WATER_VISUAL_CONFIG_H
