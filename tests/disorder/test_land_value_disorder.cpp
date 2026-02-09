/**
 * @file test_land_value_disorder.cpp
 * @brief Unit tests for land value effect on disorder (Ticket E10-074).
 *
 * Tests cover:
 * - Zero land value doubles existing disorder
 * - Max land value no additional disorder
 * - Mid land value proportional boost
 * - Zero disorder cells unaffected
 */

#include <sims3000/disorder/LandValueDisorderEffect.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <sims3000/landvalue/LandValueGrid.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::disorder;
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
        printf("\n  FAILED: %s == %s (%d != %d) (line %d)\n", #a, #b, (int)(a), (int)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Zero land value doubles existing disorder
// =============================================================================

TEST(zero_land_value_doubles_disorder) {
    DisorderGrid grid(64, 64);
    LandValueGrid lv_grid(64, 64);

    // Set disorder to 100 at (10, 10)
    grid.set_level(10, 10, 100);
    // Set land value to 0 at (10, 10)
    lv_grid.set_value(10, 10, 0);

    apply_land_value_effect(grid, lv_grid);

    // extra = 100 * (1.0 - 0/255) = 100 * 1.0 = 100
    // new = 100 + 100 = 200
    ASSERT_EQ(grid.get_level(10, 10), 200);
}

TEST(zero_land_value_doubles_small_disorder) {
    DisorderGrid grid(64, 64);
    LandValueGrid lv_grid(64, 64);

    grid.set_level(5, 5, 50);
    lv_grid.set_value(5, 5, 0);

    apply_land_value_effect(grid, lv_grid);

    // extra = 50 * 1.0 = 50; new = 50 + 50 = 100
    ASSERT_EQ(grid.get_level(5, 5), 100);
}

TEST(zero_land_value_saturates_at_255) {
    DisorderGrid grid(64, 64);
    LandValueGrid lv_grid(64, 64);

    grid.set_level(10, 10, 200);
    lv_grid.set_value(10, 10, 0);

    apply_land_value_effect(grid, lv_grid);

    // extra = 200 * 1.0 = 200; new = 200 + 200 = 400 -> saturates to 255
    ASSERT_EQ(grid.get_level(10, 10), 255);
}

// =============================================================================
// Max land value no additional disorder
// =============================================================================

TEST(max_land_value_no_additional_disorder) {
    DisorderGrid grid(64, 64);
    LandValueGrid lv_grid(64, 64);

    grid.set_level(10, 10, 100);
    lv_grid.set_value(10, 10, 255);

    apply_land_value_effect(grid, lv_grid);

    // extra = 100 * (1.0 - 255/255) = 100 * 0.0 = 0
    // new = 100 + 0 = 100
    ASSERT_EQ(grid.get_level(10, 10), 100);
}

TEST(max_land_value_high_disorder_unchanged) {
    DisorderGrid grid(64, 64);
    LandValueGrid lv_grid(64, 64);

    grid.set_level(20, 20, 250);
    lv_grid.set_value(20, 20, 255);

    apply_land_value_effect(grid, lv_grid);

    ASSERT_EQ(grid.get_level(20, 20), 250);
}

// =============================================================================
// Mid land value proportional boost
// =============================================================================

TEST(mid_land_value_proportional_boost) {
    DisorderGrid grid(64, 64);
    LandValueGrid lv_grid(64, 64);

    grid.set_level(10, 10, 100);
    // Land value ~128 -> factor = 1.0 - 128/255 ~ 0.498
    lv_grid.set_value(10, 10, 128);

    apply_land_value_effect(grid, lv_grid);

    // extra = 100 * 0.498 = 49.8 -> 49
    // new = 100 + 49 = 149
    uint8_t result = grid.get_level(10, 10);
    ASSERT(result >= 148 && result <= 150);
}

TEST(quarter_land_value_75_percent_boost) {
    DisorderGrid grid(64, 64);
    LandValueGrid lv_grid(64, 64);

    grid.set_level(10, 10, 100);
    // Land value ~64 -> factor = 1.0 - 64/255 ~ 0.749
    lv_grid.set_value(10, 10, 64);

    apply_land_value_effect(grid, lv_grid);

    // extra = 100 * 0.749 = 74.9 -> 74
    // new = 100 + 74 = 174
    uint8_t result = grid.get_level(10, 10);
    ASSERT(result >= 173 && result <= 175);
}

// =============================================================================
// Zero disorder cells unaffected
// =============================================================================

TEST(zero_disorder_unaffected_by_low_land_value) {
    DisorderGrid grid(64, 64);
    LandValueGrid lv_grid(64, 64);

    // Disorder is 0, land value is 0 (would double if disorder existed)
    grid.set_level(10, 10, 0);
    lv_grid.set_value(10, 10, 0);

    apply_land_value_effect(grid, lv_grid);

    ASSERT_EQ(grid.get_level(10, 10), 0);
}

TEST(zero_disorder_unaffected_by_mid_land_value) {
    DisorderGrid grid(64, 64);
    LandValueGrid lv_grid(64, 64);

    grid.set_level(15, 15, 0);
    lv_grid.set_value(15, 15, 128);

    apply_land_value_effect(grid, lv_grid);

    ASSERT_EQ(grid.get_level(15, 15), 0);
}

TEST(only_nonzero_cells_modified) {
    DisorderGrid grid(64, 64);
    LandValueGrid lv_grid(64, 64);

    // Set one cell with disorder, neighbors with zero
    grid.set_level(10, 10, 80);
    lv_grid.set_value(10, 10, 0);

    // Neighbors have zero disorder
    grid.set_level(9, 10, 0);
    grid.set_level(11, 10, 0);
    lv_grid.set_value(9, 10, 0);
    lv_grid.set_value(11, 10, 0);

    apply_land_value_effect(grid, lv_grid);

    ASSERT_EQ(grid.get_level(10, 10), 160); // 80 doubled
    ASSERT_EQ(grid.get_level(9, 10), 0);    // zero stays zero
    ASSERT_EQ(grid.get_level(11, 10), 0);   // zero stays zero
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Land Value Disorder Effect Tests (E10-074) ===\n\n");

    RUN_TEST(zero_land_value_doubles_disorder);
    RUN_TEST(zero_land_value_doubles_small_disorder);
    RUN_TEST(zero_land_value_saturates_at_255);
    RUN_TEST(max_land_value_no_additional_disorder);
    RUN_TEST(max_land_value_high_disorder_unchanged);
    RUN_TEST(mid_land_value_proportional_boost);
    RUN_TEST(quarter_land_value_75_percent_boost);
    RUN_TEST(zero_disorder_unaffected_by_low_land_value);
    RUN_TEST(zero_disorder_unaffected_by_mid_land_value);
    RUN_TEST(only_nonzero_cells_modified);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
