/**
 * @file test_terrain_type_info.cpp
 * @brief Unit tests for TerrainTypeInfo.h (Ticket 3-003)
 *
 * Tests cover:
 * - TerrainTypeInfo struct field definitions
 * - TERRAIN_INFO static array completeness (10 entries)
 * - Emissive intensity hierarchy validation
 * - Gameplay property consistency
 * - Accessor function correctness
 *
 * Acceptance Criteria:
 * - TerrainTypeInfo struct defined with all fields
 * - Static array TERRAIN_INFO[10] populated with design values
 * - Emissive intensity hierarchy: max=0.60 (PrismaFields), min=0.05 (Substrate)
 * - Game Designer-approved values for all gameplay modifiers
 */

#include <sims3000/terrain/TerrainTypeInfo.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace sims3000::terrain;

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n  FAILED: %s (line %d)\n", #condition, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// Floating point comparison with tolerance
#define ASSERT_FLOAT_EQ(a, b) do { \
    if (std::abs((a) - (b)) > 0.0001f) { \
        printf("\n  FAILED: %s == %s (line %d) [%f != %f]\n", #a, #b, __LINE__, (float)(a), (float)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Vec3 Helper Tests
// =============================================================================

TEST(vec3_default_construction) {
    Vec3 v;
    ASSERT_FLOAT_EQ(v.x, 0.0f);
    ASSERT_FLOAT_EQ(v.y, 0.0f);
    ASSERT_FLOAT_EQ(v.z, 0.0f);
}

TEST(vec3_value_construction) {
    Vec3 v(0.5f, 0.25f, 1.0f);
    ASSERT_FLOAT_EQ(v.x, 0.5f);
    ASSERT_FLOAT_EQ(v.y, 0.25f);
    ASSERT_FLOAT_EQ(v.z, 1.0f);
}

TEST(rgb_helper_normalization) {
    // rgb(255, 128, 0) should become (1.0, ~0.502, 0.0)
    Vec3 color = rgb(255.0f, 128.0f, 0.0f);
    ASSERT_FLOAT_EQ(color.x, 1.0f);
    ASSERT_FLOAT_EQ(color.y, 128.0f / 255.0f);
    ASSERT_FLOAT_EQ(color.z, 0.0f);
}

TEST(rgb_helper_black) {
    Vec3 color = rgb(0.0f, 0.0f, 0.0f);
    ASSERT_FLOAT_EQ(color.x, 0.0f);
    ASSERT_FLOAT_EQ(color.y, 0.0f);
    ASSERT_FLOAT_EQ(color.z, 0.0f);
}

TEST(rgb_helper_white) {
    Vec3 color = rgb(255.0f, 255.0f, 255.0f);
    ASSERT_FLOAT_EQ(color.x, 1.0f);
    ASSERT_FLOAT_EQ(color.y, 1.0f);
    ASSERT_FLOAT_EQ(color.z, 1.0f);
}

// =============================================================================
// TERRAIN_INFO Array Completeness Tests
// =============================================================================

TEST(terrain_info_array_size) {
    // Verify array has exactly TERRAIN_TYPE_COUNT (10) entries
    // This is checked at compile time by the array declaration,
    // but we verify it's accessible at runtime
    ASSERT_EQ(sizeof(TERRAIN_INFO) / sizeof(TERRAIN_INFO[0]), 10u);
}

TEST(terrain_info_indexed_by_enum) {
    // Verify each terrain type maps to correct array index
    ASSERT_EQ(&TERRAIN_INFO[0], &TERRAIN_INFO[static_cast<size_t>(TerrainType::Substrate)]);
    ASSERT_EQ(&TERRAIN_INFO[1], &TERRAIN_INFO[static_cast<size_t>(TerrainType::Ridge)]);
    ASSERT_EQ(&TERRAIN_INFO[2], &TERRAIN_INFO[static_cast<size_t>(TerrainType::DeepVoid)]);
    ASSERT_EQ(&TERRAIN_INFO[3], &TERRAIN_INFO[static_cast<size_t>(TerrainType::FlowChannel)]);
    ASSERT_EQ(&TERRAIN_INFO[4], &TERRAIN_INFO[static_cast<size_t>(TerrainType::StillBasin)]);
    ASSERT_EQ(&TERRAIN_INFO[5], &TERRAIN_INFO[static_cast<size_t>(TerrainType::BiolumeGrove)]);
    ASSERT_EQ(&TERRAIN_INFO[6], &TERRAIN_INFO[static_cast<size_t>(TerrainType::PrismaFields)]);
    ASSERT_EQ(&TERRAIN_INFO[7], &TERRAIN_INFO[static_cast<size_t>(TerrainType::SporeFlats)]);
    ASSERT_EQ(&TERRAIN_INFO[8], &TERRAIN_INFO[static_cast<size_t>(TerrainType::BlightMires)]);
    ASSERT_EQ(&TERRAIN_INFO[9], &TERRAIN_INFO[static_cast<size_t>(TerrainType::EmberCrust)]);
}

// =============================================================================
// Emissive Intensity Hierarchy Tests
// =============================================================================

TEST(emissive_intensity_substrate_minimum) {
    // Substrate should have minimum terrain intensity (0.05)
    ASSERT_FLOAT_EQ(TERRAIN_INFO[0].emissive_intensity, 0.05f);
}

TEST(emissive_intensity_prisma_maximum) {
    // PrismaFields should have maximum terrain intensity (0.60)
    ASSERT_FLOAT_EQ(TERRAIN_INFO[6].emissive_intensity, 0.60f);
}

TEST(emissive_intensity_hierarchy_order) {
    // Verify intensity hierarchy:
    // substrate(0.05) < ridge(0.10) = deep_void(0.10) = still_basin(0.10)
    // < flow_channel(0.12) < grove(0.25) < spore(0.30) = blight(0.30)
    // < ember(0.35) < prisma(0.60)

    float substrate = TERRAIN_INFO[0].emissive_intensity;
    float ridge = TERRAIN_INFO[1].emissive_intensity;
    float deep_void = TERRAIN_INFO[2].emissive_intensity;
    float flow_channel = TERRAIN_INFO[3].emissive_intensity;
    float still_basin = TERRAIN_INFO[4].emissive_intensity;
    float biolume_grove = TERRAIN_INFO[5].emissive_intensity;
    float prisma_fields = TERRAIN_INFO[6].emissive_intensity;
    float spore_flats = TERRAIN_INFO[7].emissive_intensity;
    float blight_mires = TERRAIN_INFO[8].emissive_intensity;
    float ember_crust = TERRAIN_INFO[9].emissive_intensity;

    // Substrate is minimum
    ASSERT(substrate < ridge);
    ASSERT(substrate < deep_void);

    // Ridge/water types at 0.10
    ASSERT_FLOAT_EQ(ridge, 0.10f);
    ASSERT_FLOAT_EQ(deep_void, 0.10f);
    ASSERT_FLOAT_EQ(still_basin, 0.10f);

    // FlowChannel slightly higher (active)
    ASSERT(flow_channel > ridge);
    ASSERT_FLOAT_EQ(flow_channel, 0.12f);

    // BiolumeGrove is notable
    ASSERT(biolume_grove > flow_channel);
    ASSERT_FLOAT_EQ(biolume_grove, 0.25f);

    // Spore and Blight are vibrant/hazard
    ASSERT(spore_flats > biolume_grove);
    ASSERT_FLOAT_EQ(spore_flats, 0.30f);
    ASSERT_FLOAT_EQ(blight_mires, 0.30f);

    // EmberCrust warm glow
    ASSERT(ember_crust > spore_flats);
    ASSERT_FLOAT_EQ(ember_crust, 0.35f);

    // PrismaFields is maximum
    ASSERT(prisma_fields > ember_crust);
    ASSERT_FLOAT_EQ(prisma_fields, 0.60f);
}

TEST(emissive_intensity_all_in_valid_range) {
    // All terrain intensities must be in [0.0, 1.0]
    // Buildings use 0.5-1.0, terrain uses 0.05-0.60
    for (std::uint8_t i = 0; i < TERRAIN_TYPE_COUNT; ++i) {
        float intensity = TERRAIN_INFO[i].emissive_intensity;
        ASSERT(intensity >= 0.0f);
        ASSERT(intensity <= 1.0f);
        // Terrain should be <= 0.60 to stay below building glow
        ASSERT(intensity <= 0.60f);
    }
}

// =============================================================================
// Emissive Color Tests
// =============================================================================

TEST(emissive_color_substrate) {
    // Substrate: #1a1a2e (26, 26, 46) normalized
    Vec3 color = TERRAIN_INFO[0].emissive_color;
    ASSERT_FLOAT_EQ(color.x, 26.0f / 255.0f);
    ASSERT_FLOAT_EQ(color.y, 26.0f / 255.0f);
    ASSERT_FLOAT_EQ(color.z, 46.0f / 255.0f);
}

TEST(emissive_color_prisma_fields) {
    // PrismaFields: #ff00ff (255, 0, 255) - bright magenta
    Vec3 color = TERRAIN_INFO[6].emissive_color;
    ASSERT_FLOAT_EQ(color.x, 1.0f);
    ASSERT_FLOAT_EQ(color.y, 0.0f);
    ASSERT_FLOAT_EQ(color.z, 1.0f);
}

TEST(emissive_color_ember_crust) {
    // EmberCrust: #ff4400 (255, 68, 0) - orange-red
    Vec3 color = TERRAIN_INFO[9].emissive_color;
    ASSERT_FLOAT_EQ(color.x, 1.0f);
    ASSERT_FLOAT_EQ(color.y, 68.0f / 255.0f);
    ASSERT_FLOAT_EQ(color.z, 0.0f);
}

TEST(emissive_colors_all_normalized) {
    // All color components must be in [0.0, 1.0]
    for (std::uint8_t i = 0; i < TERRAIN_TYPE_COUNT; ++i) {
        Vec3 color = TERRAIN_INFO[i].emissive_color;
        ASSERT(color.x >= 0.0f && color.x <= 1.0f);
        ASSERT(color.y >= 0.0f && color.y <= 1.0f);
        ASSERT(color.z >= 0.0f && color.z <= 1.0f);
    }
}

// =============================================================================
// Gameplay Property Tests - Buildable
// =============================================================================

TEST(buildable_substrate_true) {
    // Substrate is the primary buildable terrain
    ASSERT(TERRAIN_INFO[0].buildable == true);
}

TEST(buildable_ridge_false) {
    // Ridge (hills) is not buildable
    ASSERT(TERRAIN_INFO[1].buildable == false);
}

TEST(buildable_water_types_false) {
    // All water types should be non-buildable
    ASSERT(TERRAIN_INFO[2].buildable == false);  // DeepVoid
    ASSERT(TERRAIN_INFO[3].buildable == false);  // FlowChannel
    ASSERT(TERRAIN_INFO[4].buildable == false);  // StillBasin
}

TEST(buildable_biomes_false) {
    // Biomes require clearing first (or are unbuildable)
    ASSERT(TERRAIN_INFO[5].buildable == false);  // BiolumeGrove
    ASSERT(TERRAIN_INFO[6].buildable == false);  // PrismaFields
    ASSERT(TERRAIN_INFO[7].buildable == false);  // SporeFlats
    ASSERT(TERRAIN_INFO[8].buildable == false);  // BlightMires
    ASSERT(TERRAIN_INFO[9].buildable == false);  // EmberCrust
}

// =============================================================================
// Gameplay Property Tests - Clearable
// =============================================================================

TEST(clearable_substrate_false) {
    // Substrate has nothing to clear
    ASSERT(TERRAIN_INFO[0].clearable == false);
}

TEST(clearable_ridge_false) {
    // Ridge cannot be cleared (terrain feature)
    ASSERT(TERRAIN_INFO[1].clearable == false);
}

TEST(clearable_water_types_false) {
    // Water cannot be cleared
    ASSERT(TERRAIN_INFO[2].clearable == false);  // DeepVoid
    ASSERT(TERRAIN_INFO[3].clearable == false);  // FlowChannel
    ASSERT(TERRAIN_INFO[4].clearable == false);  // StillBasin
}

TEST(clearable_vegetation_biomes_true) {
    // Vegetation biomes can be cleared
    ASSERT(TERRAIN_INFO[5].clearable == true);  // BiolumeGrove
    ASSERT(TERRAIN_INFO[6].clearable == true);  // PrismaFields
    ASSERT(TERRAIN_INFO[7].clearable == true);  // SporeFlats
}

TEST(clearable_hazard_biomes_false) {
    // BlightMires cannot be cleared (toxic)
    ASSERT(TERRAIN_INFO[8].clearable == false);
}

TEST(clearable_ember_crust_false) {
    // EmberCrust cannot be cleared (volcanic rock)
    ASSERT(TERRAIN_INFO[9].clearable == false);
}

// =============================================================================
// Gameplay Property Tests - Contamination
// =============================================================================

TEST(contamination_only_blight_mires) {
    // Only BlightMires generates contamination
    ASSERT(TERRAIN_INFO[0].generates_contamination == false);  // Substrate
    ASSERT(TERRAIN_INFO[1].generates_contamination == false);  // Ridge
    ASSERT(TERRAIN_INFO[2].generates_contamination == false);  // DeepVoid
    ASSERT(TERRAIN_INFO[3].generates_contamination == false);  // FlowChannel
    ASSERT(TERRAIN_INFO[4].generates_contamination == false);  // StillBasin
    ASSERT(TERRAIN_INFO[5].generates_contamination == false);  // BiolumeGrove
    ASSERT(TERRAIN_INFO[6].generates_contamination == false);  // PrismaFields
    ASSERT(TERRAIN_INFO[7].generates_contamination == false);  // SporeFlats
    ASSERT(TERRAIN_INFO[8].generates_contamination == true);   // BlightMires - YES
    ASSERT(TERRAIN_INFO[9].generates_contamination == false);  // EmberCrust
}

TEST(contamination_per_tick_blight_mires) {
    // BlightMires should have non-zero contamination_per_tick
    ASSERT_EQ(TERRAIN_INFO[8].contamination_per_tick, 5);
}

TEST(contamination_per_tick_zero_for_non_contaminating) {
    // All non-contaminating terrain types should have contamination_per_tick = 0
    ASSERT_EQ(TERRAIN_INFO[0].contamination_per_tick, 0);  // Substrate
    ASSERT_EQ(TERRAIN_INFO[1].contamination_per_tick, 0);  // Ridge
    ASSERT_EQ(TERRAIN_INFO[2].contamination_per_tick, 0);  // DeepVoid
    ASSERT_EQ(TERRAIN_INFO[3].contamination_per_tick, 0);  // FlowChannel
    ASSERT_EQ(TERRAIN_INFO[4].contamination_per_tick, 0);  // StillBasin
    ASSERT_EQ(TERRAIN_INFO[5].contamination_per_tick, 0);  // BiolumeGrove
    ASSERT_EQ(TERRAIN_INFO[6].contamination_per_tick, 0);  // PrismaFields
    ASSERT_EQ(TERRAIN_INFO[7].contamination_per_tick, 0);  // SporeFlats
    ASSERT_EQ(TERRAIN_INFO[9].contamination_per_tick, 0);  // EmberCrust
}

// =============================================================================
// Gameplay Property Tests - Clear Costs
// =============================================================================

TEST(clear_cost_biolume_grove) {
    // BiolumeGrove: cost 100 to clear
    ASSERT_EQ(TERRAIN_INFO[5].clear_cost, 100);
}

TEST(clear_cost_prisma_fields_negative) {
    // PrismaFields: negative cost = revenue from clearing crystals
    ASSERT_EQ(TERRAIN_INFO[6].clear_cost, -500);
    ASSERT_EQ(TERRAIN_INFO[6].clear_revenue, 500);
}

TEST(clear_cost_spore_flats) {
    // SporeFlats: cost 50 to clear
    ASSERT_EQ(TERRAIN_INFO[7].clear_cost, 50);
}

TEST(clear_cost_zero_for_non_clearable) {
    // Non-clearable terrain should have zero clear cost
    ASSERT_EQ(TERRAIN_INFO[0].clear_cost, 0);  // Substrate
    ASSERT_EQ(TERRAIN_INFO[1].clear_cost, 0);  // Ridge
    ASSERT_EQ(TERRAIN_INFO[2].clear_cost, 0);  // DeepVoid
    ASSERT_EQ(TERRAIN_INFO[8].clear_cost, 0);  // BlightMires
    ASSERT_EQ(TERRAIN_INFO[9].clear_cost, 0);  // EmberCrust
}

// =============================================================================
// Gameplay Property Tests - Build Cost Modifier
// =============================================================================

TEST(build_cost_modifier_ember_crust) {
    // EmberCrust: 1.5x build cost for nearby buildings
    ASSERT_FLOAT_EQ(TERRAIN_INFO[9].build_cost_modifier, 1.5f);
}

TEST(build_cost_modifier_default_1x) {
    // Most terrain has 1.0 (no modifier)
    ASSERT_FLOAT_EQ(TERRAIN_INFO[0].build_cost_modifier, 1.0f);  // Substrate
    ASSERT_FLOAT_EQ(TERRAIN_INFO[1].build_cost_modifier, 1.0f);  // Ridge
    ASSERT_FLOAT_EQ(TERRAIN_INFO[5].build_cost_modifier, 1.0f);  // BiolumeGrove
    ASSERT_FLOAT_EQ(TERRAIN_INFO[6].build_cost_modifier, 1.0f);  // PrismaFields
}

// =============================================================================
// Gameplay Property Tests - Value and Harmony Bonuses
// =============================================================================

TEST(value_bonus_prisma_fields_highest) {
    // PrismaFields should have highest positive value bonus
    int prisma_bonus = TERRAIN_INFO[6].value_bonus;
    for (std::uint8_t i = 0; i < TERRAIN_TYPE_COUNT; ++i) {
        if (i != 6) {
            ASSERT(prisma_bonus >= TERRAIN_INFO[i].value_bonus);
        }
    }
    ASSERT_EQ(prisma_bonus, 20);
}

TEST(value_bonus_blight_mires_negative) {
    // BlightMires should have negative value (toxic)
    ASSERT(TERRAIN_INFO[8].value_bonus < 0);
    ASSERT_EQ(TERRAIN_INFO[8].value_bonus, -15);
}

TEST(harmony_bonus_blight_mires_negative) {
    // BlightMires should have negative harmony (unhealthy)
    ASSERT(TERRAIN_INFO[8].harmony_bonus < 0);
    ASSERT_EQ(TERRAIN_INFO[8].harmony_bonus, -10);
}

TEST(harmony_bonus_spore_flats_positive) {
    // SporeFlats should have good harmony bonus (pleasing visuals)
    ASSERT(TERRAIN_INFO[7].harmony_bonus > 0);
    ASSERT_EQ(TERRAIN_INFO[7].harmony_bonus, 6);
}

TEST(value_bonus_substrate_neutral) {
    // Substrate is baseline - no bonus
    ASSERT_EQ(TERRAIN_INFO[0].value_bonus, 0);
    ASSERT_EQ(TERRAIN_INFO[0].harmony_bonus, 0);
}

// =============================================================================
// Accessor Function Tests
// =============================================================================

TEST(accessor_getTerrainInfo_by_enum) {
    const TerrainTypeInfo& info = getTerrainInfo(TerrainType::BiolumeGrove);
    ASSERT(info.clearable == true);
    ASSERT_EQ(info.clear_cost, 100);
    ASSERT_FLOAT_EQ(info.emissive_intensity, 0.25f);
}

TEST(accessor_getTerrainInfo_by_index) {
    const TerrainTypeInfo& info = getTerrainInfo(static_cast<std::uint8_t>(5));
    ASSERT(info.clearable == true);
    ASSERT_EQ(info.clear_cost, 100);
}

TEST(accessor_isBuildable) {
    ASSERT(isBuildable(TerrainType::Substrate) == true);
    ASSERT(isBuildable(TerrainType::Ridge) == false);
    ASSERT(isBuildable(TerrainType::DeepVoid) == false);
}

TEST(accessor_isClearable) {
    ASSERT(isClearable(TerrainType::Substrate) == false);
    ASSERT(isClearable(TerrainType::BiolumeGrove) == true);
    ASSERT(isClearable(TerrainType::BlightMires) == false);
}

TEST(accessor_generatesContamination) {
    ASSERT(generatesContamination(TerrainType::Substrate) == false);
    ASSERT(generatesContamination(TerrainType::BlightMires) == true);
    ASSERT(generatesContamination(TerrainType::EmberCrust) == false);
}

TEST(accessor_getEmissiveColor) {
    Vec3 color = getEmissiveColor(TerrainType::PrismaFields);
    ASSERT_FLOAT_EQ(color.x, 1.0f);  // Magenta: full red
    ASSERT_FLOAT_EQ(color.y, 0.0f);  // No green
    ASSERT_FLOAT_EQ(color.z, 1.0f);  // Full blue
}

TEST(accessor_getEmissiveIntensity) {
    ASSERT_FLOAT_EQ(getEmissiveIntensity(TerrainType::Substrate), 0.05f);
    ASSERT_FLOAT_EQ(getEmissiveIntensity(TerrainType::PrismaFields), 0.60f);
    ASSERT_FLOAT_EQ(getEmissiveIntensity(TerrainType::EmberCrust), 0.35f);
}

// =============================================================================
// Consistency Tests
// =============================================================================

TEST(consistency_all_entries_initialized) {
    // Verify no garbage values - all entries should have reasonable data
    for (std::uint8_t i = 0; i < TERRAIN_TYPE_COUNT; ++i) {
        const TerrainTypeInfo& info = TERRAIN_INFO[i];

        // Build cost modifier should be positive
        ASSERT(info.build_cost_modifier > 0.0f);
        ASSERT(info.build_cost_modifier <= 10.0f);  // Reasonable max

        // Emissive intensity should be in valid range
        ASSERT(info.emissive_intensity >= 0.0f);
        ASSERT(info.emissive_intensity <= 1.0f);
    }
}

TEST(consistency_clearable_has_cost_or_revenue) {
    // If clearable, should have either cost or revenue (or both 0 if free)
    for (std::uint8_t i = 0; i < TERRAIN_TYPE_COUNT; ++i) {
        const TerrainTypeInfo& info = TERRAIN_INFO[i];
        if (info.clearable) {
            // At minimum, clearing should have an economic effect
            // (cost to pay OR revenue to gain)
            bool has_economic_effect = (info.clear_cost != 0 || info.clear_revenue != 0);
            ASSERT(has_economic_effect);
        }
    }
}

TEST(consistency_non_buildable_non_clearable_no_clear_cost) {
    // If not clearable, clear cost should be 0
    for (std::uint8_t i = 0; i < TERRAIN_TYPE_COUNT; ++i) {
        const TerrainTypeInfo& info = TERRAIN_INFO[i];
        if (!info.clearable) {
            ASSERT_EQ(info.clear_cost, 0);
            ASSERT_EQ(info.clear_revenue, 0);
        }
    }
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== TerrainTypeInfo Unit Tests ===\n\n");

    // Vec3 helper tests
    RUN_TEST(vec3_default_construction);
    RUN_TEST(vec3_value_construction);
    RUN_TEST(rgb_helper_normalization);
    RUN_TEST(rgb_helper_black);
    RUN_TEST(rgb_helper_white);

    // Array completeness tests
    RUN_TEST(terrain_info_array_size);
    RUN_TEST(terrain_info_indexed_by_enum);

    // Emissive intensity hierarchy tests
    RUN_TEST(emissive_intensity_substrate_minimum);
    RUN_TEST(emissive_intensity_prisma_maximum);
    RUN_TEST(emissive_intensity_hierarchy_order);
    RUN_TEST(emissive_intensity_all_in_valid_range);

    // Emissive color tests
    RUN_TEST(emissive_color_substrate);
    RUN_TEST(emissive_color_prisma_fields);
    RUN_TEST(emissive_color_ember_crust);
    RUN_TEST(emissive_colors_all_normalized);

    // Buildable property tests
    RUN_TEST(buildable_substrate_true);
    RUN_TEST(buildable_ridge_false);
    RUN_TEST(buildable_water_types_false);
    RUN_TEST(buildable_biomes_false);

    // Clearable property tests
    RUN_TEST(clearable_substrate_false);
    RUN_TEST(clearable_ridge_false);
    RUN_TEST(clearable_water_types_false);
    RUN_TEST(clearable_vegetation_biomes_true);
    RUN_TEST(clearable_hazard_biomes_false);
    RUN_TEST(clearable_ember_crust_false);

    // Contamination tests
    RUN_TEST(contamination_only_blight_mires);
    RUN_TEST(contamination_per_tick_blight_mires);
    RUN_TEST(contamination_per_tick_zero_for_non_contaminating);

    // Clear cost tests
    RUN_TEST(clear_cost_biolume_grove);
    RUN_TEST(clear_cost_prisma_fields_negative);
    RUN_TEST(clear_cost_spore_flats);
    RUN_TEST(clear_cost_zero_for_non_clearable);

    // Build cost modifier tests
    RUN_TEST(build_cost_modifier_ember_crust);
    RUN_TEST(build_cost_modifier_default_1x);

    // Value and harmony bonus tests
    RUN_TEST(value_bonus_prisma_fields_highest);
    RUN_TEST(value_bonus_blight_mires_negative);
    RUN_TEST(harmony_bonus_blight_mires_negative);
    RUN_TEST(harmony_bonus_spore_flats_positive);
    RUN_TEST(value_bonus_substrate_neutral);

    // Accessor function tests
    RUN_TEST(accessor_getTerrainInfo_by_enum);
    RUN_TEST(accessor_getTerrainInfo_by_index);
    RUN_TEST(accessor_isBuildable);
    RUN_TEST(accessor_isClearable);
    RUN_TEST(accessor_generatesContamination);
    RUN_TEST(accessor_getEmissiveColor);
    RUN_TEST(accessor_getEmissiveIntensity);

    // Consistency tests
    RUN_TEST(consistency_all_entries_initialized);
    RUN_TEST(consistency_clearable_has_cost_or_revenue);
    RUN_TEST(consistency_non_buildable_non_clearable_no_clear_cost);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
