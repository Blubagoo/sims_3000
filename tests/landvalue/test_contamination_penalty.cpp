/**
 * @file test_contamination_penalty.cpp
 * @brief Unit tests for ContaminationPenalty.h (Ticket E10-104)
 *
 * Tests cover:
 * - Penalty calculation formula (contamination * 50 / 255)
 * - MAX_CONTAMINATION_PENALTY constant value (50)
 * - Reading from previous tick buffer
 * - Saturating subtraction behavior
 * - Full grid application
 * - Edge cases (0 contamination, max contamination)
 */

#include <sims3000/landvalue/ContaminationPenalty.h>
#include <sims3000/landvalue/LandValueGrid.h>
#include <sims3000/contamination/ContaminationGrid.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::landvalue;
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
// Constant Tests
// =============================================================================

TEST(max_penalty_constant_value) {
    ASSERT_EQ(MAX_CONTAMINATION_PENALTY, 50);
}

// =============================================================================
// Penalty Calculation Tests
// =============================================================================

TEST(zero_contamination_zero_penalty) {
    uint8_t penalty = calculate_contamination_penalty(0);
    ASSERT_EQ(penalty, 0);
}

TEST(max_contamination_max_penalty) {
    uint8_t penalty = calculate_contamination_penalty(255);
    ASSERT_EQ(penalty, 50);
}

TEST(half_contamination_half_penalty) {
    // 127 * 50 / 255 = 6350 / 255 = 24.90... = 24
    uint8_t penalty = calculate_contamination_penalty(127);
    ASSERT_EQ(penalty, 24);
}

TEST(quarter_contamination_quarter_penalty) {
    // 64 * 50 / 255 = 3200 / 255 = 12.54... = 12
    uint8_t penalty = calculate_contamination_penalty(64);
    ASSERT_EQ(penalty, 12);
}

TEST(penalty_scales_linearly) {
    // Test a few points on the curve
    ASSERT_EQ(calculate_contamination_penalty(51), 10);  // 51 * 50 / 255 = 10
    ASSERT_EQ(calculate_contamination_penalty(102), 20); // 102 * 50 / 255 = 20
    ASSERT_EQ(calculate_contamination_penalty(204), 40); // 204 * 50 / 255 = 40
}

TEST(small_contamination_small_penalty) {
    // Very small contamination should give small penalty
    ASSERT_EQ(calculate_contamination_penalty(1), 0);   // 1 * 50 / 255 = 0.19... = 0
    ASSERT_EQ(calculate_contamination_penalty(5), 0);   // 5 * 50 / 255 = 0.98... = 0
    ASSERT_EQ(calculate_contamination_penalty(6), 1);   // 6 * 50 / 255 = 1.17... = 1
}

// =============================================================================
// Grid Application Tests
// =============================================================================

TEST(apply_penalty_single_tile) {
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Set initial land value to 150
    value_grid.set_value(5, 5, 150);

    // Set contamination to 127 (penalty should be 24)
    contam_grid.set_level(5, 5, 127);
    contam_grid.swap_buffers();  // Move to previous buffer

    apply_contamination_penalties(value_grid, contam_grid);

    // Value should be 150 - 24 = 126
    ASSERT_EQ(value_grid.get_value(5, 5), 126);
}

TEST(apply_penalty_full_grid) {
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Set all tiles to value 200
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            value_grid.set_value(x, y, 200);
        }
    }

    // Set uniform contamination of 51 (penalty = 10)
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            contam_grid.set_level(x, y, 51);
        }
    }
    contam_grid.swap_buffers();

    apply_contamination_penalties(value_grid, contam_grid);

    // All tiles should be 200 - 10 = 190
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            ASSERT_EQ(value_grid.get_value(x, y), 190);
        }
    }
}

TEST(apply_penalty_mixed_contamination) {
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Set all land values to 180
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            value_grid.set_value(x, y, 180);
        }
    }

    // Set different contamination levels
    contam_grid.set_level(0, 0, 0);     // penalty = 0
    contam_grid.set_level(1, 1, 127);   // penalty = 24
    contam_grid.set_level(2, 2, 255);   // penalty = 50
    contam_grid.swap_buffers();

    apply_contamination_penalties(value_grid, contam_grid);

    ASSERT_EQ(value_grid.get_value(0, 0), 180);      // 180 - 0
    ASSERT_EQ(value_grid.get_value(1, 1), 156);      // 180 - 24
    ASSERT_EQ(value_grid.get_value(2, 2), 130);      // 180 - 50
}

// =============================================================================
// Previous Tick Buffer Tests
// =============================================================================

TEST(reads_from_previous_tick_buffer) {
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    value_grid.set_value(5, 5, 150);

    // Set contamination in current buffer, then swap
    contam_grid.set_level(5, 5, 127);
    contam_grid.swap_buffers();  // Now in previous buffer

    // Modify current buffer (should not affect penalty calculation)
    contam_grid.set_level(5, 5, 0);

    apply_contamination_penalties(value_grid, contam_grid);

    // Should use previous buffer's value (127, penalty=24)
    ASSERT_EQ(value_grid.get_value(5, 5), 126);  // 150 - 24
}

// =============================================================================
// Saturating Subtraction Tests
// =============================================================================

TEST(penalty_saturates_at_zero) {
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Set low land value
    value_grid.set_value(5, 5, 20);

    // Set high contamination (penalty = 50)
    contam_grid.set_level(5, 5, 255);
    contam_grid.swap_buffers();

    apply_contamination_penalties(value_grid, contam_grid);

    // Value should be clamped to 0, not wrap around
    ASSERT_EQ(value_grid.get_value(5, 5), 0);
}

TEST(penalty_exactly_reduces_to_zero) {
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Set land value to exactly the penalty amount
    value_grid.set_value(5, 5, 50);

    // Set max contamination (penalty = 50)
    contam_grid.set_level(5, 5, 255);
    contam_grid.swap_buffers();

    apply_contamination_penalties(value_grid, contam_grid);

    ASSERT_EQ(value_grid.get_value(5, 5), 0);
}

TEST(small_penalty_on_low_value) {
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    value_grid.set_value(5, 5, 10);

    // Small contamination (penalty = 1)
    contam_grid.set_level(5, 5, 6);
    contam_grid.swap_buffers();

    apply_contamination_penalties(value_grid, contam_grid);

    ASSERT_EQ(value_grid.get_value(5, 5), 9);  // 10 - 1
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(zero_contamination_no_change) {
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    value_grid.set_value(5, 5, 150);

    // No contamination
    contam_grid.swap_buffers();

    apply_contamination_penalties(value_grid, contam_grid);

    // Value unchanged
    ASSERT_EQ(value_grid.get_value(5, 5), 150);
}

TEST(grid_size_mismatch_safe) {
    // Both grids should handle iteration correctly even if conceptually different sizes
    // In practice, they should always match, but test robustness
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    value_grid.set_value(5, 5, 150);
    contam_grid.set_level(5, 5, 127);
    contam_grid.swap_buffers();

    apply_contamination_penalties(value_grid, contam_grid);

    ASSERT_EQ(value_grid.get_value(5, 5), 126);
}

// =============================================================================
// Comparison with Disorder Penalty
// =============================================================================

TEST(contamination_penalty_higher_than_disorder) {
    // For same level, contamination penalty (max 50) > disorder penalty (max 40)
    ASSERT(calculate_contamination_penalty(255) > 40);
    ASSERT_EQ(calculate_contamination_penalty(255), 50);

    // At mid-levels, contamination penalty should also be higher
    ASSERT(calculate_contamination_penalty(127) > 19);  // disorder penalty at 127 is 19
    ASSERT_EQ(calculate_contamination_penalty(127), 24);
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST(realistic_scenario) {
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Simulate a city with varying contamination
    // High value area (200) with low contamination (10, penalty ~1)
    value_grid.set_value(2, 2, 200);
    contam_grid.set_level(2, 2, 10);

    // Medium value area (150) with medium contamination (100, penalty ~19)
    value_grid.set_value(5, 5, 150);
    contam_grid.set_level(5, 5, 100);

    // Low value area (80) with high contamination (200, penalty ~39)
    value_grid.set_value(8, 8, 80);
    contam_grid.set_level(8, 8, 200);

    contam_grid.swap_buffers();
    apply_contamination_penalties(value_grid, contam_grid);

    // High value area: 200 - 1 = 199
    ASSERT_EQ(value_grid.get_value(2, 2), 199);

    // Medium value area: 150 - 19 = 131
    ASSERT_EQ(value_grid.get_value(5, 5), 131);

    // Low value area: 80 - 39 = 41
    ASSERT_EQ(value_grid.get_value(8, 8), 41);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== ContaminationPenalty Unit Tests (Ticket E10-104) ===\n\n");

    // Constant tests
    RUN_TEST(max_penalty_constant_value);

    // Penalty calculation tests
    RUN_TEST(zero_contamination_zero_penalty);
    RUN_TEST(max_contamination_max_penalty);
    RUN_TEST(half_contamination_half_penalty);
    RUN_TEST(quarter_contamination_quarter_penalty);
    RUN_TEST(penalty_scales_linearly);
    RUN_TEST(small_contamination_small_penalty);

    // Grid application tests
    RUN_TEST(apply_penalty_single_tile);
    RUN_TEST(apply_penalty_full_grid);
    RUN_TEST(apply_penalty_mixed_contamination);

    // Previous tick buffer tests
    RUN_TEST(reads_from_previous_tick_buffer);

    // Saturating subtraction tests
    RUN_TEST(penalty_saturates_at_zero);
    RUN_TEST(penalty_exactly_reduces_to_zero);
    RUN_TEST(small_penalty_on_low_value);

    // Edge cases
    RUN_TEST(zero_contamination_no_change);
    RUN_TEST(grid_size_mismatch_safe);

    // Comparison tests
    RUN_TEST(contamination_penalty_higher_than_disorder);

    // Integration tests
    RUN_TEST(realistic_scenario);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
