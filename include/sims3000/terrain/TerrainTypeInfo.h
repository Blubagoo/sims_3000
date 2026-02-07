/**
 * @file TerrainTypeInfo.h
 * @brief Static lookup table for per-terrain-type gameplay and rendering properties
 *
 * This is the single source of truth for terrain type properties used by both:
 * - TerrainSystem (gameplay): buildable, clearable, costs, modifiers
 * - RenderingSystem (visuals): emissive color, emissive intensity
 *
 * COLOR VALUES: Edit /docs/color-tokens.yaml to change terrain colors,
 * then update the RGB values in TERRAIN_INFO array below to match.
 *
 * Emissive intensity hierarchy (0.05 to 0.60):
 * - Substrate (0.05) - Background, nearly invisible
 * - Ridge/DeepVoid/StillBasin (0.10) - Subtle terrain features
 * - FlowChannel (0.12) - Active environmental
 * - BiolumeGrove (0.25) - Notable biome
 * - SporeFlats/BlightMires (0.30) - Vibrant/hazard biomes
 * - EmberCrust (0.35) - Warm glow feature
 * - PrismaFields (0.60) - Maximum terrain glow, landmark
 *
 * @see /docs/color-tokens.yaml for editable color hex values
 * @see /docs/canon/terrain-visuals.yaml for full visual specification
 * @see TerrainTypes.h for TerrainType enum
 */

#ifndef SIMS3000_TERRAIN_TERRAINTYPEINFO_H
#define SIMS3000_TERRAIN_TERRAINTYPEINFO_H

#include <sims3000/terrain/TerrainTypes.h>
#include <cstdint>

namespace sims3000 {
namespace terrain {

/**
 * @struct Vec3
 * @brief Simple 3-component vector for emissive color (RGB, normalized 0-1).
 *
 * We define this locally to avoid pulling in glm as a dependency for
 * this header-only lookup table.
 */
struct Vec3 {
    float x;
    float y;
    float z;

    constexpr Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

/**
 * @struct TerrainTypeInfo
 * @brief Per-terrain-type gameplay and rendering properties.
 *
 * This struct defines all properties needed by both gameplay systems
 * and rendering systems for each terrain type.
 */
struct TerrainTypeInfo {
    // =========================================================================
    // Gameplay Properties
    // =========================================================================

    /// Whether structures can be placed directly on this terrain type.
    /// If false, the tile cannot be built upon (even if cleared).
    bool buildable;

    /// Whether this terrain type can be cleared (purged) to allow building.
    /// Clearing removes vegetation/features but doesn't change terrain type.
    bool clearable;

    /// Whether this terrain type naturally generates contamination.
    /// Contamination spreads to nearby tiles and reduces land value.
    bool generates_contamination;

    /// Cost in credits to clear this terrain type.
    /// Negative values indicate revenue from clearing (e.g., crystal harvesting).
    std::int32_t clear_cost;

    /// Revenue generated when clearing this terrain type.
    /// Typically 0 unless clearing yields valuable resources.
    std::int32_t clear_revenue;

    /// Multiplier applied to building construction costs on/near this terrain.
    /// 1.0 = normal cost, 1.5 = 50% more expensive, etc.
    float build_cost_modifier;

    /// Bonus applied to sector land value for tiles on/near this terrain.
    /// Positive values increase desirability.
    std::int32_t value_bonus;

    /// Bonus applied to harmony (happiness) for habitation near this terrain.
    /// Positive values increase resident satisfaction.
    std::int32_t harmony_bonus;

    /// Contamination units generated per simulation tick.
    /// Only meaningful for terrain types where generates_contamination is true.
    /// Value of 0 for non-contaminating terrain types.
    std::uint32_t contamination_per_tick;

    // =========================================================================
    // Rendering Properties
    // =========================================================================

    /// Emissive color (RGB, normalized 0-1) for terrain glow.
    /// Sourced from /docs/canon/terrain-visuals.yaml.
    Vec3 emissive_color;

    /// Emissive intensity (0.0 to 1.0) for terrain glow strength.
    /// Range for terrain: 0.05 (Substrate) to 0.60 (PrismaFields).
    /// Buildings use 0.5-1.0 to remain visually dominant.
    float emissive_intensity;
};

// =============================================================================
// Helper: Convert RGB 0-255 to normalized 0-1
// =============================================================================

/**
 * @brief Convert RGB values from 0-255 range to normalized 0-1 range.
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return Vec3 with normalized RGB values
 */
constexpr Vec3 rgb(float r, float g, float b) {
    return Vec3(r / 255.0f, g / 255.0f, b / 255.0f);
}

// =============================================================================
// Static Lookup Table: TERRAIN_INFO
// =============================================================================

/**
 * @brief Static lookup table of terrain type properties.
 *
 * Indexed by TerrainType enum value (0-9).
 * Access: TERRAIN_INFO[static_cast<size_t>(TerrainType::Substrate)]
 *
 * Property sources:
 * - Color values: /docs/color-tokens.yaml (USER EDITABLE)
 * - Full visual spec: /docs/canon/terrain-visuals.yaml
 * - Gameplay values: Game Designer initial estimates (tunable)
 *
 * To update colors: Edit color-tokens.yaml, convert hex to RGB,
 * then update the rgb() values below.
 */
constexpr TerrainTypeInfo TERRAIN_INFO[TERRAIN_TYPE_COUNT] = {
    // -------------------------------------------------------------------------
    // [0] Substrate - Standard buildable terrain (flat ground)
    // -------------------------------------------------------------------------
    // Emissive: #1a1a2e (26, 26, 46), intensity 0.05
    {
        /* buildable */               true,
        /* clearable */               false,
        /* generates_contamination */ false,
        /* clear_cost */              0,
        /* clear_revenue */           0,
        /* build_cost_modifier */     1.0f,
        /* value_bonus */             0,
        /* harmony_bonus */           0,
        /* contamination_per_tick */  0,
        /* emissive_color */          rgb(26.0f, 26.0f, 46.0f),
        /* emissive_intensity */      0.05f
    },

    // -------------------------------------------------------------------------
    // [1] Ridge - Elevated terrain (hills)
    // -------------------------------------------------------------------------
    // Emissive: #3d2d5c (61, 45, 92), intensity 0.10
    // Note: terrain-visuals.yaml lists #2e1a2e but updated to match
    // the hue_assignments section which uses #3d2d5c
    {
        /* buildable */               false,
        /* clearable */               false,
        /* generates_contamination */ false,
        /* clear_cost */              0,
        /* clear_revenue */           0,
        /* build_cost_modifier */     1.0f,
        /* value_bonus */             0,
        /* harmony_bonus */           0,
        /* contamination_per_tick */  0,
        /* emissive_color */          rgb(61.0f, 45.0f, 92.0f),
        /* emissive_intensity */      0.10f
    },

    // -------------------------------------------------------------------------
    // [2] DeepVoid - Map-edge deep water (ocean)
    // -------------------------------------------------------------------------
    // Emissive: #0066aa (0, 102, 170), intensity 0.10
    // Note: Ticket notes say #1a4a6e (26, 74, 110) but terrain-visuals.yaml
    // specifies #0066aa. Using canon value from terrain-visuals.yaml.
    {
        /* buildable */               false,
        /* clearable */               false,
        /* generates_contamination */ false,
        /* clear_cost */              0,
        /* clear_revenue */           0,
        /* build_cost_modifier */     1.0f,
        /* value_bonus */             5,   // Water proximity bonus
        /* harmony_bonus */           2,
        /* contamination_per_tick */  0,
        /* emissive_color */          rgb(0.0f, 102.0f, 170.0f),
        /* emissive_intensity */      0.10f
    },

    // -------------------------------------------------------------------------
    // [3] FlowChannel - Flowing water (river)
    // -------------------------------------------------------------------------
    // Emissive: #00aaaa (0, 170, 170), intensity 0.12
    // Note: Ticket notes say #1a5a5a (26, 90, 90) but terrain-visuals.yaml
    // specifies #00aaaa. Using canon value from terrain-visuals.yaml.
    {
        /* buildable */               false,
        /* clearable */               false,
        /* generates_contamination */ false,
        /* clear_cost */              0,
        /* clear_revenue */           0,
        /* build_cost_modifier */     1.0f,
        /* value_bonus */             8,   // Flowing water higher bonus
        /* harmony_bonus */           3,
        /* contamination_per_tick */  0,
        /* emissive_color */          rgb(0.0f, 170.0f, 170.0f),
        /* emissive_intensity */      0.12f
    },

    // -------------------------------------------------------------------------
    // [4] StillBasin - Inland water body (lake)
    // -------------------------------------------------------------------------
    // Emissive: #4488cc (68, 136, 204), intensity 0.10
    // Note: Ticket notes say #3a4a6e (58, 74, 110) but terrain-visuals.yaml
    // specifies #4488cc. Using canon value from terrain-visuals.yaml.
    {
        /* buildable */               false,
        /* clearable */               false,
        /* generates_contamination */ false,
        /* clear_cost */              0,
        /* clear_revenue */           0,
        /* build_cost_modifier */     1.0f,
        /* value_bonus */             6,   // Lake bonus between ocean and river
        /* harmony_bonus */           4,   // Calm lake = higher harmony
        /* contamination_per_tick */  0,
        /* emissive_color */          rgb(68.0f, 136.0f, 204.0f),
        /* emissive_intensity */      0.10f
    },

    // -------------------------------------------------------------------------
    // [5] BiolumeGrove - Alien vegetation cluster (forest)
    // -------------------------------------------------------------------------
    // Emissive: #00ff88 (0, 255, 136), intensity 0.25
    // Note: Ticket notes say #1a6e5a (26, 110, 90) but terrain-visuals.yaml
    // specifies #00ff88. Using canon value from terrain-visuals.yaml.
    {
        /* buildable */               false,
        /* clearable */               true,
        /* generates_contamination */ false,
        /* clear_cost */              100,
        /* clear_revenue */           0,
        /* build_cost_modifier */     1.0f,
        /* value_bonus */             10,  // Natural beauty bonus
        /* harmony_bonus */           5,
        /* contamination_per_tick */  0,
        /* emissive_color */          rgb(0.0f, 255.0f, 136.0f),
        /* emissive_intensity */      0.25f
    },

    // -------------------------------------------------------------------------
    // [6] PrismaFields - Luminous crystal formations
    // -------------------------------------------------------------------------
    // Emissive: #ff00ff (255, 0, 255), intensity 0.60 (MAXIMUM terrain glow)
    // Note: Ticket notes say #8e1a6e (142, 26, 110) but terrain-visuals.yaml
    // specifies #ff00ff. Using canon value from terrain-visuals.yaml.
    {
        /* buildable */               false,
        /* clearable */               true,
        /* generates_contamination */ false,
        /* clear_cost */              -500, // Negative = revenue from harvesting
        /* clear_revenue */           500,  // Crystal harvesting yields credits
        /* build_cost_modifier */     1.0f,
        /* value_bonus */             20,   // Rare landmark bonus
        /* harmony_bonus */           8,
        /* contamination_per_tick */  0,
        /* emissive_color */          rgb(255.0f, 0.0f, 255.0f),
        /* emissive_intensity */      0.60f
    },

    // -------------------------------------------------------------------------
    // [7] SporeFlats - Bioluminescent spore flora
    // -------------------------------------------------------------------------
    // Emissive: #88ff44 (136, 255, 68), intensity 0.30
    // Note: Ticket notes say #6e8e1a (110, 142, 26) but terrain-visuals.yaml
    // specifies #88ff44. Using canon value from terrain-visuals.yaml.
    {
        /* buildable */               false,
        /* clearable */               true,
        /* generates_contamination */ false,
        /* clear_cost */              50,
        /* clear_revenue */           0,
        /* build_cost_modifier */     1.0f,
        /* value_bonus */             5,
        /* harmony_bonus */           6,   // Pleasing visual effect
        /* contamination_per_tick */  0,
        /* emissive_color */          rgb(136.0f, 255.0f, 68.0f),
        /* emissive_intensity */      0.30f
    },

    // -------------------------------------------------------------------------
    // [8] BlightMires - Chemical runoff pools (toxic marshes)
    // -------------------------------------------------------------------------
    // Emissive: #aaff00 (170, 255, 0), intensity 0.30
    // Note: Ticket notes say #5a8e1a (90, 142, 26) but terrain-visuals.yaml
    // specifies #aaff00. Using canon value from terrain-visuals.yaml.
    {
        /* buildable */               false,
        /* clearable */               false,
        /* generates_contamination */ true,
        /* clear_cost */              0,
        /* clear_revenue */           0,
        /* build_cost_modifier */     1.0f,
        /* value_bonus */             -15, // Toxic = negative land value
        /* harmony_bonus */           -10, // Unhealthy environment
        /* contamination_per_tick */  5,   // Contamination output per tick
        /* emissive_color */          rgb(170.0f, 255.0f, 0.0f),
        /* emissive_intensity */      0.30f
    },

    // -------------------------------------------------------------------------
    // [9] EmberCrust - Hardened volcanic terrain
    // -------------------------------------------------------------------------
    // Emissive: #ff4400 (255, 68, 0), intensity 0.35
    // Note: Ticket notes say #8e4a1a (142, 74, 26) but terrain-visuals.yaml
    // specifies #ff4400. Using canon value from terrain-visuals.yaml.
    {
        /* buildable */               false,
        /* clearable */               false,
        /* generates_contamination */ false,
        /* clear_cost */              0,
        /* clear_revenue */           0,
        /* build_cost_modifier */     1.5f, // Nearby buildings cost 50% more
        /* value_bonus */             3,    // Geothermal potential
        /* harmony_bonus */           0,
        /* contamination_per_tick */  0,
        /* emissive_color */          rgb(255.0f, 68.0f, 0.0f),
        /* emissive_intensity */      0.35f
    }
};

// =============================================================================
// Accessor Functions
// =============================================================================

/**
 * @brief Get terrain type info by TerrainType enum.
 * @param type The terrain type to look up.
 * @return Reference to the TerrainTypeInfo for the given type.
 */
inline const TerrainTypeInfo& getTerrainInfo(TerrainType type) {
    return TERRAIN_INFO[static_cast<std::size_t>(type)];
}

/**
 * @brief Get terrain type info by raw uint8 index.
 * @param index The terrain type index (0-9).
 * @return Reference to the TerrainTypeInfo for the given index.
 * @note Caller must ensure index < TERRAIN_TYPE_COUNT.
 */
inline const TerrainTypeInfo& getTerrainInfo(std::uint8_t index) {
    return TERRAIN_INFO[index];
}

/**
 * @brief Check if a terrain type is buildable.
 * @param type The terrain type to check.
 * @return true if structures can be placed on this terrain.
 */
inline bool isBuildable(TerrainType type) {
    return TERRAIN_INFO[static_cast<std::size_t>(type)].buildable;
}

/**
 * @brief Check if a terrain type is clearable.
 * @param type The terrain type to check.
 * @return true if this terrain can be cleared (purged).
 */
inline bool isClearable(TerrainType type) {
    return TERRAIN_INFO[static_cast<std::size_t>(type)].clearable;
}

/**
 * @brief Check if a terrain type generates contamination.
 * @param type The terrain type to check.
 * @return true if this terrain naturally generates contamination.
 */
inline bool generatesContamination(TerrainType type) {
    return TERRAIN_INFO[static_cast<std::size_t>(type)].generates_contamination;
}

/**
 * @brief Get the emissive color for a terrain type.
 * @param type The terrain type.
 * @return Vec3 RGB color (normalized 0-1).
 */
inline Vec3 getEmissiveColor(TerrainType type) {
    return TERRAIN_INFO[static_cast<std::size_t>(type)].emissive_color;
}

/**
 * @brief Get the emissive intensity for a terrain type.
 * @param type The terrain type.
 * @return Intensity value (0.0 to 1.0).
 */
inline float getEmissiveIntensity(TerrainType type) {
    return TERRAIN_INFO[static_cast<std::size_t>(type)].emissive_intensity;
}

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINTYPEINFO_H
