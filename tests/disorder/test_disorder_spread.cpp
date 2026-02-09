/**
 * @file test_disorder_spread.cpp
 * @brief Unit tests for disorder spread algorithm (Ticket E10-075).
 *
 * Tests cover:
 * - No spread below threshold (64)
 * - Spread to 4-neighbors above threshold
 * - Water blocks spread (pass water mask)
 * - Source loses disorder after spreading
 * - Delta buffer prevents order-dependent results
 * - Edge cells spread to fewer neighbors
 */

#include <sims3000/disorder/DisorderSpread.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace sims3000::disorder;

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
// No spread below threshold
// =============================================================================

TEST(no_spread_at_threshold) {
    DisorderGrid grid(16, 16);
    grid.set_level(8, 8, SPREAD_THRESHOLD); // exactly at threshold

    apply_disorder_spread(grid);

    // Level at or below threshold should not spread
    ASSERT_EQ(grid.get_level(8, 8), SPREAD_THRESHOLD);
    ASSERT_EQ(grid.get_level(9, 8), 0);
    ASSERT_EQ(grid.get_level(7, 8), 0);
    ASSERT_EQ(grid.get_level(8, 9), 0);
    ASSERT_EQ(grid.get_level(8, 7), 0);
}

TEST(no_spread_below_threshold) {
    DisorderGrid grid(16, 16);
    grid.set_level(8, 8, 30); // well below threshold

    apply_disorder_spread(grid);

    ASSERT_EQ(grid.get_level(8, 8), 30);
    ASSERT_EQ(grid.get_level(9, 8), 0);
    ASSERT_EQ(grid.get_level(7, 8), 0);
}

TEST(no_spread_just_above_threshold_spread_zero) {
    // level = 65: spread = (65 - 64) / 8 = 0 (integer division)
    DisorderGrid grid(16, 16);
    grid.set_level(8, 8, 65);

    apply_disorder_spread(grid);

    ASSERT_EQ(grid.get_level(8, 8), 65);
    ASSERT_EQ(grid.get_level(9, 8), 0);
}

// =============================================================================
// Spread to 4-neighbors above threshold
// =============================================================================

TEST(spread_to_4_neighbors) {
    DisorderGrid grid(16, 16);
    // level = 128: spread = (128 - 64) / 8 = 8
    grid.set_level(8, 8, 128);

    apply_disorder_spread(grid);

    // Each neighbor gets +8
    ASSERT_EQ(grid.get_level(9, 8), 8);
    ASSERT_EQ(grid.get_level(7, 8), 8);
    ASSERT_EQ(grid.get_level(8, 9), 8);
    ASSERT_EQ(grid.get_level(8, 7), 8);

    // Source loses 8 * 4 = 32
    // 128 - 32 = 96
    ASSERT_EQ(grid.get_level(8, 8), 96);
}

TEST(spread_amount_proportional_to_excess) {
    DisorderGrid grid(16, 16);
    // level = 200: spread = (200 - 64) / 8 = 17
    grid.set_level(8, 8, 200);

    apply_disorder_spread(grid);

    ASSERT_EQ(grid.get_level(9, 8), 17);
    ASSERT_EQ(grid.get_level(7, 8), 17);
    ASSERT_EQ(grid.get_level(8, 9), 17);
    ASSERT_EQ(grid.get_level(8, 7), 17);

    // Source loses 17 * 4 = 68
    // 200 - 68 = 132
    ASSERT_EQ(grid.get_level(8, 8), 132);
}

TEST(diagonal_neighbors_unaffected) {
    DisorderGrid grid(16, 16);
    grid.set_level(8, 8, 128);

    apply_disorder_spread(grid);

    // Diagonals should remain zero
    ASSERT_EQ(grid.get_level(7, 7), 0);
    ASSERT_EQ(grid.get_level(9, 9), 0);
    ASSERT_EQ(grid.get_level(7, 9), 0);
    ASSERT_EQ(grid.get_level(9, 7), 0);
}

// =============================================================================
// Water blocks spread
// =============================================================================

TEST(water_blocks_spread) {
    DisorderGrid grid(16, 16);
    // level = 128: spread = 8
    grid.set_level(8, 8, 128);

    // Create water mask - mark (9,8) as water
    std::vector<bool> water_mask(16 * 16, false);
    water_mask[8 * 16 + 9] = true; // (9, 8) is water

    apply_disorder_spread(grid, &water_mask);

    // Water tile should not receive spread
    ASSERT_EQ(grid.get_level(9, 8), 0);

    // Non-water neighbors still receive spread
    ASSERT_EQ(grid.get_level(7, 8), 8);
    ASSERT_EQ(grid.get_level(8, 9), 8);
    ASSERT_EQ(grid.get_level(8, 7), 8);

    // Source only loses for 3 valid non-water neighbors
    // 128 - 8 * 3 = 128 - 24 = 104
    ASSERT_EQ(grid.get_level(8, 8), 104);
}

TEST(all_neighbors_water_no_spread) {
    DisorderGrid grid(16, 16);
    grid.set_level(8, 8, 128);

    // All 4 neighbors are water
    std::vector<bool> water_mask(16 * 16, false);
    water_mask[8 * 16 + 9] = true;   // (9, 8)
    water_mask[8 * 16 + 7] = true;   // (7, 8)
    water_mask[9 * 16 + 8] = true;   // (8, 9)
    water_mask[7 * 16 + 8] = true;   // (8, 7)

    apply_disorder_spread(grid, &water_mask);

    // No spread occurs, source unchanged
    ASSERT_EQ(grid.get_level(8, 8), 128);
    ASSERT_EQ(grid.get_level(9, 8), 0);
    ASSERT_EQ(grid.get_level(7, 8), 0);
    ASSERT_EQ(grid.get_level(8, 9), 0);
    ASSERT_EQ(grid.get_level(8, 7), 0);
}

// =============================================================================
// Source loses disorder after spreading
// =============================================================================

TEST(source_loses_disorder) {
    DisorderGrid grid(16, 16);
    // level = 128, spread = 8, 4 neighbors -> loses 32
    grid.set_level(8, 8, 128);

    apply_disorder_spread(grid);

    uint8_t source_after = grid.get_level(8, 8);
    ASSERT_EQ(source_after, 96); // 128 - 32
}

TEST(source_disorder_clamps_to_zero) {
    DisorderGrid grid(16, 16);
    // level = 255: spread = (255 - 64) / 8 = 23
    // Source loses 23 * 4 = 92
    // 255 - 92 = 163 (no clamping needed here, but test the math)
    grid.set_level(8, 8, 255);

    apply_disorder_spread(grid);

    ASSERT_EQ(grid.get_level(8, 8), 163); // 255 - 92
    ASSERT_EQ(grid.get_level(9, 8), 23);
}

// =============================================================================
// Delta buffer prevents order-dependent results
// =============================================================================

TEST(delta_buffer_order_independent) {
    // Two adjacent high-disorder cells should spread independently
    DisorderGrid grid(16, 16);
    // Both at 128: spread = 8
    grid.set_level(8, 8, 128);
    grid.set_level(9, 8, 128);

    apply_disorder_spread(grid);

    // (8,8) receives spread from (9,8): +8
    // (8,8) loses spread to its 4 neighbors: -32
    // Net for (8,8): 128 + 8 - 32 = 104
    ASSERT_EQ(grid.get_level(8, 8), 104);

    // (9,8) receives spread from (8,8): +8
    // (9,8) loses spread to its 4 neighbors: -32
    // Net for (9,8): 128 + 8 - 32 = 104
    ASSERT_EQ(grid.get_level(9, 8), 104);

    // Shared neighbors between them should only receive contributions from one
    // (7,8) only neighbor of (8,8): +8
    ASSERT_EQ(grid.get_level(7, 8), 8);
    // (10,8) only neighbor of (9,8): +8
    ASSERT_EQ(grid.get_level(10, 8), 8);

    // (8,7) and (9,7) are neighbors of (8,8) and (9,8) respectively
    ASSERT_EQ(grid.get_level(8, 7), 8);
    ASSERT_EQ(grid.get_level(9, 7), 8);

    // (8,9) and (9,9) similar
    ASSERT_EQ(grid.get_level(8, 9), 8);
    ASSERT_EQ(grid.get_level(9, 9), 8);
}

// =============================================================================
// Edge cells spread to fewer neighbors
// =============================================================================

TEST(corner_cell_spreads_to_2_neighbors) {
    DisorderGrid grid(16, 16);
    // Corner (0,0): only has 2 valid neighbors: (1,0) and (0,1)
    // level = 128: spread = 8
    grid.set_level(0, 0, 128);

    apply_disorder_spread(grid);

    ASSERT_EQ(grid.get_level(1, 0), 8);
    ASSERT_EQ(grid.get_level(0, 1), 8);

    // Source loses 8 * 2 = 16
    // 128 - 16 = 112
    ASSERT_EQ(grid.get_level(0, 0), 112);
}

TEST(edge_cell_spreads_to_3_neighbors) {
    DisorderGrid grid(16, 16);
    // Edge (0, 8): has 3 valid neighbors: (1,8), (0,7), (0,9)
    // level = 128: spread = 8
    grid.set_level(0, 8, 128);

    apply_disorder_spread(grid);

    ASSERT_EQ(grid.get_level(1, 8), 8);
    ASSERT_EQ(grid.get_level(0, 7), 8);
    ASSERT_EQ(grid.get_level(0, 9), 8);

    // Source loses 8 * 3 = 24
    // 128 - 24 = 104
    ASSERT_EQ(grid.get_level(0, 8), 104);
}

TEST(bottom_right_corner_spreads_to_2_neighbors) {
    DisorderGrid grid(16, 16);
    // Corner (15, 15): only has 2 valid neighbors: (14,15) and (15,14)
    grid.set_level(15, 15, 128);

    apply_disorder_spread(grid);

    ASSERT_EQ(grid.get_level(14, 15), 8);
    ASSERT_EQ(grid.get_level(15, 14), 8);
    ASSERT_EQ(grid.get_level(15, 15), 112); // 128 - 16
}

// =============================================================================
// Constant check
// =============================================================================

TEST(spread_threshold_is_64) {
    ASSERT_EQ(SPREAD_THRESHOLD, static_cast<uint8_t>(64));
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Disorder Spread Tests (E10-075) ===\n\n");

    RUN_TEST(no_spread_at_threshold);
    RUN_TEST(no_spread_below_threshold);
    RUN_TEST(no_spread_just_above_threshold_spread_zero);
    RUN_TEST(spread_to_4_neighbors);
    RUN_TEST(spread_amount_proportional_to_excess);
    RUN_TEST(diagonal_neighbors_unaffected);
    RUN_TEST(water_blocks_spread);
    RUN_TEST(all_neighbors_water_no_spread);
    RUN_TEST(source_loses_disorder);
    RUN_TEST(source_disorder_clamps_to_zero);
    RUN_TEST(delta_buffer_order_independent);
    RUN_TEST(corner_cell_spreads_to_2_neighbors);
    RUN_TEST(edge_cell_spreads_to_3_neighbors);
    RUN_TEST(bottom_right_corner_spreads_to_2_neighbors);
    RUN_TEST(spread_threshold_is_64);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
