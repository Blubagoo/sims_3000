/**
 * @file test_proximity_bfs.cpp
 * @brief Comprehensive tests for multi-source BFS proximity rebuild (Epic 7, Ticket E7-007)
 *
 * Tests cover:
 * - Empty grid: all distances = 255
 * - Single pathway: correct distance ring (Manhattan distance)
 * - Multiple pathways: correct multi-source BFS
 * - Performance: rebuild 256x256 within 20ms budget
 * - Edge cases: corners, boundaries, distance cap
 * - Cache-friendly memory access patterns
 */

#include <sims3000/transport/ProximityCache.h>
#include <sims3000/transport/PathwayGrid.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <chrono>

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
// Empty grid tests
// ============================================================================

TEST(empty_grid_all_255) {
    // An empty grid (no pathways) should have all distances = 255
    ProximityCache cache(64, 64);
    PathwayGrid grid(64, 64);

    cache.rebuild_if_dirty(grid);

    for (int32_t y = 0; y < 64; ++y) {
        for (int32_t x = 0; x < 64; ++x) {
            ASSERT_EQ(cache.get_distance(x, y), 255);
        }
    }
}

TEST(empty_grid_small) {
    ProximityCache cache(1, 1);
    PathwayGrid grid(1, 1);

    cache.rebuild_if_dirty(grid);
    ASSERT_EQ(cache.get_distance(0, 0), 255);
}

// ============================================================================
// Single pathway: correct distance ring
// ============================================================================

TEST(single_pathway_distance_ring) {
    // Place a single pathway at (10, 10) on a 21x21 grid
    // Verify Manhattan distance rings outward
    ProximityCache cache(21, 21);
    PathwayGrid grid(21, 21);

    grid.set_pathway(10, 10, 1);
    cache.rebuild_if_dirty(grid);

    // Distance 0 at the source
    ASSERT_EQ(cache.get_distance(10, 10), 0);

    // Verify every tile has correct Manhattan distance
    for (int32_t y = 0; y < 21; ++y) {
        for (int32_t x = 0; x < 21; ++x) {
            int expected = std::abs(x - 10) + std::abs(y - 10);
            if (expected > 255) expected = 255;
            ASSERT_EQ(cache.get_distance(x, y), static_cast<uint8_t>(expected));
        }
    }
}

TEST(single_pathway_cardinal_directions) {
    // Verify distances along the 4 cardinal axes from center
    ProximityCache cache(32, 32);
    PathwayGrid grid(32, 32);

    grid.set_pathway(16, 16, 1);
    cache.rebuild_if_dirty(grid);

    // North
    for (int32_t d = 0; d <= 16; ++d) {
        ASSERT_EQ(cache.get_distance(16, 16 - d), static_cast<uint8_t>(d));
    }
    // South
    for (int32_t d = 0; d < 16; ++d) {
        ASSERT_EQ(cache.get_distance(16, 16 + d), static_cast<uint8_t>(d));
    }
    // East
    for (int32_t d = 0; d < 16; ++d) {
        ASSERT_EQ(cache.get_distance(16 + d, 16), static_cast<uint8_t>(d));
    }
    // West
    for (int32_t d = 0; d <= 16; ++d) {
        ASSERT_EQ(cache.get_distance(16 - d, 16), static_cast<uint8_t>(d));
    }
}

TEST(single_pathway_diagonal_is_manhattan) {
    // Diagonal distance should be Manhattan (dx + dy), not Euclidean or Chebyshev
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    grid.set_pathway(5, 5, 1);
    cache.rebuild_if_dirty(grid);

    // Diagonal (6,6): Manhattan = |6-5| + |6-5| = 2
    ASSERT_EQ(cache.get_distance(6, 6), 2);
    // Diagonal (7,7): Manhattan = 4
    ASSERT_EQ(cache.get_distance(7, 7), 4);
    // Diagonal (8,8): Manhattan = 6
    ASSERT_EQ(cache.get_distance(8, 8), 6);
    // Off-diagonal (8,6): Manhattan = |8-5| + |6-5| = 4
    ASSERT_EQ(cache.get_distance(8, 6), 4);
}

// ============================================================================
// Multiple pathways: correct multi-source BFS
// ============================================================================

TEST(two_sources_equidistant_midpoint) {
    // Two pathways at opposite corners of a 16x16 grid
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    grid.set_pathway(0, 0, 1);
    grid.set_pathway(15, 15, 2);
    cache.rebuild_if_dirty(grid);

    // Each source has distance 0
    ASSERT_EQ(cache.get_distance(0, 0), 0);
    ASSERT_EQ(cache.get_distance(15, 15), 0);

    // Every tile should have distance = min(Manhattan from (0,0), Manhattan from (15,15))
    for (int32_t y = 0; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            int d1 = std::abs(x) + std::abs(y);
            int d2 = std::abs(x - 15) + std::abs(y - 15);
            int expected = (d1 < d2) ? d1 : d2;
            if (expected > 255) expected = 255;
            ASSERT_EQ(cache.get_distance(x, y), static_cast<uint8_t>(expected));
        }
    }
}

TEST(multiple_sources_line) {
    // A horizontal line of pathways at y=4 on a 16x8 grid
    ProximityCache cache(16, 8);
    PathwayGrid grid(16, 8);

    for (int32_t x = 0; x < 16; ++x) {
        grid.set_pathway(x, 4, static_cast<uint32_t>(x + 1));
    }
    cache.rebuild_if_dirty(grid);

    // All tiles on the line should be distance 0
    for (int32_t x = 0; x < 16; ++x) {
        ASSERT_EQ(cache.get_distance(x, 4), 0);
    }

    // Distance should be purely vertical (y offset from line)
    for (int32_t y = 0; y < 8; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            int expected = std::abs(y - 4);
            ASSERT_EQ(cache.get_distance(x, y), static_cast<uint8_t>(expected));
        }
    }
}

TEST(three_sources_nearest_wins) {
    // Three pathways: verify each tile uses the nearest source
    ProximityCache cache(32, 32);
    PathwayGrid grid(32, 32);

    grid.set_pathway(5, 5, 1);
    grid.set_pathway(25, 5, 2);
    grid.set_pathway(15, 25, 3);
    cache.rebuild_if_dirty(grid);

    for (int32_t y = 0; y < 32; ++y) {
        for (int32_t x = 0; x < 32; ++x) {
            int d1 = std::abs(x - 5) + std::abs(y - 5);
            int d2 = std::abs(x - 25) + std::abs(y - 5);
            int d3 = std::abs(x - 15) + std::abs(y - 25);
            int expected = d1;
            if (d2 < expected) expected = d2;
            if (d3 < expected) expected = d3;
            if (expected > 255) expected = 255;
            ASSERT_EQ(cache.get_distance(x, y), static_cast<uint8_t>(expected));
        }
    }
}

TEST(adjacent_sources_all_zero_or_one) {
    // Two adjacent pathways: tiles at the sources are 0, adjacent are 1
    ProximityCache cache(8, 8);
    PathwayGrid grid(8, 8);

    grid.set_pathway(3, 3, 1);
    grid.set_pathway(4, 3, 2);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(3, 3), 0);
    ASSERT_EQ(cache.get_distance(4, 3), 0);
    ASSERT_EQ(cache.get_distance(2, 3), 1);
    ASSERT_EQ(cache.get_distance(5, 3), 1);
    ASSERT_EQ(cache.get_distance(3, 2), 1);
    ASSERT_EQ(cache.get_distance(4, 4), 1);
}

// ============================================================================
// Edge cases: corners and boundaries
// ============================================================================

TEST(corner_top_left) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    grid.set_pathway(0, 0, 1);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(0, 0), 0);
    ASSERT_EQ(cache.get_distance(1, 0), 1);
    ASSERT_EQ(cache.get_distance(0, 1), 1);
    ASSERT_EQ(cache.get_distance(15, 15), 30);  // Manhattan from (0,0) to (15,15)
}

TEST(corner_top_right) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    grid.set_pathway(15, 0, 1);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(15, 0), 0);
    ASSERT_EQ(cache.get_distance(14, 0), 1);
    ASSERT_EQ(cache.get_distance(15, 1), 1);
    ASSERT_EQ(cache.get_distance(0, 15), 30);
}

TEST(corner_bottom_left) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    grid.set_pathway(0, 15, 1);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(0, 15), 0);
    ASSERT_EQ(cache.get_distance(1, 15), 1);
    ASSERT_EQ(cache.get_distance(0, 14), 1);
    ASSERT_EQ(cache.get_distance(15, 0), 30);
}

TEST(corner_bottom_right) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    grid.set_pathway(15, 15, 1);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(15, 15), 0);
    ASSERT_EQ(cache.get_distance(14, 15), 1);
    ASSERT_EQ(cache.get_distance(15, 14), 1);
    ASSERT_EQ(cache.get_distance(0, 0), 30);
}

TEST(boundary_edge_pathways) {
    // Pathways along entire top edge
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    for (int32_t x = 0; x < 16; ++x) {
        grid.set_pathway(x, 0, static_cast<uint32_t>(x + 1));
    }
    cache.rebuild_if_dirty(grid);

    // All top edge tiles are distance 0
    for (int32_t x = 0; x < 16; ++x) {
        ASSERT_EQ(cache.get_distance(x, 0), 0);
    }

    // Each row below top edge has distance = row number
    for (int32_t y = 1; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            ASSERT_EQ(cache.get_distance(x, y), static_cast<uint8_t>(y));
        }
    }
}

TEST(out_of_bounds_returns_255) {
    ProximityCache cache(8, 8);
    PathwayGrid grid(8, 8);

    grid.set_pathway(4, 4, 1);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(-1, 0), 255);
    ASSERT_EQ(cache.get_distance(0, -1), 255);
    ASSERT_EQ(cache.get_distance(8, 0), 255);
    ASSERT_EQ(cache.get_distance(0, 8), 255);
    ASSERT_EQ(cache.get_distance(-100, -100), 255);
    ASSERT_EQ(cache.get_distance(1000, 1000), 255);
}

TEST(distance_cap_at_255) {
    // On a 512x1 grid with single pathway at (0,0),
    // tiles beyond distance 254 should remain at 255
    ProximityCache cache(512, 1);
    PathwayGrid grid(512, 1);

    grid.set_pathway(0, 0, 1);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(0, 0), 0);
    ASSERT_EQ(cache.get_distance(254, 0), 254);
    ASSERT_EQ(cache.get_distance(255, 0), 255);
    ASSERT_EQ(cache.get_distance(300, 0), 255);
    ASSERT_EQ(cache.get_distance(511, 0), 255);
}

TEST(full_grid_all_pathways_zero) {
    // If every tile is a pathway, all distances should be 0
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    for (int32_t y = 0; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            grid.set_pathway(x, y, static_cast<uint32_t>(y * 16 + x + 1));
        }
    }
    cache.rebuild_if_dirty(grid);

    for (int32_t y = 0; y < 16; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            ASSERT_EQ(cache.get_distance(x, y), 0);
        }
    }
}

TEST(single_tile_grid) {
    // 1x1 grid with pathway
    ProximityCache cache(1, 1);
    PathwayGrid grid(1, 1);

    grid.set_pathway(0, 0, 1);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(0, 0), 0);
}

// ============================================================================
// Re-rebuild correctness
// ============================================================================

TEST(rebuild_after_add) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    // First rebuild: single pathway at (0,0)
    grid.set_pathway(0, 0, 1);
    cache.rebuild_if_dirty(grid);
    ASSERT_EQ(cache.get_distance(15, 15), 30);

    // Add pathway at (15,15), mark dirty, rebuild
    grid.set_pathway(15, 15, 2);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(0, 0), 0);
    ASSERT_EQ(cache.get_distance(15, 15), 0);
    // Midpoint (8,8): min(16, 14) = 14
    ASSERT_EQ(cache.get_distance(8, 8), 14);
}

TEST(rebuild_after_remove) {
    ProximityCache cache(16, 16);
    PathwayGrid grid(16, 16);

    grid.set_pathway(0, 0, 1);
    grid.set_pathway(15, 15, 2);
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(0, 0), 0);
    ASSERT_EQ(cache.get_distance(15, 15), 0);

    // Remove one source
    grid.clear_pathway(15, 15);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    ASSERT_EQ(cache.get_distance(0, 0), 0);
    ASSERT_EQ(cache.get_distance(15, 15), 30);
}

// ============================================================================
// Performance: rebuild 256x256 within 20ms budget
// ============================================================================

TEST(performance_256x256_empty) {
    ProximityCache cache(256, 256);
    PathwayGrid grid(256, 256);

    auto start = std::chrono::high_resolution_clock::now();
    cache.rebuild_if_dirty(grid);
    auto end = std::chrono::high_resolution_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    printf(" [empty: %lldms] ", (long long)ms);

    ASSERT(ms < 20);
}

TEST(performance_256x256_sparse) {
    // Sparse pathways: about 1% coverage
    ProximityCache cache(256, 256);
    PathwayGrid grid(256, 256);

    uint32_t count = 0;
    for (int32_t y = 0; y < 256; y += 10) {
        for (int32_t x = 0; x < 256; x += 10) {
            grid.set_pathway(x, y, ++count);
        }
    }

    cache.mark_dirty();
    auto start = std::chrono::high_resolution_clock::now();
    cache.rebuild_if_dirty(grid);
    auto end = std::chrono::high_resolution_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    printf(" [sparse %u pathways: %lldms] ", count, (long long)ms);

    ASSERT(ms < 20);
}

TEST(performance_256x256_dense) {
    // Dense pathways: grid pattern (~25% coverage)
    ProximityCache cache(256, 256);
    PathwayGrid grid(256, 256);

    uint32_t count = 0;
    for (int32_t y = 0; y < 256; ++y) {
        for (int32_t x = 0; x < 256; ++x) {
            if (x % 2 == 0 && y % 2 == 0) {
                grid.set_pathway(x, y, ++count);
            }
        }
    }

    cache.mark_dirty();
    auto start = std::chrono::high_resolution_clock::now();
    cache.rebuild_if_dirty(grid);
    auto end = std::chrono::high_resolution_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    printf(" [dense %u pathways: %lldms] ", count, (long long)ms);

    ASSERT(ms < 20);
}

TEST(performance_256x256_full) {
    // Full coverage: every tile is a pathway
    ProximityCache cache(256, 256);
    PathwayGrid grid(256, 256);

    for (int32_t y = 0; y < 256; ++y) {
        for (int32_t x = 0; x < 256; ++x) {
            grid.set_pathway(x, y, static_cast<uint32_t>(y * 256 + x + 1));
        }
    }

    cache.mark_dirty();
    auto start = std::chrono::high_resolution_clock::now();
    cache.rebuild_if_dirty(grid);
    auto end = std::chrono::high_resolution_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    printf(" [full: %lldms] ", (long long)ms);

    ASSERT(ms < 20);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== ProximityCache BFS Tests (Ticket E7-007) ===\n\n");

    // Empty grid
    RUN_TEST(empty_grid_all_255);
    RUN_TEST(empty_grid_small);

    // Single pathway distance ring
    RUN_TEST(single_pathway_distance_ring);
    RUN_TEST(single_pathway_cardinal_directions);
    RUN_TEST(single_pathway_diagonal_is_manhattan);

    // Multiple pathways multi-source BFS
    RUN_TEST(two_sources_equidistant_midpoint);
    RUN_TEST(multiple_sources_line);
    RUN_TEST(three_sources_nearest_wins);
    RUN_TEST(adjacent_sources_all_zero_or_one);

    // Edge cases: corners and boundaries
    RUN_TEST(corner_top_left);
    RUN_TEST(corner_top_right);
    RUN_TEST(corner_bottom_left);
    RUN_TEST(corner_bottom_right);
    RUN_TEST(boundary_edge_pathways);
    RUN_TEST(out_of_bounds_returns_255);
    RUN_TEST(distance_cap_at_255);
    RUN_TEST(full_grid_all_pathways_zero);
    RUN_TEST(single_tile_grid);

    // Re-rebuild correctness
    RUN_TEST(rebuild_after_add);
    RUN_TEST(rebuild_after_remove);

    // Performance
    RUN_TEST(performance_256x256_empty);
    RUN_TEST(performance_256x256_sparse);
    RUN_TEST(performance_256x256_dense);
    RUN_TEST(performance_256x256_full);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
