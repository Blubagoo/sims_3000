/**
 * @file test_disorder_grid.cpp
 * @brief Unit tests for DisorderGrid (Ticket E10-060)
 *
 * Tests cover:
 * - Construction with dimensions
 * - get/set level
 * - add_disorder saturating behavior
 * - apply_suppression saturating behavior
 * - swap_buffers: current becomes previous
 * - get_level_previous_tick returns pre-swap data
 * - Aggregate stats (total_disorder, high_disorder_tiles)
 * - is_valid boundary checks
 * - clear() zeroes both buffers
 */

#include <sims3000/disorder/DisorderGrid.h>
#include <cstdio>
#include <cstdlib>

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
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Construction Tests
// =============================================================================

TEST(construction_dimensions) {
    DisorderGrid grid(256, 256);
    ASSERT_EQ(grid.get_width(), static_cast<uint16_t>(256));
    ASSERT_EQ(grid.get_height(), static_cast<uint16_t>(256));
}

TEST(construction_non_square) {
    DisorderGrid grid(128, 64);
    ASSERT_EQ(grid.get_width(), static_cast<uint16_t>(128));
    ASSERT_EQ(grid.get_height(), static_cast<uint16_t>(64));
}

TEST(construction_all_cells_zero) {
    DisorderGrid grid(128, 128);
    ASSERT_EQ(grid.get_level(0, 0), 0);
    ASSERT_EQ(grid.get_level(64, 64), 0);
    ASSERT_EQ(grid.get_level(127, 127), 0);
}

TEST(construction_previous_buffer_zero) {
    DisorderGrid grid(128, 128);
    ASSERT_EQ(grid.get_level_previous_tick(0, 0), 0);
    ASSERT_EQ(grid.get_level_previous_tick(64, 64), 0);
    ASSERT_EQ(grid.get_level_previous_tick(127, 127), 0);
}

// =============================================================================
// Get/Set Level Tests
// =============================================================================

TEST(set_and_get_single_cell) {
    DisorderGrid grid(128, 128);
    grid.set_level(10, 20, 42);
    ASSERT_EQ(grid.get_level(10, 20), 42);
}

TEST(set_overwrites_previous_value) {
    DisorderGrid grid(128, 128);
    grid.set_level(5, 5, 100);
    ASSERT_EQ(grid.get_level(5, 5), 100);
    grid.set_level(5, 5, 200);
    ASSERT_EQ(grid.get_level(5, 5), 200);
}

TEST(set_does_not_affect_neighbors) {
    DisorderGrid grid(128, 128);
    grid.set_level(50, 50, 255);
    ASSERT_EQ(grid.get_level(49, 50), 0);
    ASSERT_EQ(grid.get_level(51, 50), 0);
    ASSERT_EQ(grid.get_level(50, 49), 0);
    ASSERT_EQ(grid.get_level(50, 51), 0);
}

TEST(set_corner_cells) {
    DisorderGrid grid(256, 256);
    grid.set_level(0, 0, 10);
    grid.set_level(255, 0, 20);
    grid.set_level(0, 255, 30);
    grid.set_level(255, 255, 40);
    ASSERT_EQ(grid.get_level(0, 0), 10);
    ASSERT_EQ(grid.get_level(255, 0), 20);
    ASSERT_EQ(grid.get_level(0, 255), 30);
    ASSERT_EQ(grid.get_level(255, 255), 40);
}

TEST(set_full_range) {
    DisorderGrid grid(16, 16);
    grid.set_level(0, 0, 0);
    grid.set_level(1, 0, 128);
    grid.set_level(2, 0, 255);
    ASSERT_EQ(grid.get_level(0, 0), 0);
    ASSERT_EQ(grid.get_level(1, 0), 128);
    ASSERT_EQ(grid.get_level(2, 0), 255);
}

// =============================================================================
// add_disorder Saturating Tests
// =============================================================================

TEST(add_disorder_basic) {
    DisorderGrid grid(64, 64);
    grid.set_level(5, 5, 100);
    grid.add_disorder(5, 5, 50);
    ASSERT_EQ(grid.get_level(5, 5), 150);
}

TEST(add_disorder_from_zero) {
    DisorderGrid grid(64, 64);
    grid.add_disorder(3, 3, 42);
    ASSERT_EQ(grid.get_level(3, 3), 42);
}

TEST(add_disorder_saturates_at_255) {
    DisorderGrid grid(64, 64);
    grid.set_level(5, 5, 200);
    grid.add_disorder(5, 5, 200);
    ASSERT_EQ(grid.get_level(5, 5), 255);
}

TEST(add_disorder_exact_255) {
    DisorderGrid grid(64, 64);
    grid.set_level(5, 5, 200);
    grid.add_disorder(5, 5, 55);
    ASSERT_EQ(grid.get_level(5, 5), 255);
}

TEST(add_disorder_already_max) {
    DisorderGrid grid(64, 64);
    grid.set_level(5, 5, 255);
    grid.add_disorder(5, 5, 1);
    ASSERT_EQ(grid.get_level(5, 5), 255);
}

TEST(add_disorder_max_plus_max) {
    DisorderGrid grid(64, 64);
    grid.set_level(5, 5, 255);
    grid.add_disorder(5, 5, 255);
    ASSERT_EQ(grid.get_level(5, 5), 255);
}

TEST(add_disorder_zero_amount) {
    DisorderGrid grid(64, 64);
    grid.set_level(5, 5, 100);
    grid.add_disorder(5, 5, 0);
    ASSERT_EQ(grid.get_level(5, 5), 100);
}

// =============================================================================
// apply_suppression Saturating Tests
// =============================================================================

TEST(apply_suppression_basic) {
    DisorderGrid grid(64, 64);
    grid.set_level(5, 5, 100);
    grid.apply_suppression(5, 5, 30);
    ASSERT_EQ(grid.get_level(5, 5), 70);
}

TEST(apply_suppression_saturates_at_zero) {
    DisorderGrid grid(64, 64);
    grid.set_level(5, 5, 50);
    grid.apply_suppression(5, 5, 200);
    ASSERT_EQ(grid.get_level(5, 5), 0);
}

TEST(apply_suppression_exact_zero) {
    DisorderGrid grid(64, 64);
    grid.set_level(5, 5, 100);
    grid.apply_suppression(5, 5, 100);
    ASSERT_EQ(grid.get_level(5, 5), 0);
}

TEST(apply_suppression_from_zero) {
    DisorderGrid grid(64, 64);
    grid.apply_suppression(5, 5, 50);
    ASSERT_EQ(grid.get_level(5, 5), 0);
}

TEST(apply_suppression_zero_amount) {
    DisorderGrid grid(64, 64);
    grid.set_level(5, 5, 100);
    grid.apply_suppression(5, 5, 0);
    ASSERT_EQ(grid.get_level(5, 5), 100);
}

// =============================================================================
// swap_buffers Tests
// =============================================================================

TEST(swap_buffers_current_becomes_previous) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 42);
    ASSERT_EQ(grid.get_level(10, 10), 42);
    ASSERT_EQ(grid.get_level_previous_tick(10, 10), 0);

    grid.swap_buffers();

    // After swap, the old current (42) is now previous
    ASSERT_EQ(grid.get_level_previous_tick(10, 10), 42);
    // The old previous (0) is now current
    ASSERT_EQ(grid.get_level(10, 10), 0);
}

TEST(swap_buffers_double_swap) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 100);

    grid.swap_buffers();
    // current=0, previous=100
    ASSERT_EQ(grid.get_level(10, 10), 0);
    ASSERT_EQ(grid.get_level_previous_tick(10, 10), 100);

    grid.set_level(10, 10, 200);
    grid.swap_buffers();
    // current=100 (old previous), previous=200
    ASSERT_EQ(grid.get_level(10, 10), 100);
    ASSERT_EQ(grid.get_level_previous_tick(10, 10), 200);
}

TEST(swap_buffers_preserves_all_data) {
    DisorderGrid grid(16, 16);
    // Fill current buffer with pattern
    for (int32_t y = 0; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            grid.set_level(x, y, static_cast<uint8_t>((x + y) % 256));
        }
    }

    grid.swap_buffers();

    // Verify previous has the pattern
    for (int32_t y = 0; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            ASSERT_EQ(grid.get_level_previous_tick(x, y),
                      static_cast<uint8_t>((x + y) % 256));
        }
    }

    // Current should be the old previous (all zeros)
    for (int32_t y = 0; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            ASSERT_EQ(grid.get_level(x, y), 0);
        }
    }
}

// =============================================================================
// get_level_previous_tick Tests
// =============================================================================

TEST(previous_tick_returns_pre_swap_data) {
    DisorderGrid grid(64, 64);
    grid.set_level(5, 5, 77);
    grid.set_level(10, 10, 88);

    grid.swap_buffers();

    ASSERT_EQ(grid.get_level_previous_tick(5, 5), 77);
    ASSERT_EQ(grid.get_level_previous_tick(10, 10), 88);
}

TEST(previous_tick_independent_of_current_writes) {
    DisorderGrid grid(64, 64);
    grid.set_level(5, 5, 77);

    grid.swap_buffers();

    // Write to current should not affect previous
    grid.set_level(5, 5, 99);
    ASSERT_EQ(grid.get_level_previous_tick(5, 5), 77);
    ASSERT_EQ(grid.get_level(5, 5), 99);
}

// =============================================================================
// Aggregate Stats Tests
// =============================================================================

TEST(stats_total_disorder) {
    DisorderGrid grid(4, 4);
    grid.set_level(0, 0, 10);
    grid.set_level(1, 0, 20);
    grid.set_level(2, 0, 30);
    grid.update_stats();
    ASSERT_EQ(grid.get_total_disorder(), 60u);
}

TEST(stats_total_disorder_empty_grid) {
    DisorderGrid grid(64, 64);
    grid.update_stats();
    ASSERT_EQ(grid.get_total_disorder(), 0u);
}

TEST(stats_high_disorder_tiles_default_threshold) {
    DisorderGrid grid(8, 8);
    grid.set_level(0, 0, 127); // below threshold
    grid.set_level(1, 0, 128); // at threshold
    grid.set_level(2, 0, 200); // above threshold
    grid.set_level(3, 0, 255); // max

    ASSERT_EQ(grid.get_high_disorder_tiles(), 3u);
}

TEST(stats_high_disorder_tiles_custom_threshold) {
    DisorderGrid grid(8, 8);
    grid.set_level(0, 0, 50);
    grid.set_level(1, 0, 100);
    grid.set_level(2, 0, 150);
    grid.set_level(3, 0, 200);

    ASSERT_EQ(grid.get_high_disorder_tiles(100), 3u);
    ASSERT_EQ(grid.get_high_disorder_tiles(200), 1u);
    ASSERT_EQ(grid.get_high_disorder_tiles(1), 4u);
}

TEST(stats_update_reflects_changes) {
    DisorderGrid grid(4, 4);
    grid.set_level(0, 0, 50);
    grid.update_stats();
    ASSERT_EQ(grid.get_total_disorder(), 50u);

    grid.set_level(1, 0, 100);
    grid.update_stats();
    ASSERT_EQ(grid.get_total_disorder(), 150u);
}

// =============================================================================
// is_valid Boundary Tests
// =============================================================================

TEST(is_valid_corners) {
    DisorderGrid grid(256, 256);
    ASSERT(grid.is_valid(0, 0));
    ASSERT(grid.is_valid(255, 0));
    ASSERT(grid.is_valid(0, 255));
    ASSERT(grid.is_valid(255, 255));
}

TEST(is_valid_center) {
    DisorderGrid grid(256, 256);
    ASSERT(grid.is_valid(128, 128));
}

TEST(is_valid_out_of_bounds_positive) {
    DisorderGrid grid(256, 256);
    ASSERT(!grid.is_valid(256, 0));
    ASSERT(!grid.is_valid(0, 256));
    ASSERT(!grid.is_valid(256, 256));
    ASSERT(!grid.is_valid(1000, 1000));
}

TEST(is_valid_negative_coordinates) {
    DisorderGrid grid(256, 256);
    ASSERT(!grid.is_valid(-1, 0));
    ASSERT(!grid.is_valid(0, -1));
    ASSERT(!grid.is_valid(-1, -1));
    ASSERT(!grid.is_valid(-100, -100));
}

TEST(out_of_bounds_get_returns_zero) {
    DisorderGrid grid(128, 128);
    grid.set_level(0, 0, 42);
    ASSERT_EQ(grid.get_level(128, 0), 0);
    ASSERT_EQ(grid.get_level(0, 128), 0);
    ASSERT_EQ(grid.get_level(-1, 0), 0);
    ASSERT_EQ(grid.get_level(0, -1), 0);
}

TEST(out_of_bounds_set_is_noop) {
    DisorderGrid grid(128, 128);
    // Should not crash
    grid.set_level(128, 0, 42);
    grid.set_level(0, 128, 42);
    grid.set_level(-1, 0, 42);
    grid.set_level(0, -1, 42);
    // Verify no corruption
    ASSERT_EQ(grid.get_level(0, 0), 0);
    ASSERT_EQ(grid.get_level(127, 127), 0);
}

TEST(out_of_bounds_add_is_noop) {
    DisorderGrid grid(128, 128);
    grid.add_disorder(128, 0, 50);
    grid.add_disorder(-1, 0, 50);
    ASSERT_EQ(grid.get_level(0, 0), 0);
}

TEST(out_of_bounds_suppression_is_noop) {
    DisorderGrid grid(128, 128);
    grid.apply_suppression(128, 0, 50);
    grid.apply_suppression(-1, 0, 50);
    ASSERT_EQ(grid.get_level(0, 0), 0);
}

TEST(out_of_bounds_previous_tick_returns_zero) {
    DisorderGrid grid(128, 128);
    grid.set_level(0, 0, 42);
    grid.swap_buffers();
    ASSERT_EQ(grid.get_level_previous_tick(128, 0), 0);
    ASSERT_EQ(grid.get_level_previous_tick(-1, 0), 0);
}

// =============================================================================
// clear() Tests
// =============================================================================

TEST(clear_zeroes_current_buffer) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 200);
    grid.set_level(20, 20, 150);
    grid.clear();
    ASSERT_EQ(grid.get_level(10, 10), 0);
    ASSERT_EQ(grid.get_level(20, 20), 0);
}

TEST(clear_zeroes_previous_buffer) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 200);
    grid.swap_buffers();
    // previous now has 200
    ASSERT_EQ(grid.get_level_previous_tick(10, 10), 200);

    grid.clear();
    ASSERT_EQ(grid.get_level_previous_tick(10, 10), 0);
    ASSERT_EQ(grid.get_level(10, 10), 0);
}

TEST(clear_resets_stats) {
    DisorderGrid grid(4, 4);
    grid.set_level(0, 0, 200);
    grid.update_stats();
    ASSERT_EQ(grid.get_total_disorder(), 200u);

    grid.clear();
    // Stats should be reset by clear
    ASSERT_EQ(grid.get_total_disorder(), 0u);
}

// =============================================================================
// Raw Data Access Tests
// =============================================================================

TEST(raw_data_access) {
    DisorderGrid grid(4, 4);
    grid.set_level(0, 0, 10);
    grid.set_level(1, 0, 20);
    grid.set_level(0, 1, 30);

    const uint8_t* data = grid.get_raw_data();
    ASSERT_EQ(data[0], 10);  // (0,0)
    ASSERT_EQ(data[1], 20);  // (1,0)
    ASSERT_EQ(data[4], 30);  // (0,1) = y*width + x = 1*4 + 0
}

// =============================================================================
// Memory Size Verification
// =============================================================================

TEST(disorder_cell_size) {
    ASSERT_EQ(sizeof(DisorderCell), 1u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== DisorderGrid Unit Tests (Ticket E10-060) ===\n\n");

    // Construction tests
    RUN_TEST(construction_dimensions);
    RUN_TEST(construction_non_square);
    RUN_TEST(construction_all_cells_zero);
    RUN_TEST(construction_previous_buffer_zero);

    // Get/set tests
    RUN_TEST(set_and_get_single_cell);
    RUN_TEST(set_overwrites_previous_value);
    RUN_TEST(set_does_not_affect_neighbors);
    RUN_TEST(set_corner_cells);
    RUN_TEST(set_full_range);

    // add_disorder tests
    RUN_TEST(add_disorder_basic);
    RUN_TEST(add_disorder_from_zero);
    RUN_TEST(add_disorder_saturates_at_255);
    RUN_TEST(add_disorder_exact_255);
    RUN_TEST(add_disorder_already_max);
    RUN_TEST(add_disorder_max_plus_max);
    RUN_TEST(add_disorder_zero_amount);

    // apply_suppression tests
    RUN_TEST(apply_suppression_basic);
    RUN_TEST(apply_suppression_saturates_at_zero);
    RUN_TEST(apply_suppression_exact_zero);
    RUN_TEST(apply_suppression_from_zero);
    RUN_TEST(apply_suppression_zero_amount);

    // swap_buffers tests
    RUN_TEST(swap_buffers_current_becomes_previous);
    RUN_TEST(swap_buffers_double_swap);
    RUN_TEST(swap_buffers_preserves_all_data);

    // previous tick tests
    RUN_TEST(previous_tick_returns_pre_swap_data);
    RUN_TEST(previous_tick_independent_of_current_writes);

    // Aggregate stats tests
    RUN_TEST(stats_total_disorder);
    RUN_TEST(stats_total_disorder_empty_grid);
    RUN_TEST(stats_high_disorder_tiles_default_threshold);
    RUN_TEST(stats_high_disorder_tiles_custom_threshold);
    RUN_TEST(stats_update_reflects_changes);

    // is_valid / bounds tests
    RUN_TEST(is_valid_corners);
    RUN_TEST(is_valid_center);
    RUN_TEST(is_valid_out_of_bounds_positive);
    RUN_TEST(is_valid_negative_coordinates);
    RUN_TEST(out_of_bounds_get_returns_zero);
    RUN_TEST(out_of_bounds_set_is_noop);
    RUN_TEST(out_of_bounds_add_is_noop);
    RUN_TEST(out_of_bounds_suppression_is_noop);
    RUN_TEST(out_of_bounds_previous_tick_returns_zero);

    // clear() tests
    RUN_TEST(clear_zeroes_current_buffer);
    RUN_TEST(clear_zeroes_previous_buffer);
    RUN_TEST(clear_resets_stats);

    // Raw data tests
    RUN_TEST(raw_data_access);

    // Memory size
    RUN_TEST(disorder_cell_size);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
