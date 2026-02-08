/**
 * @file test_pathway_grid.cpp
 * @brief Unit tests for PathwayGrid dense 2D array (Epic 7, Ticket E7-005)
 *
 * Tests cover:
 * - Construction and dimensions
 * - O(1) spatial queries (get_pathway_at, has_pathway)
 * - set_pathway / clear_pathway operations
 * - Bounds checking (out-of-bounds returns safe defaults)
 * - Dirty flag for network rebuild
 * - Memory: 4 bytes per cell (sizeof(PathwayGridCell))
 * - Default constructor (0x0 grid)
 * - Negative coordinate handling
 */

#include <sims3000/transport/PathwayGrid.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::transport;

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

// ============================================================================
// Cell size verification
// ============================================================================

TEST(cell_size_is_4_bytes) {
    ASSERT_EQ(sizeof(PathwayGridCell), 4u);
}

// ============================================================================
// Construction tests
// ============================================================================

TEST(default_constructor) {
    PathwayGrid grid;
    ASSERT_EQ(grid.width(), 0u);
    ASSERT_EQ(grid.height(), 0u);
}

TEST(parameterized_constructor) {
    PathwayGrid grid(128, 64);
    ASSERT_EQ(grid.width(), 128u);
    ASSERT_EQ(grid.height(), 64u);
}

TEST(initial_cells_empty) {
    PathwayGrid grid(16, 16);
    for (int32_t y = 0; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            ASSERT_EQ(grid.get_pathway_at(x, y), 0u);
            ASSERT(!grid.has_pathway(x, y));
        }
    }
}

// ============================================================================
// Core operation tests
// ============================================================================

TEST(set_and_get_pathway) {
    PathwayGrid grid(32, 32);

    grid.set_pathway(5, 10, 42);
    ASSERT_EQ(grid.get_pathway_at(5, 10), 42u);
    ASSERT(grid.has_pathway(5, 10));
}

TEST(clear_pathway) {
    PathwayGrid grid(32, 32);

    grid.set_pathway(5, 10, 42);
    ASSERT(grid.has_pathway(5, 10));

    grid.clear_pathway(5, 10);
    ASSERT_EQ(grid.get_pathway_at(5, 10), 0u);
    ASSERT(!grid.has_pathway(5, 10));
}

TEST(overwrite_pathway) {
    PathwayGrid grid(32, 32);

    grid.set_pathway(5, 10, 42);
    ASSERT_EQ(grid.get_pathway_at(5, 10), 42u);

    grid.set_pathway(5, 10, 99);
    ASSERT_EQ(grid.get_pathway_at(5, 10), 99u);
}

TEST(multiple_pathways) {
    PathwayGrid grid(32, 32);

    grid.set_pathway(0, 0, 1);
    grid.set_pathway(31, 31, 2);
    grid.set_pathway(15, 15, 3);

    ASSERT_EQ(grid.get_pathway_at(0, 0), 1u);
    ASSERT_EQ(grid.get_pathway_at(31, 31), 2u);
    ASSERT_EQ(grid.get_pathway_at(15, 15), 3u);

    // Unset cells remain empty
    ASSERT_EQ(grid.get_pathway_at(1, 0), 0u);
    ASSERT(!grid.has_pathway(1, 0));
}

TEST(corner_cells) {
    PathwayGrid grid(64, 64);

    // All four corners
    grid.set_pathway(0, 0, 10);
    grid.set_pathway(63, 0, 20);
    grid.set_pathway(0, 63, 30);
    grid.set_pathway(63, 63, 40);

    ASSERT_EQ(grid.get_pathway_at(0, 0), 10u);
    ASSERT_EQ(grid.get_pathway_at(63, 0), 20u);
    ASSERT_EQ(grid.get_pathway_at(0, 63), 30u);
    ASSERT_EQ(grid.get_pathway_at(63, 63), 40u);
}

TEST(max_entity_id) {
    PathwayGrid grid(8, 8);

    grid.set_pathway(0, 0, UINT32_MAX);
    ASSERT_EQ(grid.get_pathway_at(0, 0), UINT32_MAX);
    ASSERT(grid.has_pathway(0, 0));
}

// ============================================================================
// Bounds checking tests
// ============================================================================

TEST(in_bounds) {
    PathwayGrid grid(32, 32);

    ASSERT(grid.in_bounds(0, 0));
    ASSERT(grid.in_bounds(31, 31));
    ASSERT(grid.in_bounds(15, 15));

    ASSERT(!grid.in_bounds(-1, 0));
    ASSERT(!grid.in_bounds(0, -1));
    ASSERT(!grid.in_bounds(32, 0));
    ASSERT(!grid.in_bounds(0, 32));
    ASSERT(!grid.in_bounds(-1, -1));
    ASSERT(!grid.in_bounds(32, 32));
}

TEST(out_of_bounds_get_returns_zero) {
    PathwayGrid grid(16, 16);

    ASSERT_EQ(grid.get_pathway_at(-1, 0), 0u);
    ASSERT_EQ(grid.get_pathway_at(0, -1), 0u);
    ASSERT_EQ(grid.get_pathway_at(16, 0), 0u);
    ASSERT_EQ(grid.get_pathway_at(0, 16), 0u);
    ASSERT_EQ(grid.get_pathway_at(100, 100), 0u);
}

TEST(out_of_bounds_has_returns_false) {
    PathwayGrid grid(16, 16);

    ASSERT(!grid.has_pathway(-1, 0));
    ASSERT(!grid.has_pathway(0, -1));
    ASSERT(!grid.has_pathway(16, 0));
    ASSERT(!grid.has_pathway(0, 16));
}

TEST(out_of_bounds_set_is_noop) {
    PathwayGrid grid(16, 16);

    // Should not crash
    grid.set_pathway(-1, 0, 42);
    grid.set_pathway(0, -1, 42);
    grid.set_pathway(16, 0, 42);
    grid.set_pathway(0, 16, 42);

    // Grid remains unmodified
    for (int32_t y = 0; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            ASSERT_EQ(grid.get_pathway_at(x, y), 0u);
        }
    }
}

TEST(out_of_bounds_clear_is_noop) {
    PathwayGrid grid(16, 16);

    // Should not crash
    grid.clear_pathway(-1, 0);
    grid.clear_pathway(0, -1);
    grid.clear_pathway(16, 0);
    grid.clear_pathway(0, 16);
}

// ============================================================================
// Dirty flag tests
// ============================================================================

TEST(initial_dirty_flag) {
    PathwayGrid grid(16, 16);
    ASSERT(grid.is_network_dirty());
}

TEST(set_pathway_marks_dirty) {
    PathwayGrid grid(16, 16);

    grid.mark_network_clean();
    ASSERT(!grid.is_network_dirty());

    grid.set_pathway(5, 5, 42);
    ASSERT(grid.is_network_dirty());
}

TEST(clear_pathway_marks_dirty) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 42);

    grid.mark_network_clean();
    ASSERT(!grid.is_network_dirty());

    grid.clear_pathway(5, 5);
    ASSERT(grid.is_network_dirty());
}

TEST(mark_network_clean) {
    PathwayGrid grid(16, 16);
    ASSERT(grid.is_network_dirty());

    grid.mark_network_clean();
    ASSERT(!grid.is_network_dirty());
}

TEST(mark_network_dirty_manual) {
    PathwayGrid grid(16, 16);

    grid.mark_network_clean();
    ASSERT(!grid.is_network_dirty());

    grid.mark_network_dirty();
    ASSERT(grid.is_network_dirty());
}

TEST(default_constructor_dirty) {
    PathwayGrid grid;
    ASSERT(grid.is_network_dirty());
}

// ============================================================================
// Row-major layout verification
// ============================================================================

TEST(row_major_layout) {
    // Verify that (x=1,y=0) and (x=0,y=1) map to different cells
    PathwayGrid grid(8, 8);

    grid.set_pathway(1, 0, 10);
    grid.set_pathway(0, 1, 20);

    ASSERT_EQ(grid.get_pathway_at(1, 0), 10u);
    ASSERT_EQ(grid.get_pathway_at(0, 1), 20u);
}

// ============================================================================
// Large grid test
// ============================================================================

TEST(large_grid_128x128) {
    PathwayGrid grid(128, 128);
    ASSERT_EQ(grid.width(), 128u);
    ASSERT_EQ(grid.height(), 128u);

    // Set a few pathways
    grid.set_pathway(0, 0, 1);
    grid.set_pathway(127, 127, 2);
    grid.set_pathway(64, 64, 3);

    ASSERT_EQ(grid.get_pathway_at(0, 0), 1u);
    ASSERT_EQ(grid.get_pathway_at(127, 127), 2u);
    ASSERT_EQ(grid.get_pathway_at(64, 64), 3u);
    ASSERT_EQ(grid.get_pathway_at(1, 1), 0u);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== PathwayGrid Unit Tests (Ticket E7-005) ===\n\n");

    // Cell size
    RUN_TEST(cell_size_is_4_bytes);

    // Construction
    RUN_TEST(default_constructor);
    RUN_TEST(parameterized_constructor);
    RUN_TEST(initial_cells_empty);

    // Core operations
    RUN_TEST(set_and_get_pathway);
    RUN_TEST(clear_pathway);
    RUN_TEST(overwrite_pathway);
    RUN_TEST(multiple_pathways);
    RUN_TEST(corner_cells);
    RUN_TEST(max_entity_id);

    // Bounds checking
    RUN_TEST(in_bounds);
    RUN_TEST(out_of_bounds_get_returns_zero);
    RUN_TEST(out_of_bounds_has_returns_false);
    RUN_TEST(out_of_bounds_set_is_noop);
    RUN_TEST(out_of_bounds_clear_is_noop);

    // Dirty flag
    RUN_TEST(initial_dirty_flag);
    RUN_TEST(set_pathway_marks_dirty);
    RUN_TEST(clear_pathway_marks_dirty);
    RUN_TEST(mark_network_clean);
    RUN_TEST(mark_network_dirty_manual);
    RUN_TEST(default_constructor_dirty);

    // Layout
    RUN_TEST(row_major_layout);

    // Large grid
    RUN_TEST(large_grid_128x128);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
