/**
 * @file test_landvalue_grid.cpp
 * @brief Unit tests for LandValueGrid (Ticket E10-062)
 *
 * Tests cover:
 * - Construction with dimensions (default value = 128)
 * - get/set value
 * - get/set terrain_bonus
 * - reset_values
 * - is_valid boundary checks
 */

#include <sims3000/landvalue/LandValueGrid.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::landvalue;

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
// Construction Tests
// =============================================================================

TEST(construction_dimensions) {
    LandValueGrid grid(256, 256);
    ASSERT_EQ(grid.get_width(), static_cast<uint16_t>(256));
    ASSERT_EQ(grid.get_height(), static_cast<uint16_t>(256));
}

TEST(construction_non_square) {
    LandValueGrid grid(128, 64);
    ASSERT_EQ(grid.get_width(), static_cast<uint16_t>(128));
    ASSERT_EQ(grid.get_height(), static_cast<uint16_t>(64));
}

TEST(construction_default_value_128) {
    LandValueGrid grid(128, 128);
    ASSERT_EQ(grid.get_value(0, 0), 128);
    ASSERT_EQ(grid.get_value(64, 64), 128);
    ASSERT_EQ(grid.get_value(127, 127), 128);
}

TEST(construction_default_terrain_bonus_zero) {
    LandValueGrid grid(128, 128);
    ASSERT_EQ(grid.get_terrain_bonus(0, 0), 0);
    ASSERT_EQ(grid.get_terrain_bonus(64, 64), 0);
    ASSERT_EQ(grid.get_terrain_bonus(127, 127), 0);
}

TEST(construction_all_cells_neutral) {
    // Verify a larger sample of cells all default to 128
    LandValueGrid grid(64, 64);
    for (int32_t y = 0; y < 64; y += 8) {
        for (int32_t x = 0; x < 64; x += 8) {
            ASSERT_EQ(grid.get_value(x, y), 128);
        }
    }
}

// =============================================================================
// Get/Set Value Tests
// =============================================================================

TEST(set_and_get_value) {
    LandValueGrid grid(128, 128);
    grid.set_value(10, 20, 200);
    ASSERT_EQ(grid.get_value(10, 20), 200);
}

TEST(set_value_overwrites) {
    LandValueGrid grid(128, 128);
    grid.set_value(5, 5, 50);
    ASSERT_EQ(grid.get_value(5, 5), 50);
    grid.set_value(5, 5, 250);
    ASSERT_EQ(grid.get_value(5, 5), 250);
}

TEST(set_value_does_not_affect_neighbors) {
    LandValueGrid grid(128, 128);
    grid.set_value(50, 50, 10);
    ASSERT_EQ(grid.get_value(49, 50), 128); // default
    ASSERT_EQ(grid.get_value(51, 50), 128);
    ASSERT_EQ(grid.get_value(50, 49), 128);
    ASSERT_EQ(grid.get_value(50, 51), 128);
}

TEST(set_value_full_range) {
    LandValueGrid grid(16, 16);
    grid.set_value(0, 0, 0);
    grid.set_value(1, 0, 128);
    grid.set_value(2, 0, 255);
    ASSERT_EQ(grid.get_value(0, 0), 0);
    ASSERT_EQ(grid.get_value(1, 0), 128);
    ASSERT_EQ(grid.get_value(2, 0), 255);
}

TEST(set_value_corner_cells) {
    LandValueGrid grid(256, 256);
    grid.set_value(0, 0, 10);
    grid.set_value(255, 0, 20);
    grid.set_value(0, 255, 30);
    grid.set_value(255, 255, 40);
    ASSERT_EQ(grid.get_value(0, 0), 10);
    ASSERT_EQ(grid.get_value(255, 0), 20);
    ASSERT_EQ(grid.get_value(0, 255), 30);
    ASSERT_EQ(grid.get_value(255, 255), 40);
}

// =============================================================================
// Get/Set Terrain Bonus Tests
// =============================================================================

TEST(set_and_get_terrain_bonus) {
    LandValueGrid grid(128, 128);
    grid.set_terrain_bonus(10, 20, 50);
    ASSERT_EQ(grid.get_terrain_bonus(10, 20), 50);
}

TEST(set_terrain_bonus_overwrites) {
    LandValueGrid grid(128, 128);
    grid.set_terrain_bonus(5, 5, 30);
    ASSERT_EQ(grid.get_terrain_bonus(5, 5), 30);
    grid.set_terrain_bonus(5, 5, 100);
    ASSERT_EQ(grid.get_terrain_bonus(5, 5), 100);
}

TEST(set_terrain_bonus_does_not_affect_value) {
    LandValueGrid grid(128, 128);
    grid.set_value(10, 10, 200);
    grid.set_terrain_bonus(10, 10, 50);
    ASSERT_EQ(grid.get_value(10, 10), 200);
    ASSERT_EQ(grid.get_terrain_bonus(10, 10), 50);
}

TEST(set_value_does_not_affect_terrain_bonus) {
    LandValueGrid grid(128, 128);
    grid.set_terrain_bonus(10, 10, 50);
    grid.set_value(10, 10, 200);
    ASSERT_EQ(grid.get_terrain_bonus(10, 10), 50);
    ASSERT_EQ(grid.get_value(10, 10), 200);
}

TEST(terrain_bonus_full_range) {
    LandValueGrid grid(16, 16);
    grid.set_terrain_bonus(0, 0, 0);
    grid.set_terrain_bonus(1, 0, 128);
    grid.set_terrain_bonus(2, 0, 255);
    ASSERT_EQ(grid.get_terrain_bonus(0, 0), 0);
    ASSERT_EQ(grid.get_terrain_bonus(1, 0), 128);
    ASSERT_EQ(grid.get_terrain_bonus(2, 0), 255);
}

// =============================================================================
// reset_values Tests
// =============================================================================

TEST(reset_values_resets_to_128) {
    LandValueGrid grid(64, 64);
    grid.set_value(0, 0, 10);
    grid.set_value(32, 32, 200);
    grid.set_value(63, 63, 255);

    grid.reset_values();

    ASSERT_EQ(grid.get_value(0, 0), 128);
    ASSERT_EQ(grid.get_value(32, 32), 128);
    ASSERT_EQ(grid.get_value(63, 63), 128);
}

TEST(reset_values_preserves_terrain_bonus) {
    LandValueGrid grid(64, 64);
    grid.set_value(10, 10, 200);
    grid.set_terrain_bonus(10, 10, 50);
    grid.set_terrain_bonus(20, 20, 75);

    grid.reset_values();

    // Values should be reset
    ASSERT_EQ(grid.get_value(10, 10), 128);
    // Terrain bonuses should be preserved
    ASSERT_EQ(grid.get_terrain_bonus(10, 10), 50);
    ASSERT_EQ(grid.get_terrain_bonus(20, 20), 75);
}

TEST(reset_values_full_grid) {
    LandValueGrid grid(32, 32);
    // Set all cells to non-default values
    for (int32_t y = 0; y < 32; ++y) {
        for (int32_t x = 0; x < 32; ++x) {
            grid.set_value(x, y, static_cast<uint8_t>((x + y) % 256));
        }
    }

    grid.reset_values();

    // All should be back to 128
    for (int32_t y = 0; y < 32; ++y) {
        for (int32_t x = 0; x < 32; ++x) {
            ASSERT_EQ(grid.get_value(x, y), 128);
        }
    }
}

// =============================================================================
// is_valid Boundary Tests
// =============================================================================

TEST(is_valid_corners) {
    LandValueGrid grid(256, 256);
    ASSERT(grid.is_valid(0, 0));
    ASSERT(grid.is_valid(255, 0));
    ASSERT(grid.is_valid(0, 255));
    ASSERT(grid.is_valid(255, 255));
}

TEST(is_valid_center) {
    LandValueGrid grid(256, 256);
    ASSERT(grid.is_valid(128, 128));
}

TEST(is_valid_out_of_bounds_positive) {
    LandValueGrid grid(256, 256);
    ASSERT(!grid.is_valid(256, 0));
    ASSERT(!grid.is_valid(0, 256));
    ASSERT(!grid.is_valid(256, 256));
    ASSERT(!grid.is_valid(1000, 1000));
}

TEST(is_valid_negative_coordinates) {
    LandValueGrid grid(256, 256);
    ASSERT(!grid.is_valid(-1, 0));
    ASSERT(!grid.is_valid(0, -1));
    ASSERT(!grid.is_valid(-1, -1));
    ASSERT(!grid.is_valid(-100, -100));
}

TEST(out_of_bounds_get_value_returns_zero) {
    LandValueGrid grid(128, 128);
    ASSERT_EQ(grid.get_value(128, 0), 0);
    ASSERT_EQ(grid.get_value(0, 128), 0);
    ASSERT_EQ(grid.get_value(-1, 0), 0);
    ASSERT_EQ(grid.get_value(0, -1), 0);
}

TEST(out_of_bounds_get_terrain_bonus_returns_zero) {
    LandValueGrid grid(128, 128);
    grid.set_terrain_bonus(0, 0, 50);
    ASSERT_EQ(grid.get_terrain_bonus(128, 0), 0);
    ASSERT_EQ(grid.get_terrain_bonus(-1, 0), 0);
}

TEST(out_of_bounds_set_value_is_noop) {
    LandValueGrid grid(128, 128);
    grid.set_value(128, 0, 42);
    grid.set_value(-1, 0, 42);
    grid.set_value(0, 128, 42);
    // Verify no corruption
    ASSERT_EQ(grid.get_value(0, 0), 128);
    ASSERT_EQ(grid.get_value(127, 127), 128);
}

TEST(out_of_bounds_set_terrain_bonus_is_noop) {
    LandValueGrid grid(128, 128);
    grid.set_terrain_bonus(128, 0, 42);
    grid.set_terrain_bonus(-1, 0, 42);
    ASSERT_EQ(grid.get_terrain_bonus(0, 0), 0);
}

// =============================================================================
// clear() Tests
// =============================================================================

TEST(clear_resets_values_to_128) {
    LandValueGrid grid(64, 64);
    grid.set_value(10, 10, 200);
    grid.set_value(20, 20, 50);
    grid.clear();
    ASSERT_EQ(grid.get_value(10, 10), 128);
    ASSERT_EQ(grid.get_value(20, 20), 128);
}

TEST(clear_resets_terrain_bonus_to_zero) {
    LandValueGrid grid(64, 64);
    grid.set_terrain_bonus(10, 10, 50);
    grid.set_terrain_bonus(20, 20, 75);
    grid.clear();
    ASSERT_EQ(grid.get_terrain_bonus(10, 10), 0);
    ASSERT_EQ(grid.get_terrain_bonus(20, 20), 0);
}

// =============================================================================
// Value Data Access Tests
// =============================================================================

TEST(value_data_access) {
    LandValueGrid grid(4, 4);
    grid.set_value(0, 0, 10);
    grid.set_value(1, 0, 20);
    grid.set_value(0, 1, 30);

    const uint8_t* data = grid.get_value_data();
    ASSERT_EQ(data[0], 10);  // (0,0)
    ASSERT_EQ(data[1], 20);  // (1,0)
    ASSERT_EQ(data[4], 30);  // (0,1) = y*width + x = 1*4 + 0
}

TEST(value_data_default_values) {
    LandValueGrid grid(4, 4);
    const uint8_t* data = grid.get_value_data();
    for (int i = 0; i < 16; ++i) {
        ASSERT_EQ(data[i], 128);
    }
}

// =============================================================================
// Memory Size Verification
// =============================================================================

TEST(landvalue_cell_size) {
    ASSERT_EQ(sizeof(LandValueCell), 2u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== LandValueGrid Unit Tests (Ticket E10-062) ===\n\n");

    // Construction tests
    RUN_TEST(construction_dimensions);
    RUN_TEST(construction_non_square);
    RUN_TEST(construction_default_value_128);
    RUN_TEST(construction_default_terrain_bonus_zero);
    RUN_TEST(construction_all_cells_neutral);

    // Get/set value tests
    RUN_TEST(set_and_get_value);
    RUN_TEST(set_value_overwrites);
    RUN_TEST(set_value_does_not_affect_neighbors);
    RUN_TEST(set_value_full_range);
    RUN_TEST(set_value_corner_cells);

    // Get/set terrain bonus tests
    RUN_TEST(set_and_get_terrain_bonus);
    RUN_TEST(set_terrain_bonus_overwrites);
    RUN_TEST(set_terrain_bonus_does_not_affect_value);
    RUN_TEST(set_value_does_not_affect_terrain_bonus);
    RUN_TEST(terrain_bonus_full_range);

    // reset_values tests
    RUN_TEST(reset_values_resets_to_128);
    RUN_TEST(reset_values_preserves_terrain_bonus);
    RUN_TEST(reset_values_full_grid);

    // is_valid / bounds tests
    RUN_TEST(is_valid_corners);
    RUN_TEST(is_valid_center);
    RUN_TEST(is_valid_out_of_bounds_positive);
    RUN_TEST(is_valid_negative_coordinates);
    RUN_TEST(out_of_bounds_get_value_returns_zero);
    RUN_TEST(out_of_bounds_get_terrain_bonus_returns_zero);
    RUN_TEST(out_of_bounds_set_value_is_noop);
    RUN_TEST(out_of_bounds_set_terrain_bonus_is_noop);

    // clear() tests
    RUN_TEST(clear_resets_values_to_128);
    RUN_TEST(clear_resets_terrain_bonus_to_zero);

    // Value data access tests
    RUN_TEST(value_data_access);
    RUN_TEST(value_data_default_values);

    // Memory size
    RUN_TEST(landvalue_cell_size);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
