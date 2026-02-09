/**
 * @file test_contamination_grid.cpp
 * @brief Unit tests for ContaminationGrid (Ticket E10-061)
 *
 * Tests cover:
 * - Construction with dimensions
 * - get/set level and dominant type
 * - add_contamination saturating + dominant type tracking
 * - apply_decay saturating subtraction
 * - swap_buffers: current becomes previous
 * - Previous tick accessors
 * - Aggregate stats
 * - is_valid boundary checks
 */

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
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Construction Tests
// =============================================================================

TEST(construction_dimensions) {
    ContaminationGrid grid(256, 256);
    ASSERT_EQ(grid.get_width(), static_cast<uint16_t>(256));
    ASSERT_EQ(grid.get_height(), static_cast<uint16_t>(256));
}

TEST(construction_non_square) {
    ContaminationGrid grid(128, 64);
    ASSERT_EQ(grid.get_width(), static_cast<uint16_t>(128));
    ASSERT_EQ(grid.get_height(), static_cast<uint16_t>(64));
}

TEST(construction_all_cells_zero) {
    ContaminationGrid grid(128, 128);
    ASSERT_EQ(grid.get_level(0, 0), 0);
    ASSERT_EQ(grid.get_dominant_type(0, 0), 0);
    ASSERT_EQ(grid.get_level(64, 64), 0);
    ASSERT_EQ(grid.get_level(127, 127), 0);
}

TEST(construction_previous_buffer_zero) {
    ContaminationGrid grid(128, 128);
    ASSERT_EQ(grid.get_level_previous_tick(0, 0), 0);
    ASSERT_EQ(grid.get_dominant_type_previous_tick(0, 0), 0);
    ASSERT_EQ(grid.get_level_previous_tick(64, 64), 0);
}

// =============================================================================
// Get/Set Level and Dominant Type Tests
// =============================================================================

TEST(set_and_get_level) {
    ContaminationGrid grid(128, 128);
    grid.set_level(10, 20, 42);
    ASSERT_EQ(grid.get_level(10, 20), 42);
}

TEST(set_level_does_not_affect_type) {
    ContaminationGrid grid(128, 128);
    grid.add_contamination(10, 20, 50, 3);
    ASSERT_EQ(grid.get_dominant_type(10, 20), 3);
    grid.set_level(10, 20, 100);
    // set_level should not change the dominant type
    ASSERT_EQ(grid.get_dominant_type(10, 20), 3);
}

TEST(set_level_overwrites) {
    ContaminationGrid grid(128, 128);
    grid.set_level(5, 5, 100);
    ASSERT_EQ(grid.get_level(5, 5), 100);
    grid.set_level(5, 5, 200);
    ASSERT_EQ(grid.get_level(5, 5), 200);
}

TEST(set_does_not_affect_neighbors) {
    ContaminationGrid grid(128, 128);
    grid.set_level(50, 50, 255);
    ASSERT_EQ(grid.get_level(49, 50), 0);
    ASSERT_EQ(grid.get_level(51, 50), 0);
    ASSERT_EQ(grid.get_level(50, 49), 0);
    ASSERT_EQ(grid.get_level(50, 51), 0);
}

// =============================================================================
// add_contamination Tests
// =============================================================================

TEST(add_contamination_basic) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 50, 1);
    ASSERT_EQ(grid.get_level(5, 5), 50);
    ASSERT_EQ(grid.get_dominant_type(5, 5), 1);
}

TEST(add_contamination_accumulates) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 50, 1);
    grid.add_contamination(5, 5, 30, 2);
    ASSERT_EQ(grid.get_level(5, 5), 80);
}

TEST(add_contamination_saturates_at_255) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 200, 1);
    grid.add_contamination(5, 5, 200, 1);
    ASSERT_EQ(grid.get_level(5, 5), 255);
}

TEST(add_contamination_exact_255) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 200, 1);
    grid.add_contamination(5, 5, 55, 1);
    ASSERT_EQ(grid.get_level(5, 5), 255);
}

TEST(add_contamination_already_max) {
    ContaminationGrid grid(64, 64);
    grid.set_level(5, 5, 255);
    grid.add_contamination(5, 5, 1, 1);
    ASSERT_EQ(grid.get_level(5, 5), 255);
}

TEST(add_contamination_max_plus_max) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 255, 1);
    grid.add_contamination(5, 5, 255, 2);
    ASSERT_EQ(grid.get_level(5, 5), 255);
}

TEST(add_contamination_updates_dominant_type) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 50, 1);
    ASSERT_EQ(grid.get_dominant_type(5, 5), 1);
    // Adding more with type 2 should update dominant
    grid.add_contamination(5, 5, 60, 2);
    ASSERT_EQ(grid.get_dominant_type(5, 5), 2);
}

TEST(add_contamination_zero_amount_on_empty) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 0, 1);
    ASSERT_EQ(grid.get_level(5, 5), 0);
}

// =============================================================================
// apply_decay Tests
// =============================================================================

TEST(apply_decay_basic) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 100, 1);
    grid.apply_decay(5, 5, 30);
    ASSERT_EQ(grid.get_level(5, 5), 70);
}

TEST(apply_decay_saturates_at_zero) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 50, 1);
    grid.apply_decay(5, 5, 200);
    ASSERT_EQ(grid.get_level(5, 5), 0);
}

TEST(apply_decay_exact_zero) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 100, 1);
    grid.apply_decay(5, 5, 100);
    ASSERT_EQ(grid.get_level(5, 5), 0);
}

TEST(apply_decay_resets_type_at_zero) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 50, 3);
    ASSERT_EQ(grid.get_dominant_type(5, 5), 3);
    grid.apply_decay(5, 5, 50);
    ASSERT_EQ(grid.get_level(5, 5), 0);
    ASSERT_EQ(grid.get_dominant_type(5, 5), 0);
}

TEST(apply_decay_preserves_type_above_zero) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 100, 3);
    grid.apply_decay(5, 5, 50);
    ASSERT_EQ(grid.get_level(5, 5), 50);
    ASSERT_EQ(grid.get_dominant_type(5, 5), 3);
}

TEST(apply_decay_from_zero) {
    ContaminationGrid grid(64, 64);
    grid.apply_decay(5, 5, 50);
    ASSERT_EQ(grid.get_level(5, 5), 0);
}

TEST(apply_decay_zero_amount) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 100, 1);
    grid.apply_decay(5, 5, 0);
    ASSERT_EQ(grid.get_level(5, 5), 100);
}

// =============================================================================
// swap_buffers Tests
// =============================================================================

TEST(swap_buffers_current_becomes_previous) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 42, 2);
    ASSERT_EQ(grid.get_level(10, 10), 42);
    ASSERT_EQ(grid.get_dominant_type(10, 10), 2);
    ASSERT_EQ(grid.get_level_previous_tick(10, 10), 0);

    grid.swap_buffers();

    ASSERT_EQ(grid.get_level_previous_tick(10, 10), 42);
    ASSERT_EQ(grid.get_dominant_type_previous_tick(10, 10), 2);
    ASSERT_EQ(grid.get_level(10, 10), 0);
    ASSERT_EQ(grid.get_dominant_type(10, 10), 0);
}

TEST(swap_buffers_double_swap) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, 1);

    grid.swap_buffers();
    ASSERT_EQ(grid.get_level(10, 10), 0);
    ASSERT_EQ(grid.get_level_previous_tick(10, 10), 100);

    grid.add_contamination(10, 10, 200, 3);
    grid.swap_buffers();
    ASSERT_EQ(grid.get_level(10, 10), 100);
    ASSERT_EQ(grid.get_dominant_type(10, 10), 1);
    ASSERT_EQ(grid.get_level_previous_tick(10, 10), 200);
    ASSERT_EQ(grid.get_dominant_type_previous_tick(10, 10), 3);
}

TEST(swap_buffers_preserves_all_data) {
    ContaminationGrid grid(16, 16);
    for (int32_t y = 0; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            grid.add_contamination(x, y, static_cast<uint8_t>((x + y) % 256),
                                   static_cast<uint8_t>((x * y) % 5 + 1));
        }
    }

    grid.swap_buffers();

    for (int32_t y = 0; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            ASSERT_EQ(grid.get_level_previous_tick(x, y),
                      static_cast<uint8_t>((x + y) % 256));
        }
    }

    for (int32_t y = 0; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            ASSERT_EQ(grid.get_level(x, y), 0);
        }
    }
}

// =============================================================================
// Previous Tick Accessor Tests
// =============================================================================

TEST(previous_tick_level_returns_pre_swap_data) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 77, 2);
    grid.swap_buffers();
    ASSERT_EQ(grid.get_level_previous_tick(5, 5), 77);
}

TEST(previous_tick_type_returns_pre_swap_data) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 77, 4);
    grid.swap_buffers();
    ASSERT_EQ(grid.get_dominant_type_previous_tick(5, 5), 4);
}

TEST(previous_tick_independent_of_current_writes) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(5, 5, 77, 2);
    grid.swap_buffers();
    grid.add_contamination(5, 5, 99, 3);
    ASSERT_EQ(grid.get_level_previous_tick(5, 5), 77);
    ASSERT_EQ(grid.get_dominant_type_previous_tick(5, 5), 2);
    ASSERT_EQ(grid.get_level(5, 5), 99);
    ASSERT_EQ(grid.get_dominant_type(5, 5), 3);
}

// =============================================================================
// Aggregate Stats Tests
// =============================================================================

TEST(stats_total_contamination) {
    ContaminationGrid grid(4, 4);
    grid.add_contamination(0, 0, 10, 1);
    grid.add_contamination(1, 0, 20, 1);
    grid.add_contamination(2, 0, 30, 1);
    grid.update_stats();
    ASSERT_EQ(grid.get_total_contamination(), 60u);
}

TEST(stats_total_contamination_empty) {
    ContaminationGrid grid(64, 64);
    grid.update_stats();
    ASSERT_EQ(grid.get_total_contamination(), 0u);
}

TEST(stats_toxic_tiles_default_threshold) {
    ContaminationGrid grid(8, 8);
    grid.add_contamination(0, 0, 127, 1); // below
    grid.add_contamination(1, 0, 128, 1); // at threshold
    grid.add_contamination(2, 0, 200, 1); // above
    grid.add_contamination(3, 0, 255, 1); // max

    ASSERT_EQ(grid.get_toxic_tiles(), 3u);
}

TEST(stats_toxic_tiles_custom_threshold) {
    ContaminationGrid grid(8, 8);
    grid.add_contamination(0, 0, 50, 1);
    grid.add_contamination(1, 0, 100, 1);
    grid.add_contamination(2, 0, 150, 1);
    grid.add_contamination(3, 0, 200, 1);

    ASSERT_EQ(grid.get_toxic_tiles(100), 3u);
    ASSERT_EQ(grid.get_toxic_tiles(200), 1u);
    ASSERT_EQ(grid.get_toxic_tiles(1), 4u);
}

// =============================================================================
// is_valid Boundary Tests
// =============================================================================

TEST(is_valid_corners) {
    ContaminationGrid grid(256, 256);
    ASSERT(grid.is_valid(0, 0));
    ASSERT(grid.is_valid(255, 0));
    ASSERT(grid.is_valid(0, 255));
    ASSERT(grid.is_valid(255, 255));
}

TEST(is_valid_out_of_bounds_positive) {
    ContaminationGrid grid(256, 256);
    ASSERT(!grid.is_valid(256, 0));
    ASSERT(!grid.is_valid(0, 256));
    ASSERT(!grid.is_valid(256, 256));
    ASSERT(!grid.is_valid(1000, 1000));
}

TEST(is_valid_negative_coordinates) {
    ContaminationGrid grid(256, 256);
    ASSERT(!grid.is_valid(-1, 0));
    ASSERT(!grid.is_valid(0, -1));
    ASSERT(!grid.is_valid(-1, -1));
}

TEST(out_of_bounds_get_returns_zero) {
    ContaminationGrid grid(128, 128);
    grid.add_contamination(0, 0, 42, 2);
    ASSERT_EQ(grid.get_level(128, 0), 0);
    ASSERT_EQ(grid.get_dominant_type(128, 0), 0);
    ASSERT_EQ(grid.get_level(-1, 0), 0);
    ASSERT_EQ(grid.get_dominant_type(-1, 0), 0);
}

TEST(out_of_bounds_set_is_noop) {
    ContaminationGrid grid(128, 128);
    grid.set_level(128, 0, 42);
    grid.set_level(-1, 0, 42);
    ASSERT_EQ(grid.get_level(0, 0), 0);
}

TEST(out_of_bounds_add_is_noop) {
    ContaminationGrid grid(128, 128);
    grid.add_contamination(128, 0, 50, 1);
    grid.add_contamination(-1, 0, 50, 1);
    ASSERT_EQ(grid.get_level(0, 0), 0);
}

TEST(out_of_bounds_decay_is_noop) {
    ContaminationGrid grid(128, 128);
    grid.apply_decay(128, 0, 50);
    grid.apply_decay(-1, 0, 50);
    ASSERT_EQ(grid.get_level(0, 0), 0);
}

TEST(out_of_bounds_previous_tick_returns_zero) {
    ContaminationGrid grid(128, 128);
    grid.add_contamination(0, 0, 42, 2);
    grid.swap_buffers();
    ASSERT_EQ(grid.get_level_previous_tick(128, 0), 0);
    ASSERT_EQ(grid.get_dominant_type_previous_tick(128, 0), 0);
    ASSERT_EQ(grid.get_level_previous_tick(-1, 0), 0);
}

// =============================================================================
// clear() Tests
// =============================================================================

TEST(clear_zeroes_both_buffers) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 200, 3);
    grid.swap_buffers();
    grid.add_contamination(20, 20, 150, 2);

    grid.clear();

    ASSERT_EQ(grid.get_level(10, 10), 0);
    ASSERT_EQ(grid.get_level(20, 20), 0);
    ASSERT_EQ(grid.get_level_previous_tick(10, 10), 0);
    ASSERT_EQ(grid.get_dominant_type(20, 20), 0);
    ASSERT_EQ(grid.get_dominant_type_previous_tick(10, 10), 0);
}

// =============================================================================
// Memory Size Verification
// =============================================================================

TEST(contamination_cell_size) {
    ASSERT_EQ(sizeof(ContaminationCell), 2u);
}

// =============================================================================
// Level Data Access Tests
// =============================================================================

TEST(level_data_access) {
    ContaminationGrid grid(4, 4);
    grid.add_contamination(0, 0, 10, 1);
    grid.add_contamination(1, 0, 20, 2);
    grid.add_contamination(0, 1, 30, 3);

    const uint8_t* data = grid.get_level_data();
    ASSERT_EQ(data[0], 10);  // (0,0)
    ASSERT_EQ(data[1], 20);  // (1,0)
    ASSERT_EQ(data[4], 30);  // (0,1) = y*width + x = 1*4 + 0
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== ContaminationGrid Unit Tests (Ticket E10-061) ===\n\n");

    // Construction tests
    RUN_TEST(construction_dimensions);
    RUN_TEST(construction_non_square);
    RUN_TEST(construction_all_cells_zero);
    RUN_TEST(construction_previous_buffer_zero);

    // Get/set tests
    RUN_TEST(set_and_get_level);
    RUN_TEST(set_level_does_not_affect_type);
    RUN_TEST(set_level_overwrites);
    RUN_TEST(set_does_not_affect_neighbors);

    // add_contamination tests
    RUN_TEST(add_contamination_basic);
    RUN_TEST(add_contamination_accumulates);
    RUN_TEST(add_contamination_saturates_at_255);
    RUN_TEST(add_contamination_exact_255);
    RUN_TEST(add_contamination_already_max);
    RUN_TEST(add_contamination_max_plus_max);
    RUN_TEST(add_contamination_updates_dominant_type);
    RUN_TEST(add_contamination_zero_amount_on_empty);

    // apply_decay tests
    RUN_TEST(apply_decay_basic);
    RUN_TEST(apply_decay_saturates_at_zero);
    RUN_TEST(apply_decay_exact_zero);
    RUN_TEST(apply_decay_resets_type_at_zero);
    RUN_TEST(apply_decay_preserves_type_above_zero);
    RUN_TEST(apply_decay_from_zero);
    RUN_TEST(apply_decay_zero_amount);

    // swap_buffers tests
    RUN_TEST(swap_buffers_current_becomes_previous);
    RUN_TEST(swap_buffers_double_swap);
    RUN_TEST(swap_buffers_preserves_all_data);

    // Previous tick tests
    RUN_TEST(previous_tick_level_returns_pre_swap_data);
    RUN_TEST(previous_tick_type_returns_pre_swap_data);
    RUN_TEST(previous_tick_independent_of_current_writes);

    // Aggregate stats tests
    RUN_TEST(stats_total_contamination);
    RUN_TEST(stats_total_contamination_empty);
    RUN_TEST(stats_toxic_tiles_default_threshold);
    RUN_TEST(stats_toxic_tiles_custom_threshold);

    // is_valid / bounds tests
    RUN_TEST(is_valid_corners);
    RUN_TEST(is_valid_out_of_bounds_positive);
    RUN_TEST(is_valid_negative_coordinates);
    RUN_TEST(out_of_bounds_get_returns_zero);
    RUN_TEST(out_of_bounds_set_is_noop);
    RUN_TEST(out_of_bounds_add_is_noop);
    RUN_TEST(out_of_bounds_decay_is_noop);
    RUN_TEST(out_of_bounds_previous_tick_returns_zero);

    // clear() tests
    RUN_TEST(clear_zeroes_both_buffers);

    // Memory size
    RUN_TEST(contamination_cell_size);

    // Level data access
    RUN_TEST(level_data_access);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
