/**
 * @file test_terrain_contamination.cpp
 * @brief Unit tests for terrain contamination / blight mires (Ticket E10-086).
 *
 * Tests cover:
 * - apply_terrain_contamination with one toxic tile: level = 30
 * - Multiple toxic tiles: each gets 30
 * - Empty list: no changes
 * - ContaminationType::Terrain used as dominant type
 * - Multiple applications accumulate
 */

#include <sims3000/contamination/TerrainContamination.h>
#include <sims3000/contamination/ContaminationGrid.h>
#include <sims3000/contamination/ContaminationType.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace sims3000::contamination;

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

// =============================================================================
// Single toxic tile
// =============================================================================

TEST(single_toxic_tile_level_30) {
    ContaminationGrid grid(64, 64);
    std::vector<TerrainContaminationSource> tiles = { { 10, 20 } };

    apply_terrain_contamination(grid, tiles);

    ASSERT_EQ(grid.get_level(10, 20), 30);
}

TEST(single_toxic_tile_terrain_type) {
    ContaminationGrid grid(64, 64);
    std::vector<TerrainContaminationSource> tiles = { { 10, 20 } };

    apply_terrain_contamination(grid, tiles);

    ASSERT_EQ(grid.get_dominant_type(10, 20),
              static_cast<uint8_t>(ContaminationType::Terrain));
}

// =============================================================================
// Multiple toxic tiles
// =============================================================================

TEST(multiple_toxic_tiles_each_gets_30) {
    ContaminationGrid grid(64, 64);
    std::vector<TerrainContaminationSource> tiles = {
        { 5, 5 },
        { 10, 10 },
        { 30, 40 }
    };

    apply_terrain_contamination(grid, tiles);

    ASSERT_EQ(grid.get_level(5, 5), 30);
    ASSERT_EQ(grid.get_level(10, 10), 30);
    ASSERT_EQ(grid.get_level(30, 40), 30);
}

TEST(multiple_toxic_tiles_all_terrain_type) {
    ContaminationGrid grid(64, 64);
    std::vector<TerrainContaminationSource> tiles = {
        { 5, 5 },
        { 15, 25 }
    };

    apply_terrain_contamination(grid, tiles);

    ASSERT_EQ(grid.get_dominant_type(5, 5),
              static_cast<uint8_t>(ContaminationType::Terrain));
    ASSERT_EQ(grid.get_dominant_type(15, 25),
              static_cast<uint8_t>(ContaminationType::Terrain));
}

TEST(non_toxic_tiles_unaffected) {
    ContaminationGrid grid(64, 64);
    std::vector<TerrainContaminationSource> tiles = { { 10, 10 } };

    apply_terrain_contamination(grid, tiles);

    ASSERT_EQ(grid.get_level(0, 0), 0);
    ASSERT_EQ(grid.get_level(9, 10), 0);
    ASSERT_EQ(grid.get_level(11, 10), 0);
    ASSERT_EQ(grid.get_level(10, 9), 0);
    ASSERT_EQ(grid.get_level(10, 11), 0);
}

// =============================================================================
// Empty list
// =============================================================================

TEST(empty_list_no_changes) {
    ContaminationGrid grid(64, 64);
    std::vector<TerrainContaminationSource> tiles;

    apply_terrain_contamination(grid, tiles);

    ASSERT_EQ(grid.get_level(0, 0), 0);
    ASSERT_EQ(grid.get_level(32, 32), 0);
    ASSERT_EQ(grid.get_level(63, 63), 0);
}

// =============================================================================
// Accumulation
// =============================================================================

TEST(multiple_applications_accumulate) {
    ContaminationGrid grid(64, 64);
    std::vector<TerrainContaminationSource> tiles = { { 10, 10 } };

    apply_terrain_contamination(grid, tiles);
    ASSERT_EQ(grid.get_level(10, 10), 30);

    apply_terrain_contamination(grid, tiles);
    ASSERT_EQ(grid.get_level(10, 10), 60);

    apply_terrain_contamination(grid, tiles);
    ASSERT_EQ(grid.get_level(10, 10), 90);
}

TEST(accumulation_saturates_at_255) {
    ContaminationGrid grid(64, 64);
    std::vector<TerrainContaminationSource> tiles = { { 10, 10 } };

    // 9 * 30 = 270, should saturate at 255
    for (int i = 0; i < 9; ++i) {
        apply_terrain_contamination(grid, tiles);
    }

    ASSERT_EQ(grid.get_level(10, 10), 255);
}

// =============================================================================
// Constant value check
// =============================================================================

TEST(blight_mire_constant_is_30) {
    ASSERT_EQ(BLIGHT_MIRE_CONTAMINATION, static_cast<uint8_t>(30));
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Terrain Contamination Tests (E10-086) ===\n\n");

    RUN_TEST(single_toxic_tile_level_30);
    RUN_TEST(single_toxic_tile_terrain_type);
    RUN_TEST(multiple_toxic_tiles_each_gets_30);
    RUN_TEST(multiple_toxic_tiles_all_terrain_type);
    RUN_TEST(non_toxic_tiles_unaffected);
    RUN_TEST(empty_list_no_changes);
    RUN_TEST(multiple_applications_accumulate);
    RUN_TEST(accumulation_saturates_at_255);
    RUN_TEST(blight_mire_constant_is_30);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
