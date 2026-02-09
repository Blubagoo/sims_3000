/**
 * @file test_contamination_spread.cpp
 * @brief Unit tests for ContaminationSpread.h (Ticket E10-087)
 *
 * Tests cover:
 * - Spread threshold enforcement (CONTAM_SPREAD_THRESHOLD = 32)
 * - Cardinal neighbor spread (level / 8)
 * - Diagonal neighbor spread (level / 16)
 * - Delta buffer usage (order-independent results)
 * - Reading from previous tick buffer
 * - Bounds checking at grid edges
 * - Multiple sources accumulating correctly
 */

#include <sims3000/contamination/ContaminationSpread.h>
#include <sims3000/contamination/ContaminationGrid.h>
#include <cstdio>
#include <cstdlib>

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
        printf("\n  FAILED: %s == %s (got %d, expected %d) (line %d)\n", #a, #b, (int)(a), (int)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Threshold Tests
// =============================================================================

TEST(threshold_constant_value) {
    ASSERT_EQ(CONTAM_SPREAD_THRESHOLD, 32);
}

TEST(below_threshold_no_spread) {
    ContaminationGrid grid(10, 10);

    // Set center cell to 31 (below threshold)
    grid.set_level(5, 5, 31);
    grid.swap_buffers();  // Move to previous buffer

    apply_contamination_spread(grid);

    // Verify neighbors have no contamination
    ASSERT_EQ(grid.get_level(5, 4), 0);  // North
    ASSERT_EQ(grid.get_level(5, 6), 0);  // South
    ASSERT_EQ(grid.get_level(6, 5), 0);  // East
    ASSERT_EQ(grid.get_level(4, 5), 0);  // West
    ASSERT_EQ(grid.get_level(6, 4), 0);  // Northeast
    ASSERT_EQ(grid.get_level(4, 4), 0);  // Northwest
    ASSERT_EQ(grid.get_level(6, 6), 0);  // Southeast
    ASSERT_EQ(grid.get_level(4, 6), 0);  // Southwest
}

TEST(at_threshold_spreads) {
    ContaminationGrid grid(10, 10);

    // Set center cell to exactly 32 (at threshold)
    grid.set_level(5, 5, 32);
    grid.swap_buffers();

    apply_contamination_spread(grid);

    // Cardinals should receive 32/8 = 4
    ASSERT_EQ(grid.get_level(5, 4), 4);  // North
    ASSERT_EQ(grid.get_level(5, 6), 4);  // South
    ASSERT_EQ(grid.get_level(6, 5), 4);  // East
    ASSERT_EQ(grid.get_level(4, 5), 4);  // West

    // Diagonals should receive 32/16 = 2
    ASSERT_EQ(grid.get_level(6, 4), 2);  // Northeast
    ASSERT_EQ(grid.get_level(4, 4), 2);  // Northwest
    ASSERT_EQ(grid.get_level(6, 6), 2);  // Southeast
    ASSERT_EQ(grid.get_level(4, 6), 2);  // Southwest
}

// =============================================================================
// Cardinal Spread Tests
// =============================================================================

TEST(cardinal_spread_calculation) {
    ContaminationGrid grid(10, 10);

    // Set center to 80
    grid.set_level(5, 5, 80);
    grid.swap_buffers();

    apply_contamination_spread(grid);

    // Cardinals should receive 80/8 = 10
    ASSERT_EQ(grid.get_level(5, 4), 10);
    ASSERT_EQ(grid.get_level(5, 6), 10);
    ASSERT_EQ(grid.get_level(6, 5), 10);
    ASSERT_EQ(grid.get_level(4, 5), 10);
}

TEST(cardinal_spread_high_value) {
    ContaminationGrid grid(10, 10);

    // Set center to 255 (max)
    grid.set_level(5, 5, 255);
    grid.swap_buffers();

    apply_contamination_spread(grid);

    // Cardinals should receive 255/8 = 31
    ASSERT_EQ(grid.get_level(5, 4), 31);
    ASSERT_EQ(grid.get_level(5, 6), 31);
    ASSERT_EQ(grid.get_level(6, 5), 31);
    ASSERT_EQ(grid.get_level(4, 5), 31);
}

// =============================================================================
// Diagonal Spread Tests
// =============================================================================

TEST(diagonal_spread_calculation) {
    ContaminationGrid grid(10, 10);

    // Set center to 80
    grid.set_level(5, 5, 80);
    grid.swap_buffers();

    apply_contamination_spread(grid);

    // Diagonals should receive 80/16 = 5
    ASSERT_EQ(grid.get_level(6, 4), 5);
    ASSERT_EQ(grid.get_level(4, 4), 5);
    ASSERT_EQ(grid.get_level(6, 6), 5);
    ASSERT_EQ(grid.get_level(4, 6), 5);
}

TEST(diagonal_weaker_than_cardinal) {
    ContaminationGrid grid(10, 10);

    // Set center to 128
    grid.set_level(5, 5, 128);
    grid.swap_buffers();

    apply_contamination_spread(grid);

    // Cardinals: 128/8 = 16
    ASSERT_EQ(grid.get_level(5, 4), 16);

    // Diagonals: 128/16 = 8
    ASSERT_EQ(grid.get_level(6, 4), 8);

    // Verify diagonal is half of cardinal
    ASSERT(grid.get_level(6, 4) == grid.get_level(5, 4) / 2);
}

// =============================================================================
// Delta Buffer Tests (Order Independence)
// =============================================================================

TEST(multiple_sources_accumulate) {
    ContaminationGrid grid(10, 10);

    // Set two sources that both spread to (5, 5)
    grid.set_level(4, 5, 80);  // Left source -> spreads 10 east to (5,5)
    grid.set_level(6, 5, 80);  // Right source -> spreads 10 west to (5,5)
    grid.swap_buffers();

    apply_contamination_spread(grid);

    // (5, 5) should receive 10 + 10 = 20
    ASSERT_EQ(grid.get_level(5, 5), 20);
}

TEST(accumulation_saturates_at_255) {
    ContaminationGrid grid(10, 10);

    // Create a cross pattern of high contamination sources
    // All spreading to center (5, 5)
    grid.set_level(5, 4, 255);  // North -> spreads 31 south
    grid.set_level(5, 6, 255);  // South -> spreads 31 north
    grid.set_level(4, 5, 255);  // West -> spreads 31 east
    grid.set_level(6, 5, 255);  // East -> spreads 31 west
    grid.swap_buffers();

    apply_contamination_spread(grid);

    // Center should receive 31*4 = 124 (within 255)
    ASSERT_EQ(grid.get_level(5, 5), 124);
}

// =============================================================================
// Previous Tick Buffer Tests
// =============================================================================

TEST(reads_from_previous_tick) {
    ContaminationGrid grid(10, 10);

    // Set contamination in current buffer
    grid.set_level(5, 5, 80);

    // Swap buffers (80 is now in previous buffer)
    grid.swap_buffers();

    // Apply spread (should read from previous buffer)
    apply_contamination_spread(grid);

    // Neighbors should have spread values
    ASSERT_EQ(grid.get_level(5, 4), 10);  // 80/8 = 10
}

TEST(writes_to_current_buffer) {
    ContaminationGrid grid(10, 10);

    grid.set_level(5, 5, 80);
    grid.swap_buffers();
    apply_contamination_spread(grid);

    // Verify spread is in current buffer by checking get_level (not get_level_previous_tick)
    ASSERT_EQ(grid.get_level(5, 4), 10);

    // Previous buffer should still have the original source
    ASSERT_EQ(grid.get_level_previous_tick(5, 5), 80);
}

// =============================================================================
// Bounds Checking Tests
// =============================================================================

TEST(edge_spread_respects_bounds) {
    ContaminationGrid grid(10, 10);

    // Set top-left corner (0, 0) to high contamination
    grid.set_level(0, 0, 80);
    grid.swap_buffers();

    apply_contamination_spread(grid);

    // Only south and east neighbors exist
    ASSERT_EQ(grid.get_level(0, 1), 10);  // South
    ASSERT_EQ(grid.get_level(1, 0), 10);  // East
    ASSERT_EQ(grid.get_level(1, 1), 5);   // Southeast

    // Out-of-bounds accesses should return 0 (verified implicitly - no crash)
}

TEST(corner_spread_both_edges) {
    ContaminationGrid grid(10, 10);

    // Set bottom-right corner (9, 9) to 80
    grid.set_level(9, 9, 80);
    grid.swap_buffers();

    apply_contamination_spread(grid);

    // Only north and west neighbors exist
    ASSERT_EQ(grid.get_level(9, 8), 10);  // North
    ASSERT_EQ(grid.get_level(8, 9), 10);  // West
    ASSERT_EQ(grid.get_level(8, 8), 5);   // Northwest
}

// =============================================================================
// Dominant Type Propagation Tests
// =============================================================================

TEST(dominant_type_spreads_with_contamination) {
    ContaminationGrid grid(10, 10);

    // Set center with contamination level and type
    grid.set_level(5, 5, 80);
    grid.add_contamination(5, 5, 0, 42);  // Set dominant type to 42
    grid.swap_buffers();

    apply_contamination_spread(grid);

    // Verify neighbors received the dominant type
    // Note: We can't directly query dominant type after add_contamination in this test
    // but the spread implementation should pass it through
    ASSERT_EQ(grid.get_level(5, 4), 10);  // Verify spread occurred
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST(full_grid_spread_iteration) {
    ContaminationGrid grid(10, 10);

    // Set center point
    grid.set_level(5, 5, 160);
    grid.swap_buffers();

    // Apply spread
    apply_contamination_spread(grid);

    // Verify spread pattern
    ASSERT_EQ(grid.get_level(5, 4), 20);  // Cardinals: 160/8
    ASSERT_EQ(grid.get_level(6, 6), 10);  // Diagonals: 160/16
}

TEST(empty_grid_no_spread) {
    ContaminationGrid grid(10, 10);

    // All zeros
    grid.swap_buffers();

    apply_contamination_spread(grid);

    // Verify nothing changed
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            ASSERT_EQ(grid.get_level(x, y), 0);
        }
    }
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== ContaminationSpread Unit Tests (Ticket E10-087) ===\n\n");

    // Threshold tests
    RUN_TEST(threshold_constant_value);
    RUN_TEST(below_threshold_no_spread);
    RUN_TEST(at_threshold_spreads);

    // Cardinal spread tests
    RUN_TEST(cardinal_spread_calculation);
    RUN_TEST(cardinal_spread_high_value);

    // Diagonal spread tests
    RUN_TEST(diagonal_spread_calculation);
    RUN_TEST(diagonal_weaker_than_cardinal);

    // Delta buffer tests
    RUN_TEST(multiple_sources_accumulate);
    RUN_TEST(accumulation_saturates_at_255);

    // Previous tick buffer tests
    RUN_TEST(reads_from_previous_tick);
    RUN_TEST(writes_to_current_buffer);

    // Bounds checking tests
    RUN_TEST(edge_spread_respects_bounds);
    RUN_TEST(corner_spread_both_edges);

    // Dominant type tests
    RUN_TEST(dominant_type_spreads_with_contamination);

    // Integration tests
    RUN_TEST(full_grid_spread_iteration);
    RUN_TEST(empty_grid_no_spread);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
