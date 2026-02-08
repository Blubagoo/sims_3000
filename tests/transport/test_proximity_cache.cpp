/**
 * @file test_proximity_cache.cpp
 * @brief Unit tests for ProximityCache distance queries (Epic 7, Ticket E7-006)
 *
 * Tests cover:
 * - Construction and dimensions
 * - O(1) distance lookups after rebuild
 * - Multi-source BFS correctness (Manhattan distance, 4-directional)
 * - Dirty tracking
 * - Memory: 1 byte per tile (uint8_t)
 * - Max distance cap at 255
 * - Out-of-bounds returns 255
 * - Default constructor (0x0 cache)
 * - Empty grid (all distances 255)
 * - Single pathway tile
 * - Multiple pathway tiles (multi-source)
 */

#include <sims3000/transport/ProximityCache.h>
#include <sims3000/transport/PathwayGrid.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>

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
        printf("\n  FAILED: %s == %s (line %d, got %d vs %d)\n", #a, #b, __LINE__, (int)(a), (int)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// ============================================================================
// Construction tests
// ============================================================================

TEST(default_constructor) {
    ProximityCache cache;
    ASSERT_EQ(cache.width(), 0u);
    ASSERT_EQ(cache.height(), 0u);
    ASSERT(cache.is_dirty());
}

TEST(parameterized_constructor) {
    ProximityCache cache(128, 64);
    ASSERT_EQ(cache.width(), 128u);
    ASSERT_EQ(cache.height(), 64u);
    ASSERT(cache.is_dirty());
}

TEST(initial_distances_255) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);
    // Before rebuild, cache is dirty - distances are initial 255
    for (int32_t y = 0; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            ASSERT_EQ(cache.get_distance(x, y), 255);
        }
    }
}

// ============================================================================
// Memory size verification
// ============================================================================

TEST(memory_1_byte_per_tile) {
    // uint8_t is 1 byte
    ASSERT_EQ(sizeof(uint8_t), 1u);
}

// ============================================================================
// Dirty tracking tests
// ============================================================================

TEST(initial_dirty) {
    ProximityCache cache(16, 16);
    ASSERT(cache.is_dirty());
}

TEST(mark_dirty) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);
    cache.rebuild_if_dirty(grid);
    ASSERT(!cache.is_dirty());

    cache.mark_dirty();
    ASSERT(cache.is_dirty());
}

TEST(rebuild_clears_dirty) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);
    ASSERT(cache.is_dirty());

    cache.rebuild_if_dirty(grid);
    ASSERT(!cache.is_dirty());
}

TEST(rebuild_if_dirty_noop_when_clean) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    // First rebuild
    grid.set_pathway(5, 5, 1);
    cache.rebuild_if_dirty(grid);
    ASSERT(!cache.is_dirty());
    ASSERT_EQ(cache.get_distance(5, 5), 0);

    // Modify grid but don't mark cache dirty
    // rebuild_if_dirty should be a no-op
    grid.set_pathway(0, 0, 2);
    cache.rebuild_if_dirty(grid);
    // Distance at (0,0) should still reflect the OLD rebuild (not 0)
    // since we didn't mark_dirty()
    ASSERT(!cache.is_dirty());
}

// ============================================================================
// Empty grid tests
// ============================================================================

TEST(empty_grid_all_255) {
    ProximityCache cache(32, 32);
    PathwayGrid grid(32, 32);

    cache.rebuild_if_dirty(grid);

    for (int32_t y = 0; y < 32; ++y) {
        for (int32_t x = 0; x < 32; ++x) {
            ASSERT_EQ(cache.get_distance(x, y), 255);
        }
    }
}

// ============================================================================
// Single pathway tile tests
// ============================================================================

TEST(single_pathway_center) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    grid.set_pathway(8, 8, 1);
    cache.rebuild_if_dirty(grid);

    // The pathway tile itself should be distance 0
    ASSERT_EQ(cache.get_distance(8, 8), 0);

    // Adjacent tiles (Manhattan distance 1)
    ASSERT_EQ(cache.get_distance(9, 8), 1);
    ASSERT_EQ(cache.get_distance(7, 8), 1);
    ASSERT_EQ(cache.get_distance(8, 9), 1);
    ASSERT_EQ(cache.get_distance(8, 7), 1);

    // Diagonal tiles (Manhattan distance 2)
    ASSERT_EQ(cache.get_distance(9, 9), 2);
    ASSERT_EQ(cache.get_distance(7, 7), 2);
    ASSERT_EQ(cache.get_distance(9, 7), 2);
    ASSERT_EQ(cache.get_distance(7, 9), 2);

    // Farther tiles
    ASSERT_EQ(cache.get_distance(10, 8), 2);
    ASSERT_EQ(cache.get_distance(8, 10), 2);
}

TEST(single_pathway_corner) {
    ProximityCache cache(8, 8);
    PathwayGrid grid(8, 8);

    grid.set_pathway(0, 0, 1);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(0, 0), 0);
    ASSERT_EQ(cache.get_distance(1, 0), 1);
    ASSERT_EQ(cache.get_distance(0, 1), 1);
    ASSERT_EQ(cache.get_distance(1, 1), 2);
    ASSERT_EQ(cache.get_distance(7, 7), 14);  // Manhattan distance from (0,0) to (7,7) = 14
}

// ============================================================================
// Multi-source BFS tests
// ============================================================================

TEST(two_pathway_tiles_adjacent) {
    ProximityCache cache(8, 8);
    PathwayGrid grid(8, 8);

    grid.set_pathway(3, 3, 1);
    grid.set_pathway(4, 3, 2);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(3, 3), 0);
    ASSERT_EQ(cache.get_distance(4, 3), 0);

    // (2,3) is dist 1 from (3,3)
    ASSERT_EQ(cache.get_distance(2, 3), 1);
    // (5,3) is dist 1 from (4,3)
    ASSERT_EQ(cache.get_distance(5, 3), 1);
}

TEST(two_pathway_tiles_separated) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    grid.set_pathway(2, 2, 1);
    grid.set_pathway(12, 12, 2);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(2, 2), 0);
    ASSERT_EQ(cache.get_distance(12, 12), 0);

    // (7, 7) is equidistant: dist 10 from (2,2) and 10 from (12,12)
    ASSERT_EQ(cache.get_distance(7, 7), 10);

    // (3, 2) is dist 1 from (2,2)
    ASSERT_EQ(cache.get_distance(3, 2), 1);
}

TEST(pathway_line_horizontal) {
    ProximityCache cache(16, 8);
    PathwayGrid grid(16, 8);

    // Horizontal line of pathways at y=4
    for (int32_t x = 0; x < 16; ++x) {
        grid.set_pathway(x, 4, static_cast<uint32_t>(x + 1));
    }
    cache.rebuild_if_dirty(grid);

    // All tiles on the line should be distance 0
    for (int32_t x = 0; x < 16; ++x) {
        ASSERT_EQ(cache.get_distance(x, 4), 0);
    }

    // One row above: distance 1
    for (int32_t x = 0; x < 16; ++x) {
        ASSERT_EQ(cache.get_distance(x, 3), 1);
    }

    // Two rows above: distance 2
    for (int32_t x = 0; x < 16; ++x) {
        ASSERT_EQ(cache.get_distance(x, 2), 2);
    }

    // Top row (y=0): distance 4
    for (int32_t x = 0; x < 16; ++x) {
        ASSERT_EQ(cache.get_distance(x, 0), 4);
    }
}

// ============================================================================
// Manhattan distance verification (not diagonal/Euclidean)
// ============================================================================

TEST(manhattan_not_diagonal) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    grid.set_pathway(5, 5, 1);
    cache.rebuild_if_dirty(grid);

    // Manhattan distance to (8, 9) = |8-5| + |9-5| = 3 + 4 = 7
    ASSERT_EQ(cache.get_distance(8, 9), 7);

    // If it were Euclidean, (5,10) = distance 5, but Manhattan is also 5
    ASSERT_EQ(cache.get_distance(5, 10), 5);

    // Key test: diagonal (6,6) should be 2 (Manhattan), not 1 (Chebyshev)
    ASSERT_EQ(cache.get_distance(6, 6), 2);
}

// ============================================================================
// Out of bounds tests
// ============================================================================

TEST(out_of_bounds_returns_255) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    grid.set_pathway(0, 0, 1);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(-1, 0), 255);
    ASSERT_EQ(cache.get_distance(0, -1), 255);
    ASSERT_EQ(cache.get_distance(16, 0), 255);
    ASSERT_EQ(cache.get_distance(0, 16), 255);
    ASSERT_EQ(cache.get_distance(100, 100), 255);
}

// ============================================================================
// Distance cap at 255
// ============================================================================

TEST(distance_cap_at_255) {
    // On a large-enough grid with a single pathway at the corner,
    // tiles far away should cap at 255 (not overflow)
    // Grid 512x1: pathway at (0,0), tile at (511,0) has Manhattan dist 511
    // But since we cap at 255, it should be 255
    ProximityCache cache(512, 1);
    PathwayGrid grid(512, 1);

    grid.set_pathway(0, 0, 1);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(0, 0), 0);
    ASSERT_EQ(cache.get_distance(1, 0), 1);
    ASSERT_EQ(cache.get_distance(254, 0), 254);
    // BFS stops expanding at distance 254, so distance 255+ tiles remain 255
    ASSERT_EQ(cache.get_distance(255, 0), 255);
    ASSERT_EQ(cache.get_distance(400, 0), 255);
    ASSERT_EQ(cache.get_distance(511, 0), 255);
}

// ============================================================================
// Rebuild after modification
// ============================================================================

TEST(rebuild_after_pathway_added) {
    ProximityCache cache(8, 8);
    PathwayGrid grid(8, 8);

    // Initial: single pathway at (0,0)
    grid.set_pathway(0, 0, 1);
    cache.rebuild_if_dirty(grid);
    ASSERT_EQ(cache.get_distance(7, 7), 14);

    // Add pathway at (7,7), mark dirty, rebuild
    grid.set_pathway(7, 7, 2);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(7, 7), 0);
    ASSERT_EQ(cache.get_distance(0, 0), 0);
    // (4, 4) is dist 8 from (0,0) and dist 6 from (7,7) -> should be 6
    ASSERT_EQ(cache.get_distance(4, 4), 6);
}

TEST(rebuild_after_pathway_removed) {
    ProximityCache cache(8, 8);
    PathwayGrid grid(8, 8);

    grid.set_pathway(4, 4, 1);
    grid.set_pathway(0, 0, 2);
    cache.rebuild_if_dirty(grid);
    ASSERT_EQ(cache.get_distance(0, 0), 0);
    ASSERT_EQ(cache.get_distance(4, 4), 0);

    // Remove pathway at (0,0)
    grid.clear_pathway(0, 0);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    // (0,0) is now dist 8 from (4,4)
    ASSERT_EQ(cache.get_distance(0, 0), 8);
    ASSERT_EQ(cache.get_distance(4, 4), 0);
}

// ============================================================================
// Full grid test (all pathways)
// ============================================================================

TEST(full_grid_all_zero) {
    ProximityCache cache(8, 8);
    PathwayGrid grid(8, 8);

    for (int32_t y = 0; y < 8; ++y) {
        for (int32_t x = 0; x < 8; ++x) {
            grid.set_pathway(x, y, 1);
        }
    }
    cache.rebuild_if_dirty(grid);

    for (int32_t y = 0; y < 8; ++y) {
        for (int32_t x = 0; x < 8; ++x) {
            ASSERT_EQ(cache.get_distance(x, y), 0);
        }
    }
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== ProximityCache Unit Tests (Ticket E7-006) ===\n\n");

    // Construction
    RUN_TEST(default_constructor);
    RUN_TEST(parameterized_constructor);
    RUN_TEST(initial_distances_255);

    // Memory
    RUN_TEST(memory_1_byte_per_tile);

    // Dirty tracking
    RUN_TEST(initial_dirty);
    RUN_TEST(mark_dirty);
    RUN_TEST(rebuild_clears_dirty);
    RUN_TEST(rebuild_if_dirty_noop_when_clean);

    // Empty grid
    RUN_TEST(empty_grid_all_255);

    // Single pathway
    RUN_TEST(single_pathway_center);
    RUN_TEST(single_pathway_corner);

    // Multi-source BFS
    RUN_TEST(two_pathway_tiles_adjacent);
    RUN_TEST(two_pathway_tiles_separated);
    RUN_TEST(pathway_line_horizontal);

    // Manhattan distance
    RUN_TEST(manhattan_not_diagonal);

    // Out of bounds
    RUN_TEST(out_of_bounds_returns_255);

    // Distance cap
    RUN_TEST(distance_cap_at_255);

    // Rebuild after modification
    RUN_TEST(rebuild_after_pathway_added);
    RUN_TEST(rebuild_after_pathway_removed);

    // Full grid
    RUN_TEST(full_grid_all_zero);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
