/**
 * @file test_fluid_placement_validation.cpp
 * @brief Unit tests for fluid placement validation (Tickets 6-027, 6-028)
 *
 * Tests cover:
 * - Extractor: valid placement near water
 * - Extractor: rejected when too far from water (>8 tiles)
 * - Extractor: out of bounds rejected
 * - Extractor: efficiency at various distances
 * - Extractor: will_be_operational at edge distances
 * - Extractor: non-buildable terrain rejected
 * - Extractor: nullptr terrain passes (no water check without terrain)
 * - Reservoir: valid placement (no water requirement)
 * - Reservoir: out of bounds rejected
 * - Reservoir: non-buildable terrain rejected
 * - Reservoir: nullptr terrain passes
 * - Water factor curve: test each tier (0, 1-2, 3-4, 5-6, 7-8, 9+)
 */

#include <sims3000/fluid/FluidPlacementValidation.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace sims3000::fluid;

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

#define ASSERT_FLOAT_EQ(a, b) do { \
    if (std::fabs((a) - (b)) > 0.001f) { \
        printf("\n  FAILED: %s ~= %s (got %f vs %f, line %d)\n", #a, #b, \
               static_cast<double>(a), static_cast<double>(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Stub terrain for testing
// =============================================================================

/**
 * @brief Stub terrain that returns configurable buildability and water distance.
 *
 * All other ITerrainQueryable methods return safe defaults.
 */
class StubTerrain : public sims3000::terrain::ITerrainQueryable {
public:
    bool buildable_value = true;
    uint32_t water_distance_value = 0;

    sims3000::terrain::TerrainType get_terrain_type(
        std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return sims3000::terrain::TerrainType::Substrate;
    }

    std::uint8_t get_elevation(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return 10;
    }

    bool is_buildable(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return buildable_value;
    }

    std::uint8_t get_slope(std::int32_t /*x1*/, std::int32_t /*y1*/,
                           std::int32_t /*x2*/, std::int32_t /*y2*/) const override {
        return 0;
    }

    float get_average_elevation(std::int32_t /*x*/, std::int32_t /*y*/,
                                std::uint32_t /*radius*/) const override {
        return 10.0f;
    }

    std::uint32_t get_water_distance(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return water_distance_value;
    }

    float get_value_bonus(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return 0.0f;
    }

    float get_harmony_bonus(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return 0.0f;
    }

    std::int32_t get_build_cost_modifier(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return 100;
    }

    std::uint32_t get_contamination_output(std::int32_t /*x*/, std::int32_t /*y*/) const override {
        return 0;
    }

    std::uint32_t get_map_width() const override { return 128; }
    std::uint32_t get_map_height() const override { return 128; }
    std::uint8_t get_sea_level() const override { return 8; }

    void get_tiles_in_rect(const sims3000::terrain::GridRect& /*rect*/,
                           std::vector<sims3000::terrain::TerrainComponent>& out_tiles) const override {
        out_tiles.clear();
    }

    std::uint32_t get_buildable_tiles_in_rect(
        const sims3000::terrain::GridRect& /*rect*/) const override {
        return 0;
    }

    std::uint32_t count_terrain_type_in_rect(
        const sims3000::terrain::GridRect& /*rect*/,
        sims3000::terrain::TerrainType /*type*/) const override {
        return 0;
    }
};

// =============================================================================
// Water Factor Curve Tests
// =============================================================================

TEST(water_factor_distance_0) {
    ASSERT_FLOAT_EQ(calculate_water_factor(0), 1.0f);
}

TEST(water_factor_distance_1) {
    ASSERT_FLOAT_EQ(calculate_water_factor(1), 0.9f);
}

TEST(water_factor_distance_2) {
    ASSERT_FLOAT_EQ(calculate_water_factor(2), 0.9f);
}

TEST(water_factor_distance_3) {
    ASSERT_FLOAT_EQ(calculate_water_factor(3), 0.7f);
}

TEST(water_factor_distance_4) {
    ASSERT_FLOAT_EQ(calculate_water_factor(4), 0.7f);
}

TEST(water_factor_distance_5) {
    ASSERT_FLOAT_EQ(calculate_water_factor(5), 0.5f);
}

TEST(water_factor_distance_6) {
    ASSERT_FLOAT_EQ(calculate_water_factor(6), 0.5f);
}

TEST(water_factor_distance_7) {
    ASSERT_FLOAT_EQ(calculate_water_factor(7), 0.3f);
}

TEST(water_factor_distance_8) {
    ASSERT_FLOAT_EQ(calculate_water_factor(8), 0.3f);
}

TEST(water_factor_distance_9) {
    ASSERT_FLOAT_EQ(calculate_water_factor(9), 0.0f);
}

TEST(water_factor_distance_10) {
    ASSERT_FLOAT_EQ(calculate_water_factor(10), 0.0f);
}

TEST(water_factor_distance_255) {
    ASSERT_FLOAT_EQ(calculate_water_factor(255), 0.0f);
}

// =============================================================================
// Extractor Placement: Valid placement near water
// =============================================================================

TEST(extractor_valid_near_water_distance_0) {
    StubTerrain terrain;
    terrain.buildable_value = true;
    terrain.water_distance_value = 0;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.can_place);
    ASSERT_EQ(result.water_distance, 0);
    ASSERT_FLOAT_EQ(result.expected_efficiency, 1.0f);
    ASSERT(result.will_be_operational);
}

TEST(extractor_valid_near_water_distance_3) {
    StubTerrain terrain;
    terrain.buildable_value = true;
    terrain.water_distance_value = 3;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.can_place);
    ASSERT_EQ(result.water_distance, 3);
    ASSERT_FLOAT_EQ(result.expected_efficiency, 0.7f);
    ASSERT(result.will_be_operational);
}

TEST(extractor_valid_near_water_distance_8) {
    StubTerrain terrain;
    terrain.buildable_value = true;
    terrain.water_distance_value = 8;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.can_place);
    ASSERT_EQ(result.water_distance, 8);
    ASSERT_FLOAT_EQ(result.expected_efficiency, 0.3f);
    // distance 8 > MAX_OPERATIONAL_DISTANCE (5)
    ASSERT(!result.will_be_operational);
}

// =============================================================================
// Extractor Placement: Rejected when too far from water (>8 tiles)
// =============================================================================

TEST(extractor_rejected_too_far_from_water_distance_9) {
    StubTerrain terrain;
    terrain.buildable_value = true;
    terrain.water_distance_value = 9;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(!result.can_place);
    ASSERT(result.reason[0] != '\0'); // reason should not be empty
    ASSERT_EQ(result.water_distance, 9);
    ASSERT_FLOAT_EQ(result.expected_efficiency, 0.0f);
    ASSERT(!result.will_be_operational);
}

TEST(extractor_rejected_too_far_from_water_distance_50) {
    StubTerrain terrain;
    terrain.buildable_value = true;
    terrain.water_distance_value = 50;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(!result.can_place);
    ASSERT_EQ(result.water_distance, 50);
}

TEST(extractor_rejected_too_far_from_water_distance_255) {
    StubTerrain terrain;
    terrain.buildable_value = true;
    terrain.water_distance_value = 255;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(!result.can_place);
    ASSERT_EQ(result.water_distance, 255);
}

// =============================================================================
// Extractor Placement: Out of bounds rejected
// =============================================================================

TEST(extractor_out_of_bounds_x) {
    ExtractorPlacementResult result = validate_extractor_placement(
        128, 64, 0, 128, 128, nullptr, nullptr);

    ASSERT(!result.can_place);
    ASSERT(result.reason[0] != '\0');
}

TEST(extractor_out_of_bounds_y) {
    ExtractorPlacementResult result = validate_extractor_placement(
        64, 128, 0, 128, 128, nullptr, nullptr);

    ASSERT(!result.can_place);
}

TEST(extractor_out_of_bounds_both) {
    ExtractorPlacementResult result = validate_extractor_placement(
        200, 200, 0, 128, 128, nullptr, nullptr);

    ASSERT(!result.can_place);
}

TEST(extractor_out_of_bounds_large) {
    ExtractorPlacementResult result = validate_extractor_placement(
        999999, 999999, 0, 128, 128, nullptr, nullptr);

    ASSERT(!result.can_place);
}

// =============================================================================
// Extractor Placement: Efficiency at various distances
// =============================================================================

TEST(extractor_efficiency_distance_0) {
    StubTerrain terrain;
    terrain.water_distance_value = 0;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.can_place);
    ASSERT_FLOAT_EQ(result.expected_efficiency, 1.0f);
}

TEST(extractor_efficiency_distance_1) {
    StubTerrain terrain;
    terrain.water_distance_value = 1;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.can_place);
    ASSERT_FLOAT_EQ(result.expected_efficiency, 0.9f);
}

TEST(extractor_efficiency_distance_4) {
    StubTerrain terrain;
    terrain.water_distance_value = 4;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.can_place);
    ASSERT_FLOAT_EQ(result.expected_efficiency, 0.7f);
}

TEST(extractor_efficiency_distance_6) {
    StubTerrain terrain;
    terrain.water_distance_value = 6;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.can_place);
    ASSERT_FLOAT_EQ(result.expected_efficiency, 0.5f);
}

TEST(extractor_efficiency_distance_7) {
    StubTerrain terrain;
    terrain.water_distance_value = 7;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.can_place);
    ASSERT_FLOAT_EQ(result.expected_efficiency, 0.3f);
}

// =============================================================================
// Extractor Placement: will_be_operational at edge distances
// =============================================================================

TEST(extractor_operational_at_distance_0) {
    StubTerrain terrain;
    terrain.water_distance_value = 0;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.will_be_operational);
}

TEST(extractor_operational_at_distance_5) {
    StubTerrain terrain;
    terrain.water_distance_value = 5;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    // MAX_OPERATIONAL_DISTANCE = 5, so distance 5 is operational
    ASSERT(result.will_be_operational);
}

TEST(extractor_not_operational_at_distance_6) {
    StubTerrain terrain;
    terrain.water_distance_value = 6;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    // distance 6 > MAX_OPERATIONAL_DISTANCE (5), not operational
    ASSERT(!result.will_be_operational);
}

TEST(extractor_not_operational_at_distance_8) {
    StubTerrain terrain;
    terrain.water_distance_value = 8;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    // distance 8 > MAX_OPERATIONAL_DISTANCE (5), not operational
    ASSERT(!result.will_be_operational);
}

// =============================================================================
// Extractor Placement: Terrain checks
// =============================================================================

TEST(extractor_non_buildable_terrain_fails) {
    StubTerrain terrain;
    terrain.buildable_value = false;
    terrain.water_distance_value = 2;

    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(!result.can_place);
    ASSERT(result.reason[0] != '\0');
}

TEST(extractor_nullptr_terrain_passes) {
    // Without terrain, no water distance check and no buildability check
    ExtractorPlacementResult result = validate_extractor_placement(
        64, 64, 0, 128, 128, nullptr, nullptr);

    // With nullptr terrain, water_distance defaults to 255 which is > 8
    // so this will fail on water proximity check
    ASSERT(!result.can_place);
}

// =============================================================================
// Reservoir Placement: Valid placement (no water requirement)
// =============================================================================

TEST(reservoir_valid_placement) {
    StubTerrain terrain;
    terrain.buildable_value = true;
    terrain.water_distance_value = 255; // very far from water

    ReservoirPlacementResult result = validate_reservoir_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.can_place);
}

TEST(reservoir_valid_at_origin) {
    StubTerrain terrain;
    terrain.buildable_value = true;

    ReservoirPlacementResult result = validate_reservoir_placement(
        0, 0, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.can_place);
}

TEST(reservoir_valid_at_max_bound) {
    StubTerrain terrain;
    terrain.buildable_value = true;

    ReservoirPlacementResult result = validate_reservoir_placement(
        127, 127, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.can_place);
}

TEST(reservoir_valid_nullptr_terrain) {
    ReservoirPlacementResult result = validate_reservoir_placement(
        64, 64, 0, 128, 128, nullptr, nullptr);

    ASSERT(result.can_place);
}

TEST(reservoir_no_water_requirement) {
    // Reservoir should be placeable regardless of water distance
    StubTerrain terrain;
    terrain.buildable_value = true;
    terrain.water_distance_value = 100;

    ReservoirPlacementResult result = validate_reservoir_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(result.can_place);
}

// =============================================================================
// Reservoir Placement: Out of bounds rejected
// =============================================================================

TEST(reservoir_out_of_bounds_x) {
    ReservoirPlacementResult result = validate_reservoir_placement(
        128, 64, 0, 128, 128, nullptr, nullptr);

    ASSERT(!result.can_place);
    ASSERT(result.reason[0] != '\0');
}

TEST(reservoir_out_of_bounds_y) {
    ReservoirPlacementResult result = validate_reservoir_placement(
        64, 128, 0, 128, 128, nullptr, nullptr);

    ASSERT(!result.can_place);
}

TEST(reservoir_out_of_bounds_both) {
    ReservoirPlacementResult result = validate_reservoir_placement(
        200, 200, 0, 128, 128, nullptr, nullptr);

    ASSERT(!result.can_place);
}

TEST(reservoir_out_of_bounds_large) {
    ReservoirPlacementResult result = validate_reservoir_placement(
        999999, 999999, 0, 128, 128, nullptr, nullptr);

    ASSERT(!result.can_place);
}

// =============================================================================
// Reservoir Placement: Terrain checks
// =============================================================================

TEST(reservoir_non_buildable_terrain_fails) {
    StubTerrain terrain;
    terrain.buildable_value = false;

    ReservoirPlacementResult result = validate_reservoir_placement(
        64, 64, 0, 128, 128, &terrain, nullptr);

    ASSERT(!result.can_place);
    ASSERT(result.reason[0] != '\0');
}

// =============================================================================
// Extractor Placement: Different players
// =============================================================================

TEST(extractor_valid_different_players) {
    StubTerrain terrain;
    terrain.water_distance_value = 2;

    for (uint8_t player = 0; player < 4; ++player) {
        ExtractorPlacementResult result = validate_extractor_placement(
            64, 64, player, 128, 128, &terrain, nullptr);
        ASSERT(result.can_place);
    }
}

TEST(reservoir_valid_different_players) {
    StubTerrain terrain;
    terrain.buildable_value = true;

    for (uint8_t player = 0; player < 4; ++player) {
        ReservoirPlacementResult result = validate_reservoir_placement(
            64, 64, player, 128, 128, &terrain, nullptr);
        ASSERT(result.can_place);
    }
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Fluid Placement Validation Unit Tests (Tickets 6-027, 6-028) ===\n\n");

    // Water factor curve
    printf("--- Water Factor Curve ---\n");
    RUN_TEST(water_factor_distance_0);
    RUN_TEST(water_factor_distance_1);
    RUN_TEST(water_factor_distance_2);
    RUN_TEST(water_factor_distance_3);
    RUN_TEST(water_factor_distance_4);
    RUN_TEST(water_factor_distance_5);
    RUN_TEST(water_factor_distance_6);
    RUN_TEST(water_factor_distance_7);
    RUN_TEST(water_factor_distance_8);
    RUN_TEST(water_factor_distance_9);
    RUN_TEST(water_factor_distance_10);
    RUN_TEST(water_factor_distance_255);

    // Extractor: valid placement near water
    printf("\n--- Extractor: Valid Placement ---\n");
    RUN_TEST(extractor_valid_near_water_distance_0);
    RUN_TEST(extractor_valid_near_water_distance_3);
    RUN_TEST(extractor_valid_near_water_distance_8);

    // Extractor: rejected when too far
    printf("\n--- Extractor: Too Far From Water ---\n");
    RUN_TEST(extractor_rejected_too_far_from_water_distance_9);
    RUN_TEST(extractor_rejected_too_far_from_water_distance_50);
    RUN_TEST(extractor_rejected_too_far_from_water_distance_255);

    // Extractor: out of bounds
    printf("\n--- Extractor: Out of Bounds ---\n");
    RUN_TEST(extractor_out_of_bounds_x);
    RUN_TEST(extractor_out_of_bounds_y);
    RUN_TEST(extractor_out_of_bounds_both);
    RUN_TEST(extractor_out_of_bounds_large);

    // Extractor: efficiency at various distances
    printf("\n--- Extractor: Efficiency ---\n");
    RUN_TEST(extractor_efficiency_distance_0);
    RUN_TEST(extractor_efficiency_distance_1);
    RUN_TEST(extractor_efficiency_distance_4);
    RUN_TEST(extractor_efficiency_distance_6);
    RUN_TEST(extractor_efficiency_distance_7);

    // Extractor: will_be_operational at edge distances
    printf("\n--- Extractor: Operational Status ---\n");
    RUN_TEST(extractor_operational_at_distance_0);
    RUN_TEST(extractor_operational_at_distance_5);
    RUN_TEST(extractor_not_operational_at_distance_6);
    RUN_TEST(extractor_not_operational_at_distance_8);

    // Extractor: terrain checks
    printf("\n--- Extractor: Terrain Checks ---\n");
    RUN_TEST(extractor_non_buildable_terrain_fails);
    RUN_TEST(extractor_nullptr_terrain_passes);

    // Extractor: different players
    printf("\n--- Extractor: Player Variants ---\n");
    RUN_TEST(extractor_valid_different_players);

    // Reservoir: valid placement
    printf("\n--- Reservoir: Valid Placement ---\n");
    RUN_TEST(reservoir_valid_placement);
    RUN_TEST(reservoir_valid_at_origin);
    RUN_TEST(reservoir_valid_at_max_bound);
    RUN_TEST(reservoir_valid_nullptr_terrain);
    RUN_TEST(reservoir_no_water_requirement);

    // Reservoir: out of bounds
    printf("\n--- Reservoir: Out of Bounds ---\n");
    RUN_TEST(reservoir_out_of_bounds_x);
    RUN_TEST(reservoir_out_of_bounds_y);
    RUN_TEST(reservoir_out_of_bounds_both);
    RUN_TEST(reservoir_out_of_bounds_large);

    // Reservoir: terrain checks
    printf("\n--- Reservoir: Terrain Checks ---\n");
    RUN_TEST(reservoir_non_buildable_terrain_fails);

    // Reservoir: different players
    printf("\n--- Reservoir: Player Variants ---\n");
    RUN_TEST(reservoir_valid_different_players);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
