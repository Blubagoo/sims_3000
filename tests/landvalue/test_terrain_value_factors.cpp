/**
 * @file test_terrain_value_factors.cpp
 * @brief Unit tests for TerrainValueFactors (Ticket E10-101)
 *
 * Tests cover:
 * - Water adjacent: +30
 * - Crystal fields: +25
 * - Spore plains: +15
 * - Forest: +10
 * - Toxic marshes: -30
 * - Water proximity stacks with terrain type
 * - apply_terrain_bonuses updates grid
 */

#include <sims3000/landvalue/TerrainValueFactors.h>
#include <sims3000/landvalue/LandValueGrid.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace sims3000::landvalue;

// Terrain type enum values (from terrain::TerrainType)
static constexpr uint8_t TERRAIN_SUBSTRATE     = 0;
static constexpr uint8_t TERRAIN_RIDGE         = 1;
static constexpr uint8_t TERRAIN_DEEP_VOID     = 2;
static constexpr uint8_t TERRAIN_FLOW_CHANNEL  = 3;
static constexpr uint8_t TERRAIN_STILL_BASIN   = 4;
static constexpr uint8_t TERRAIN_BIOLUME_GROVE = 5;  // forest
static constexpr uint8_t TERRAIN_PRISMA_FIELDS = 6;  // crystal
static constexpr uint8_t TERRAIN_SPORE_FLATS   = 7;  // spore
static constexpr uint8_t TERRAIN_BLIGHT_MIRES  = 8;  // toxic
static constexpr uint8_t TERRAIN_EMBER_CRUST   = 9;

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
        printf("\n  FAILED: %s == %s (%d != %d) (line %d)\n", \
               #a, #b, static_cast<int>(a), static_cast<int>(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Water Proximity Tests
// =============================================================================

TEST(water_adjacent_bonus) {
    // Water adjacent (dist <= 1) should give +30
    int8_t bonus = calculate_terrain_bonus(TERRAIN_SUBSTRATE, 0);
    ASSERT_EQ(bonus, 30);
    bonus = calculate_terrain_bonus(TERRAIN_SUBSTRATE, 1);
    ASSERT_EQ(bonus, 30);
}

TEST(water_1_tile_bonus) {
    // dist == 2 should give +20
    int8_t bonus = calculate_terrain_bonus(TERRAIN_SUBSTRATE, 2);
    ASSERT_EQ(bonus, 20);
}

TEST(water_2_tiles_bonus) {
    // dist == 3 should give +10
    int8_t bonus = calculate_terrain_bonus(TERRAIN_SUBSTRATE, 3);
    ASSERT_EQ(bonus, 10);
}

TEST(water_far_no_bonus) {
    // dist > 3 should give +0
    int8_t bonus = calculate_terrain_bonus(TERRAIN_SUBSTRATE, 4);
    ASSERT_EQ(bonus, 0);
    bonus = calculate_terrain_bonus(TERRAIN_SUBSTRATE, 255);
    ASSERT_EQ(bonus, 0);
}

// =============================================================================
// Terrain Type Tests
// =============================================================================

TEST(crystal_fields_bonus) {
    // PrismaFields (crystal) should give +25 (no water)
    int8_t bonus = calculate_terrain_bonus(TERRAIN_PRISMA_FIELDS, 255);
    ASSERT_EQ(bonus, 25);
}

TEST(spore_plains_bonus) {
    // SporeFlats (spore) should give +15 (no water)
    int8_t bonus = calculate_terrain_bonus(TERRAIN_SPORE_FLATS, 255);
    ASSERT_EQ(bonus, 15);
}

TEST(forest_bonus) {
    // BiolumeGrove (forest) should give +10 (no water)
    int8_t bonus = calculate_terrain_bonus(TERRAIN_BIOLUME_GROVE, 255);
    ASSERT_EQ(bonus, 10);
}

TEST(toxic_marshes_penalty) {
    // BlightMires (toxic) should give -30 (no water)
    int8_t bonus = calculate_terrain_bonus(TERRAIN_BLIGHT_MIRES, 255);
    ASSERT_EQ(bonus, -30);
}

TEST(substrate_no_bonus) {
    // Substrate (no special terrain, no water) should give +0
    int8_t bonus = calculate_terrain_bonus(TERRAIN_SUBSTRATE, 255);
    ASSERT_EQ(bonus, 0);
}

TEST(ridge_no_bonus) {
    // Ridge (no special terrain, no water) should give +0
    int8_t bonus = calculate_terrain_bonus(TERRAIN_RIDGE, 255);
    ASSERT_EQ(bonus, 0);
}

TEST(ember_crust_no_bonus) {
    // EmberCrust (no special terrain, no water) should give +0
    int8_t bonus = calculate_terrain_bonus(TERRAIN_EMBER_CRUST, 255);
    ASSERT_EQ(bonus, 0);
}

// =============================================================================
// Stacking Tests (Water Proximity + Terrain Type)
// =============================================================================

TEST(crystal_plus_water_adjacent) {
    // PrismaFields + water adjacent: 25 + 30 = 55
    int8_t bonus = calculate_terrain_bonus(TERRAIN_PRISMA_FIELDS, 1);
    ASSERT_EQ(bonus, 55);
}

TEST(spore_plus_water_1_tile) {
    // SporeFlats + water 1 tile: 15 + 20 = 35
    int8_t bonus = calculate_terrain_bonus(TERRAIN_SPORE_FLATS, 2);
    ASSERT_EQ(bonus, 35);
}

TEST(forest_plus_water_2_tiles) {
    // BiolumeGrove + water 2 tiles: 10 + 10 = 20
    int8_t bonus = calculate_terrain_bonus(TERRAIN_BIOLUME_GROVE, 3);
    ASSERT_EQ(bonus, 20);
}

TEST(toxic_plus_water_adjacent) {
    // BlightMires + water adjacent: -30 + 30 = 0
    int8_t bonus = calculate_terrain_bonus(TERRAIN_BLIGHT_MIRES, 1);
    ASSERT_EQ(bonus, 0);
}

TEST(toxic_plus_water_2_tiles) {
    // BlightMires + water 2 tiles: -30 + 10 = -20
    int8_t bonus = calculate_terrain_bonus(TERRAIN_BLIGHT_MIRES, 3);
    ASSERT_EQ(bonus, -20);
}

// =============================================================================
// apply_terrain_bonuses Tests
// =============================================================================

TEST(apply_terrain_bonuses_updates_grid) {
    LandValueGrid grid(16, 16);

    // Grid starts at 128 (neutral). Apply crystal terrain with no water.
    std::vector<TerrainTileInfo> info;
    info.push_back({5, 5, TERRAIN_PRISMA_FIELDS, 255});  // crystal, no water
    info.push_back({6, 6, TERRAIN_BIOLUME_GROVE, 255});   // forest, no water
    info.push_back({7, 7, TERRAIN_SUBSTRATE, 1});          // substrate, water adjacent

    apply_terrain_bonuses(grid, info);

    // Crystal: 128 + 25 = 153
    ASSERT_EQ(grid.get_value(5, 5), static_cast<uint8_t>(153));
    // Forest: 128 + 10 = 138
    ASSERT_EQ(grid.get_value(6, 6), static_cast<uint8_t>(138));
    // Substrate + water adjacent: 128 + 30 = 158
    ASSERT_EQ(grid.get_value(7, 7), static_cast<uint8_t>(158));
}

TEST(apply_terrain_bonuses_stores_terrain_bonus) {
    LandValueGrid grid(16, 16);

    std::vector<TerrainTileInfo> info;
    info.push_back({3, 3, TERRAIN_PRISMA_FIELDS, 255});   // crystal +25
    info.push_back({4, 4, TERRAIN_BLIGHT_MIRES, 255});    // toxic -30

    apply_terrain_bonuses(grid, info);

    // Positive bonus stored
    ASSERT_EQ(grid.get_terrain_bonus(3, 3), static_cast<uint8_t>(25));
    // Negative bonus stored as 0
    ASSERT_EQ(grid.get_terrain_bonus(4, 4), static_cast<uint8_t>(0));
}

TEST(apply_terrain_bonuses_toxic_reduces_value) {
    LandValueGrid grid(16, 16);

    std::vector<TerrainTileInfo> info;
    info.push_back({2, 2, TERRAIN_BLIGHT_MIRES, 255});  // toxic -30, no water

    apply_terrain_bonuses(grid, info);

    // 128 + (-30) = 98
    ASSERT_EQ(grid.get_value(2, 2), static_cast<uint8_t>(98));
}

TEST(apply_terrain_bonuses_clamps_to_zero) {
    LandValueGrid grid(16, 16);
    // Set a low value first
    grid.set_value(1, 1, 10);

    std::vector<TerrainTileInfo> info;
    info.push_back({1, 1, TERRAIN_BLIGHT_MIRES, 255});  // toxic -30

    apply_terrain_bonuses(grid, info);

    // 10 + (-30) = -20, clamped to 0
    ASSERT_EQ(grid.get_value(1, 1), static_cast<uint8_t>(0));
}

TEST(apply_terrain_bonuses_clamps_to_255) {
    LandValueGrid grid(16, 16);
    // Set a high value first
    grid.set_value(1, 1, 240);

    std::vector<TerrainTileInfo> info;
    // Crystal + water adjacent: +25 + 30 = +55
    info.push_back({1, 1, TERRAIN_PRISMA_FIELDS, 1});

    apply_terrain_bonuses(grid, info);

    // 240 + 55 = 295, clamped to 255
    ASSERT_EQ(grid.get_value(1, 1), static_cast<uint8_t>(255));
}

TEST(apply_terrain_bonuses_empty_vector) {
    LandValueGrid grid(16, 16);
    std::vector<TerrainTileInfo> info;

    apply_terrain_bonuses(grid, info);

    // Grid should remain unchanged (all 128)
    ASSERT_EQ(grid.get_value(0, 0), static_cast<uint8_t>(128));
    ASSERT_EQ(grid.get_value(8, 8), static_cast<uint8_t>(128));
}

TEST(apply_terrain_bonuses_out_of_bounds_ignored) {
    LandValueGrid grid(16, 16);

    std::vector<TerrainTileInfo> info;
    info.push_back({-1, 0, TERRAIN_PRISMA_FIELDS, 255});  // out of bounds
    info.push_back({16, 0, TERRAIN_PRISMA_FIELDS, 255});   // out of bounds
    info.push_back({0, 16, TERRAIN_PRISMA_FIELDS, 255});   // out of bounds

    apply_terrain_bonuses(grid, info);

    // Grid should remain unchanged (out-of-bounds writes are no-ops)
    ASSERT_EQ(grid.get_value(0, 0), static_cast<uint8_t>(128));
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== TerrainValueFactors Unit Tests (Ticket E10-101) ===\n\n");

    // Water proximity tests
    RUN_TEST(water_adjacent_bonus);
    RUN_TEST(water_1_tile_bonus);
    RUN_TEST(water_2_tiles_bonus);
    RUN_TEST(water_far_no_bonus);

    // Terrain type tests
    RUN_TEST(crystal_fields_bonus);
    RUN_TEST(spore_plains_bonus);
    RUN_TEST(forest_bonus);
    RUN_TEST(toxic_marshes_penalty);
    RUN_TEST(substrate_no_bonus);
    RUN_TEST(ridge_no_bonus);
    RUN_TEST(ember_crust_no_bonus);

    // Stacking tests
    RUN_TEST(crystal_plus_water_adjacent);
    RUN_TEST(spore_plus_water_1_tile);
    RUN_TEST(forest_plus_water_2_tiles);
    RUN_TEST(toxic_plus_water_adjacent);
    RUN_TEST(toxic_plus_water_2_tiles);

    // apply_terrain_bonuses tests
    RUN_TEST(apply_terrain_bonuses_updates_grid);
    RUN_TEST(apply_terrain_bonuses_stores_terrain_bonus);
    RUN_TEST(apply_terrain_bonuses_toxic_reduces_value);
    RUN_TEST(apply_terrain_bonuses_clamps_to_zero);
    RUN_TEST(apply_terrain_bonuses_clamps_to_255);
    RUN_TEST(apply_terrain_bonuses_empty_vector);
    RUN_TEST(apply_terrain_bonuses_out_of_bounds_ignored);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
