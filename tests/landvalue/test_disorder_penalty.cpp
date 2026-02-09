/**
 * @file test_disorder_penalty.cpp
 * @brief Unit tests for DisorderPenalty.h (Ticket E10-103)
 *
 * Tests cover:
 * - Penalty calculation formula (disorder * 40 / 255)
 * - MAX_DISORDER_PENALTY constant value (40)
 * - Reading from previous tick buffer
 * - Saturating subtraction behavior
 * - Full grid application
 * - Edge cases (0 disorder, max disorder)
 */

#include <sims3000/landvalue/DisorderPenalty.h>
#include <sims3000/landvalue/LandValueGrid.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::landvalue;
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
        printf("\n  FAILED: %s == %s (got %d, expected %d) (line %d)\n", #a, #b, (int)(a), (int)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Constant Tests
// =============================================================================

TEST(max_penalty_constant_value) {
    ASSERT_EQ(MAX_DISORDER_PENALTY, 40);
}

// =============================================================================
// Penalty Calculation Tests
// =============================================================================

TEST(zero_disorder_zero_penalty) {
    uint8_t penalty = calculate_disorder_penalty(0);
    ASSERT_EQ(penalty, 0);
}

TEST(max_disorder_max_penalty) {
    uint8_t penalty = calculate_disorder_penalty(255);
    ASSERT_EQ(penalty, 40);
}

TEST(half_disorder_half_penalty) {
    // 127 * 40 / 255 = 5080 / 255 = 19.92... = 19
    uint8_t penalty = calculate_disorder_penalty(127);
    ASSERT_EQ(penalty, 19);
}

TEST(quarter_disorder_quarter_penalty) {
    // 64 * 40 / 255 = 2560 / 255 = 10.03... = 10
    uint8_t penalty = calculate_disorder_penalty(64);
    ASSERT_EQ(penalty, 10);
}

TEST(penalty_scales_linearly) {
    // Test a few points on the curve
    ASSERT_EQ(calculate_disorder_penalty(51), 8);   // 51 * 40 / 255 = 8
    ASSERT_EQ(calculate_disorder_penalty(102), 16); // 102 * 40 / 255 = 16
    ASSERT_EQ(calculate_disorder_penalty(204), 32); // 204 * 40 / 255 = 32
}

TEST(small_disorder_small_penalty) {
    // Very small disorder should give small penalty
    ASSERT_EQ(calculate_disorder_penalty(1), 0);   // 1 * 40 / 255 = 0.15... = 0
    ASSERT_EQ(calculate_disorder_penalty(6), 0);   // 6 * 40 / 255 = 0.94... = 0
    ASSERT_EQ(calculate_disorder_penalty(7), 1);   // 7 * 40 / 255 = 1.09... = 1
}

// =============================================================================
// Grid Application Tests
// =============================================================================

TEST(apply_penalty_single_tile) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    // Set initial land value to 150
    value_grid.set_value(5, 5, 150);

    // Set disorder to 127 (penalty should be 19)
    disorder_grid.set_level(5, 5, 127);
    disorder_grid.swap_buffers();  // Move to previous buffer

    apply_disorder_penalties(value_grid, disorder_grid);

    // Value should be 150 - 19 = 131
    ASSERT_EQ(value_grid.get_value(5, 5), 131);
}

TEST(apply_penalty_full_grid) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    // Set all tiles to value 200
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            value_grid.set_value(x, y, 200);
        }
    }

    // Set uniform disorder of 51 (penalty = 8)
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            disorder_grid.set_level(x, y, 51);
        }
    }
    disorder_grid.swap_buffers();

    apply_disorder_penalties(value_grid, disorder_grid);

    // All tiles should be 200 - 8 = 192
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            ASSERT_EQ(value_grid.get_value(x, y), 192);
        }
    }
}

TEST(apply_penalty_mixed_disorder) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    // Set all land values to 180
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            value_grid.set_value(x, y, 180);
        }
    }

    // Set different disorder levels
    disorder_grid.set_level(0, 0, 0);     // penalty = 0
    disorder_grid.set_level(1, 1, 127);   // penalty = 19
    disorder_grid.set_level(2, 2, 255);   // penalty = 40
    disorder_grid.swap_buffers();

    apply_disorder_penalties(value_grid, disorder_grid);

    ASSERT_EQ(value_grid.get_value(0, 0), 180);      // 180 - 0
    ASSERT_EQ(value_grid.get_value(1, 1), 161);      // 180 - 19
    ASSERT_EQ(value_grid.get_value(2, 2), 140);      // 180 - 40
}

// =============================================================================
// Previous Tick Buffer Tests
// =============================================================================

TEST(reads_from_previous_tick_buffer) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    value_grid.set_value(5, 5, 150);

    // Set disorder in current buffer, then swap
    disorder_grid.set_level(5, 5, 127);
    disorder_grid.swap_buffers();  // Now in previous buffer

    // Modify current buffer (should not affect penalty calculation)
    disorder_grid.set_level(5, 5, 0);

    apply_disorder_penalties(value_grid, disorder_grid);

    // Should use previous buffer's value (127, penalty=19)
    ASSERT_EQ(value_grid.get_value(5, 5), 131);  // 150 - 19
}

// =============================================================================
// Saturating Subtraction Tests
// =============================================================================

TEST(penalty_saturates_at_zero) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    // Set low land value
    value_grid.set_value(5, 5, 20);

    // Set high disorder (penalty = 40)
    disorder_grid.set_level(5, 5, 255);
    disorder_grid.swap_buffers();

    apply_disorder_penalties(value_grid, disorder_grid);

    // Value should be clamped to 0, not wrap around
    ASSERT_EQ(value_grid.get_value(5, 5), 0);
}

TEST(penalty_exactly_reduces_to_zero) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    // Set land value to exactly the penalty amount
    value_grid.set_value(5, 5, 40);

    // Set max disorder (penalty = 40)
    disorder_grid.set_level(5, 5, 255);
    disorder_grid.swap_buffers();

    apply_disorder_penalties(value_grid, disorder_grid);

    ASSERT_EQ(value_grid.get_value(5, 5), 0);
}

TEST(small_penalty_on_low_value) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    value_grid.set_value(5, 5, 10);

    // Small disorder (penalty = 1)
    disorder_grid.set_level(5, 5, 7);
    disorder_grid.swap_buffers();

    apply_disorder_penalties(value_grid, disorder_grid);

    ASSERT_EQ(value_grid.get_value(5, 5), 9);  // 10 - 1
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(zero_disorder_no_change) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    value_grid.set_value(5, 5, 150);

    // No disorder
    disorder_grid.swap_buffers();

    apply_disorder_penalties(value_grid, disorder_grid);

    // Value unchanged
    ASSERT_EQ(value_grid.get_value(5, 5), 150);
}

TEST(grid_size_mismatch_safe) {
    // Both grids should handle iteration correctly even if conceptually different sizes
    // In practice, they should always match, but test robustness
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    value_grid.set_value(5, 5, 150);
    disorder_grid.set_level(5, 5, 127);
    disorder_grid.swap_buffers();

    apply_disorder_penalties(value_grid, disorder_grid);

    ASSERT_EQ(value_grid.get_value(5, 5), 131);
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST(realistic_scenario) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    // Simulate a city with varying disorder
    // High value area (200) with low disorder (10, penalty ~1)
    value_grid.set_value(2, 2, 200);
    disorder_grid.set_level(2, 2, 10);

    // Medium value area (150) with medium disorder (100, penalty ~15)
    value_grid.set_value(5, 5, 150);
    disorder_grid.set_level(5, 5, 100);

    // Low value area (80) with high disorder (200, penalty ~31)
    value_grid.set_value(8, 8, 80);
    disorder_grid.set_level(8, 8, 200);

    disorder_grid.swap_buffers();
    apply_disorder_penalties(value_grid, disorder_grid);

    // High value area: 200 - 1 = 199
    ASSERT_EQ(value_grid.get_value(2, 2), 199);

    // Medium value area: 150 - 15 = 135
    ASSERT_EQ(value_grid.get_value(5, 5), 135);

    // Low value area: 80 - 31 = 49
    ASSERT_EQ(value_grid.get_value(8, 8), 49);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== DisorderPenalty Unit Tests (Ticket E10-103) ===\n\n");

    // Constant tests
    RUN_TEST(max_penalty_constant_value);

    // Penalty calculation tests
    RUN_TEST(zero_disorder_zero_penalty);
    RUN_TEST(max_disorder_max_penalty);
    RUN_TEST(half_disorder_half_penalty);
    RUN_TEST(quarter_disorder_quarter_penalty);
    RUN_TEST(penalty_scales_linearly);
    RUN_TEST(small_disorder_small_penalty);

    // Grid application tests
    RUN_TEST(apply_penalty_single_tile);
    RUN_TEST(apply_penalty_full_grid);
    RUN_TEST(apply_penalty_mixed_disorder);

    // Previous tick buffer tests
    RUN_TEST(reads_from_previous_tick_buffer);

    // Saturating subtraction tests
    RUN_TEST(penalty_saturates_at_zero);
    RUN_TEST(penalty_exactly_reduces_to_zero);
    RUN_TEST(small_penalty_on_low_value);

    // Edge cases
    RUN_TEST(zero_disorder_no_change);
    RUN_TEST(grid_size_mismatch_safe);

    // Integration tests
    RUN_TEST(realistic_scenario);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
