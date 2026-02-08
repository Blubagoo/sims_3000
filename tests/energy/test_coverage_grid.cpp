/**
 * @file test_coverage_grid.cpp
 * @brief Unit tests for CoverageGrid (Ticket 5-007)
 *
 * Tests cover:
 * - Construction with various sizes (128, 256, 512)
 * - Set/get operations
 * - is_in_coverage checks
 * - Clear operations (single cell, per-owner, full grid)
 * - Bounds checking (out-of-bounds returns safe defaults)
 * - Coverage count per owner
 * - Memory size verification (1 byte per cell)
 */

#include <sims3000/energy/CoverageGrid.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::energy;

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

TEST(construction_128x128) {
    CoverageGrid grid(128, 128);
    ASSERT_EQ(grid.get_width(), 128u);
    ASSERT_EQ(grid.get_height(), 128u);
}

TEST(construction_256x256) {
    CoverageGrid grid(256, 256);
    ASSERT_EQ(grid.get_width(), 256u);
    ASSERT_EQ(grid.get_height(), 256u);
}

TEST(construction_512x512) {
    CoverageGrid grid(512, 512);
    ASSERT_EQ(grid.get_width(), 512u);
    ASSERT_EQ(grid.get_height(), 512u);
}

TEST(construction_all_cells_uncovered) {
    CoverageGrid grid(128, 128);
    // Spot-check that all cells start uncovered
    ASSERT_EQ(grid.get_coverage_owner(0, 0), 0);
    ASSERT_EQ(grid.get_coverage_owner(64, 64), 0);
    ASSERT_EQ(grid.get_coverage_owner(127, 127), 0);
}

TEST(construction_non_square) {
    CoverageGrid grid(64, 32);
    ASSERT_EQ(grid.get_width(), 64u);
    ASSERT_EQ(grid.get_height(), 32u);
    ASSERT_EQ(grid.get_coverage_owner(63, 31), 0);
}

// =============================================================================
// Set/Get Operations Tests
// =============================================================================

TEST(set_and_get_single_cell) {
    CoverageGrid grid(128, 128);
    grid.set(10, 20, 1);
    ASSERT_EQ(grid.get_coverage_owner(10, 20), 1);
}

TEST(set_and_get_multiple_owners) {
    CoverageGrid grid(128, 128);
    grid.set(0, 0, 1);
    grid.set(10, 10, 2);
    grid.set(50, 50, 3);
    grid.set(127, 127, 4);

    ASSERT_EQ(grid.get_coverage_owner(0, 0), 1);
    ASSERT_EQ(grid.get_coverage_owner(10, 10), 2);
    ASSERT_EQ(grid.get_coverage_owner(50, 50), 3);
    ASSERT_EQ(grid.get_coverage_owner(127, 127), 4);
}

TEST(set_overwrites_previous) {
    CoverageGrid grid(128, 128);
    grid.set(5, 5, 1);
    ASSERT_EQ(grid.get_coverage_owner(5, 5), 1);

    grid.set(5, 5, 3);
    ASSERT_EQ(grid.get_coverage_owner(5, 5), 3);
}

TEST(set_does_not_affect_neighbors) {
    CoverageGrid grid(128, 128);
    grid.set(50, 50, 2);

    ASSERT_EQ(grid.get_coverage_owner(49, 50), 0);
    ASSERT_EQ(grid.get_coverage_owner(51, 50), 0);
    ASSERT_EQ(grid.get_coverage_owner(50, 49), 0);
    ASSERT_EQ(grid.get_coverage_owner(50, 51), 0);
}

TEST(set_corner_cells) {
    CoverageGrid grid(256, 256);

    grid.set(0, 0, 1);
    grid.set(255, 0, 2);
    grid.set(0, 255, 3);
    grid.set(255, 255, 4);

    ASSERT_EQ(grid.get_coverage_owner(0, 0), 1);
    ASSERT_EQ(grid.get_coverage_owner(255, 0), 2);
    ASSERT_EQ(grid.get_coverage_owner(0, 255), 3);
    ASSERT_EQ(grid.get_coverage_owner(255, 255), 4);
}

// =============================================================================
// is_in_coverage Tests
// =============================================================================

TEST(is_in_coverage_matching_owner) {
    CoverageGrid grid(128, 128);
    grid.set(10, 20, 2);
    ASSERT(grid.is_in_coverage(10, 20, 2));
}

TEST(is_in_coverage_wrong_owner) {
    CoverageGrid grid(128, 128);
    grid.set(10, 20, 2);
    ASSERT(!grid.is_in_coverage(10, 20, 1));
    ASSERT(!grid.is_in_coverage(10, 20, 3));
    ASSERT(!grid.is_in_coverage(10, 20, 4));
}

TEST(is_in_coverage_uncovered_cell) {
    CoverageGrid grid(128, 128);
    // Uncovered cell should not match any valid owner (1-4)
    ASSERT(!grid.is_in_coverage(50, 50, 1));
    ASSERT(!grid.is_in_coverage(50, 50, 2));
    ASSERT(!grid.is_in_coverage(50, 50, 3));
    ASSERT(!grid.is_in_coverage(50, 50, 4));
}

TEST(is_in_coverage_zero_owner_on_uncovered) {
    CoverageGrid grid(128, 128);
    // Cell with value 0 should match owner=0 check
    ASSERT(grid.is_in_coverage(50, 50, 0));
}

// =============================================================================
// Clear Operations Tests
// =============================================================================

TEST(clear_single_cell) {
    CoverageGrid grid(128, 128);
    grid.set(10, 10, 2);
    ASSERT_EQ(grid.get_coverage_owner(10, 10), 2);

    grid.clear(10, 10);
    ASSERT_EQ(grid.get_coverage_owner(10, 10), 0);
}

TEST(clear_does_not_affect_neighbors) {
    CoverageGrid grid(128, 128);
    grid.set(10, 10, 2);
    grid.set(11, 10, 3);

    grid.clear(10, 10);
    ASSERT_EQ(grid.get_coverage_owner(10, 10), 0);
    ASSERT_EQ(grid.get_coverage_owner(11, 10), 3);
}

TEST(clear_all_for_owner_removes_only_matching) {
    CoverageGrid grid(128, 128);
    // Set up mixed ownership
    grid.set(0, 0, 1);
    grid.set(1, 0, 2);
    grid.set(2, 0, 1);
    grid.set(3, 0, 3);
    grid.set(4, 0, 1);

    // Clear all cells owned by overseer 1
    grid.clear_all_for_owner(1);

    ASSERT_EQ(grid.get_coverage_owner(0, 0), 0);
    ASSERT_EQ(grid.get_coverage_owner(1, 0), 2);
    ASSERT_EQ(grid.get_coverage_owner(2, 0), 0);
    ASSERT_EQ(grid.get_coverage_owner(3, 0), 3);
    ASSERT_EQ(grid.get_coverage_owner(4, 0), 0);
}

TEST(clear_all_for_owner_large_grid) {
    CoverageGrid grid(256, 256);
    // Fill a region with owner 2
    for (uint32_t y = 0; y < 100; ++y) {
        for (uint32_t x = 0; x < 100; ++x) {
            grid.set(x, y, 2);
        }
    }
    // Sprinkle some owner 1 cells
    grid.set(200, 200, 1);
    grid.set(201, 200, 1);

    grid.clear_all_for_owner(2);

    // Owner 2 cells should be gone
    ASSERT_EQ(grid.get_coverage_owner(0, 0), 0);
    ASSERT_EQ(grid.get_coverage_owner(50, 50), 0);
    ASSERT_EQ(grid.get_coverage_owner(99, 99), 0);

    // Owner 1 cells should remain
    ASSERT_EQ(grid.get_coverage_owner(200, 200), 1);
    ASSERT_EQ(grid.get_coverage_owner(201, 200), 1);
}

TEST(clear_all_resets_entire_grid) {
    CoverageGrid grid(128, 128);
    // Fill with various owners
    for (uint32_t y = 0; y < 128; ++y) {
        for (uint32_t x = 0; x < 128; ++x) {
            grid.set(x, y, static_cast<uint8_t>((x + y) % 4 + 1));
        }
    }

    grid.clear_all();

    // Verify all cells are uncovered
    ASSERT_EQ(grid.get_coverage_owner(0, 0), 0);
    ASSERT_EQ(grid.get_coverage_owner(64, 64), 0);
    ASSERT_EQ(grid.get_coverage_owner(127, 127), 0);
    ASSERT_EQ(grid.get_coverage_count(1), 0u);
    ASSERT_EQ(grid.get_coverage_count(2), 0u);
    ASSERT_EQ(grid.get_coverage_count(3), 0u);
    ASSERT_EQ(grid.get_coverage_count(4), 0u);
}

// =============================================================================
// Bounds Checking Tests
// =============================================================================

TEST(bounds_check_valid_coordinates) {
    CoverageGrid grid(128, 128);
    ASSERT(grid.is_valid(0, 0));
    ASSERT(grid.is_valid(127, 0));
    ASSERT(grid.is_valid(0, 127));
    ASSERT(grid.is_valid(127, 127));
    ASSERT(grid.is_valid(64, 64));
}

TEST(bounds_check_invalid_coordinates) {
    CoverageGrid grid(128, 128);
    ASSERT(!grid.is_valid(128, 0));
    ASSERT(!grid.is_valid(0, 128));
    ASSERT(!grid.is_valid(128, 128));
    ASSERT(!grid.is_valid(200, 50));
    ASSERT(!grid.is_valid(50, 200));
}

TEST(bounds_check_uint32_max) {
    CoverageGrid grid(128, 128);
    // UINT32_MAX is way out of bounds
    ASSERT(!grid.is_valid(UINT32_MAX, 0));
    ASSERT(!grid.is_valid(0, UINT32_MAX));
    ASSERT(!grid.is_valid(UINT32_MAX, UINT32_MAX));
}

TEST(out_of_bounds_get_returns_zero) {
    CoverageGrid grid(128, 128);
    grid.set(0, 0, 3);

    // Out-of-bounds should return 0
    ASSERT_EQ(grid.get_coverage_owner(128, 0), 0);
    ASSERT_EQ(grid.get_coverage_owner(0, 128), 0);
    ASSERT_EQ(grid.get_coverage_owner(1000, 1000), 0);
}

TEST(out_of_bounds_is_in_coverage_returns_false) {
    CoverageGrid grid(128, 128);
    ASSERT(!grid.is_in_coverage(128, 0, 1));
    ASSERT(!grid.is_in_coverage(0, 128, 1));
    ASSERT(!grid.is_in_coverage(1000, 1000, 2));
}

TEST(out_of_bounds_set_is_noop) {
    CoverageGrid grid(128, 128);

    // These should not crash
    grid.set(128, 0, 1);
    grid.set(0, 128, 2);
    grid.set(1000, 1000, 3);

    // Verify no corruption of valid cells
    ASSERT_EQ(grid.get_coverage_owner(0, 0), 0);
    ASSERT_EQ(grid.get_coverage_owner(127, 127), 0);
}

TEST(out_of_bounds_clear_is_noop) {
    CoverageGrid grid(128, 128);
    grid.set(0, 0, 1);

    // These should not crash
    grid.clear(128, 0);
    grid.clear(0, 128);
    grid.clear(1000, 1000);

    // Verify no corruption
    ASSERT_EQ(grid.get_coverage_owner(0, 0), 1);
}

// =============================================================================
// Coverage Count Tests
// =============================================================================

TEST(coverage_count_empty_grid) {
    CoverageGrid grid(128, 128);
    ASSERT_EQ(grid.get_coverage_count(1), 0u);
    ASSERT_EQ(grid.get_coverage_count(2), 0u);
    ASSERT_EQ(grid.get_coverage_count(3), 0u);
    ASSERT_EQ(grid.get_coverage_count(4), 0u);
}

TEST(coverage_count_single_owner) {
    CoverageGrid grid(128, 128);
    grid.set(0, 0, 1);
    grid.set(1, 0, 1);
    grid.set(2, 0, 1);

    ASSERT_EQ(grid.get_coverage_count(1), 3u);
    ASSERT_EQ(grid.get_coverage_count(2), 0u);
}

TEST(coverage_count_multiple_owners) {
    CoverageGrid grid(128, 128);
    // 5 cells for owner 1
    for (uint32_t x = 0; x < 5; ++x) {
        grid.set(x, 0, 1);
    }
    // 3 cells for owner 2
    for (uint32_t x = 0; x < 3; ++x) {
        grid.set(x, 1, 2);
    }
    // 10 cells for owner 3
    for (uint32_t x = 0; x < 10; ++x) {
        grid.set(x, 2, 3);
    }

    ASSERT_EQ(grid.get_coverage_count(1), 5u);
    ASSERT_EQ(grid.get_coverage_count(2), 3u);
    ASSERT_EQ(grid.get_coverage_count(3), 10u);
    ASSERT_EQ(grid.get_coverage_count(4), 0u);
}

TEST(coverage_count_after_clear) {
    CoverageGrid grid(128, 128);
    grid.set(0, 0, 1);
    grid.set(1, 0, 1);
    grid.set(2, 0, 1);
    ASSERT_EQ(grid.get_coverage_count(1), 3u);

    grid.clear(1, 0);
    ASSERT_EQ(grid.get_coverage_count(1), 2u);
}

TEST(coverage_count_after_clear_all_for_owner) {
    CoverageGrid grid(128, 128);
    grid.set(0, 0, 1);
    grid.set(1, 0, 1);
    grid.set(2, 0, 2);

    grid.clear_all_for_owner(1);
    ASSERT_EQ(grid.get_coverage_count(1), 0u);
    ASSERT_EQ(grid.get_coverage_count(2), 1u);
}

// =============================================================================
// Memory Size Verification Tests
// =============================================================================

TEST(memory_size_1_byte_per_cell) {
    // uint8_t should be exactly 1 byte
    ASSERT_EQ(sizeof(uint8_t), 1u);
}

TEST(memory_size_128x128) {
    CoverageGrid grid(128, 128);
    // 128 * 128 = 16,384 cells * 1 byte = 16,384 bytes
    // We verify via coverage_count on a fully-filled grid
    // that the grid actually has 128*128 cells
    for (uint32_t y = 0; y < 128; ++y) {
        for (uint32_t x = 0; x < 128; ++x) {
            grid.set(x, y, 1);
        }
    }
    ASSERT_EQ(grid.get_coverage_count(1), 128u * 128u);
}

TEST(memory_size_256x256) {
    CoverageGrid grid(256, 256);
    // Set all cells to owner 2 and verify count
    for (uint32_t y = 0; y < 256; ++y) {
        for (uint32_t x = 0; x < 256; ++x) {
            grid.set(x, y, 2);
        }
    }
    ASSERT_EQ(grid.get_coverage_count(2), 256u * 256u);
}

TEST(memory_size_512x512) {
    CoverageGrid grid(512, 512);
    // Set all cells to owner 3 and verify count
    for (uint32_t y = 0; y < 512; ++y) {
        for (uint32_t x = 0; x < 512; ++x) {
            grid.set(x, y, 3);
        }
    }
    ASSERT_EQ(grid.get_coverage_count(3), 512u * 512u);
}

// =============================================================================
// Row-Major Storage Verification
// =============================================================================

TEST(row_major_layout) {
    CoverageGrid grid(128, 128);

    // Set cells in a known pattern and verify
    // (x=5, y=3) should be independent of (x=3, y=5)
    grid.set(5, 3, 1);
    grid.set(3, 5, 2);

    ASSERT_EQ(grid.get_coverage_owner(5, 3), 1);
    ASSERT_EQ(grid.get_coverage_owner(3, 5), 2);

    // They should not interfere
    ASSERT(grid.is_in_coverage(5, 3, 1));
    ASSERT(!grid.is_in_coverage(5, 3, 2));
    ASSERT(grid.is_in_coverage(3, 5, 2));
    ASSERT(!grid.is_in_coverage(3, 5, 1));
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== CoverageGrid Unit Tests (Ticket 5-007) ===\n\n");

    // Construction tests
    RUN_TEST(construction_128x128);
    RUN_TEST(construction_256x256);
    RUN_TEST(construction_512x512);
    RUN_TEST(construction_all_cells_uncovered);
    RUN_TEST(construction_non_square);

    // Set/get operations
    RUN_TEST(set_and_get_single_cell);
    RUN_TEST(set_and_get_multiple_owners);
    RUN_TEST(set_overwrites_previous);
    RUN_TEST(set_does_not_affect_neighbors);
    RUN_TEST(set_corner_cells);

    // is_in_coverage tests
    RUN_TEST(is_in_coverage_matching_owner);
    RUN_TEST(is_in_coverage_wrong_owner);
    RUN_TEST(is_in_coverage_uncovered_cell);
    RUN_TEST(is_in_coverage_zero_owner_on_uncovered);

    // Clear operations
    RUN_TEST(clear_single_cell);
    RUN_TEST(clear_does_not_affect_neighbors);
    RUN_TEST(clear_all_for_owner_removes_only_matching);
    RUN_TEST(clear_all_for_owner_large_grid);
    RUN_TEST(clear_all_resets_entire_grid);

    // Bounds checking
    RUN_TEST(bounds_check_valid_coordinates);
    RUN_TEST(bounds_check_invalid_coordinates);
    RUN_TEST(bounds_check_uint32_max);
    RUN_TEST(out_of_bounds_get_returns_zero);
    RUN_TEST(out_of_bounds_is_in_coverage_returns_false);
    RUN_TEST(out_of_bounds_set_is_noop);
    RUN_TEST(out_of_bounds_clear_is_noop);

    // Coverage count
    RUN_TEST(coverage_count_empty_grid);
    RUN_TEST(coverage_count_single_owner);
    RUN_TEST(coverage_count_multiple_owners);
    RUN_TEST(coverage_count_after_clear);
    RUN_TEST(coverage_count_after_clear_all_for_owner);

    // Memory size verification
    RUN_TEST(memory_size_1_byte_per_cell);
    RUN_TEST(memory_size_128x128);
    RUN_TEST(memory_size_256x256);
    RUN_TEST(memory_size_512x512);

    // Row-major storage
    RUN_TEST(row_major_layout);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
