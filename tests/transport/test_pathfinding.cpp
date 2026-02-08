/**
 * @file test_pathfinding.cpp
 * @brief Unit tests for A* Pathfinding (Epic 7, Ticket E7-023)
 *
 * Tests:
 * - Start == end (trivial path)
 * - Straight line path
 * - Path around obstacle
 * - No path (disconnected components)
 * - Start/end not on pathway
 * - Path cost calculation
 * - Early exit via network_id check
 * - Larger grid path
 */

#include <sims3000/transport/Pathfinding.h>
#include <sims3000/transport/PathwayGrid.h>
#include <sims3000/transport/NetworkGraph.h>
#include <cassert>
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
        printf("\n  FAILED: %s == %s (line %d, got %lld vs %lld)\n", \
               #a, #b, __LINE__, (long long)(a), (long long)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// ============================================================================
// Start == end (trivial path)
// ============================================================================

TEST(trivial_same_position) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    Pathfinding pf;
    PathResult result = pf.find_path({5, 5}, {5, 5}, grid, graph);

    ASSERT(result.found);
    ASSERT_EQ(result.path.size(), 1u);
    ASSERT_EQ(result.path[0].x, 5);
    ASSERT_EQ(result.path[0].y, 5);
    ASSERT_EQ(result.total_cost, 0u);
}

// ============================================================================
// Straight line path
// ============================================================================

TEST(straight_line_path) {
    PathwayGrid grid(16, 16);
    // Horizontal line: (2,5) through (7,5)
    for (int x = 2; x <= 7; ++x) {
        grid.set_pathway(x, 5, 1);
    }

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    Pathfinding pf;
    PathResult result = pf.find_path({2, 5}, {7, 5}, grid, graph);

    ASSERT(result.found);
    // Path should have 6 positions (2,5) to (7,5) inclusive
    ASSERT_EQ(result.path.size(), 6u);
    // Check start and end
    ASSERT_EQ(result.path.front().x, 2);
    ASSERT_EQ(result.path.front().y, 5);
    ASSERT_EQ(result.path.back().x, 7);
    ASSERT_EQ(result.path.back().y, 5);
    // Cost = 5 steps * 10 = 50
    ASSERT_EQ(result.total_cost, 50u);
}

// ============================================================================
// Path around obstacle
// ============================================================================

TEST(path_around_obstacle) {
    PathwayGrid grid(16, 16);
    // Create an L-shaped path (no direct route)
    // Horizontal: (2,5) - (3,5) - (4,5)
    // Vertical:   (4,5) - (4,6) - (4,7)
    // Horizontal: (4,7) - (5,7) - (6,7)
    grid.set_pathway(2, 5, 1);
    grid.set_pathway(3, 5, 1);
    grid.set_pathway(4, 5, 1);
    grid.set_pathway(4, 6, 1);
    grid.set_pathway(4, 7, 1);
    grid.set_pathway(5, 7, 1);
    grid.set_pathway(6, 7, 1);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    Pathfinding pf;
    PathResult result = pf.find_path({2, 5}, {6, 7}, grid, graph);

    ASSERT(result.found);
    // Shortest path: (2,5)->(3,5)->(4,5)->(4,6)->(4,7)->(5,7)->(6,7) = 7 tiles, 6 steps
    ASSERT_EQ(result.path.size(), 7u);
    ASSERT_EQ(result.total_cost, 60u);
    ASSERT_EQ(result.path.front().x, 2);
    ASSERT_EQ(result.path.front().y, 5);
    ASSERT_EQ(result.path.back().x, 6);
    ASSERT_EQ(result.path.back().y, 7);
}

// ============================================================================
// No path - disconnected components
// ============================================================================

TEST(no_path_disconnected) {
    PathwayGrid grid(32, 32);
    // Segment A: (2,2) - (3,2)
    grid.set_pathway(2, 2, 1);
    grid.set_pathway(3, 2, 1);
    // Segment B: (20,20) - (21,20)
    grid.set_pathway(20, 20, 2);
    grid.set_pathway(21, 20, 2);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    Pathfinding pf;
    PathResult result = pf.find_path({2, 2}, {20, 20}, grid, graph);

    ASSERT(!result.found);
    ASSERT(result.path.empty());
    ASSERT_EQ(result.total_cost, 0u);
}

// ============================================================================
// Start not on pathway
// ============================================================================

TEST(start_not_on_pathway) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);
    grid.set_pathway(6, 5, 1);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    Pathfinding pf;
    // (10,10) has no pathway
    PathResult result = pf.find_path({10, 10}, {5, 5}, grid, graph);

    ASSERT(!result.found);
}

// ============================================================================
// End not on pathway
// ============================================================================

TEST(end_not_on_pathway) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);
    grid.set_pathway(6, 5, 1);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    Pathfinding pf;
    PathResult result = pf.find_path({5, 5}, {10, 10}, grid, graph);

    ASSERT(!result.found);
}

// ============================================================================
// Adjacent tiles
// ============================================================================

TEST(adjacent_tiles) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);
    grid.set_pathway(6, 5, 1);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    Pathfinding pf;
    PathResult result = pf.find_path({5, 5}, {6, 5}, grid, graph);

    ASSERT(result.found);
    ASSERT_EQ(result.path.size(), 2u);
    ASSERT_EQ(result.total_cost, 10u);
}

// ============================================================================
// Path with multiple routes (shortest wins)
// ============================================================================

TEST(shortest_route_chosen) {
    PathwayGrid grid(16, 16);
    // Two routes from (0,0) to (2,0):
    // Direct: (0,0) -> (1,0) -> (2,0)   = 2 steps
    // Detour: (0,0) -> (0,1) -> (1,1) -> (2,1) -> (2,0) = 4 steps
    grid.set_pathway(0, 0, 1);
    grid.set_pathway(1, 0, 1);
    grid.set_pathway(2, 0, 1);
    grid.set_pathway(0, 1, 1);
    grid.set_pathway(1, 1, 1);
    grid.set_pathway(2, 1, 1);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    Pathfinding pf;
    PathResult result = pf.find_path({0, 0}, {2, 0}, grid, graph);

    ASSERT(result.found);
    // Direct route: 2 steps * 10 = 20
    ASSERT_EQ(result.total_cost, 20u);
    ASSERT_EQ(result.path.size(), 3u);
}

// ============================================================================
// Path on larger grid
// ============================================================================

TEST(larger_grid_path) {
    PathwayGrid grid(64, 64);
    // Build a long horizontal pathway
    for (int x = 0; x < 50; ++x) {
        grid.set_pathway(x, 10, 1);
    }

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    Pathfinding pf;
    PathResult result = pf.find_path({0, 10}, {49, 10}, grid, graph);

    ASSERT(result.found);
    ASSERT_EQ(result.path.size(), 50u);
    ASSERT_EQ(result.total_cost, 490u);  // 49 steps * 10
}

// ============================================================================
// Early exit via network_id (should return fast for disconnected)
// ============================================================================

TEST(early_exit_different_network_ids) {
    PathwayGrid grid(128, 128);
    // Two disconnected patches
    grid.set_pathway(0, 0, 1);
    grid.set_pathway(1, 0, 1);
    grid.set_pathway(100, 100, 2);
    grid.set_pathway(101, 100, 2);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    // Verify they have different network IDs
    uint16_t net_a = graph.get_network_id({0, 0});
    uint16_t net_b = graph.get_network_id({100, 100});
    ASSERT(net_a != 0);
    ASSERT(net_b != 0);
    ASSERT(net_a != net_b);

    Pathfinding pf;
    PathResult result = pf.find_path({0, 0}, {100, 100}, grid, graph);

    // Should return not found (early exit)
    ASSERT(!result.found);
}

// ============================================================================
// Vertical path
// ============================================================================

TEST(vertical_path) {
    PathwayGrid grid(16, 16);
    for (int y = 0; y < 10; ++y) {
        grid.set_pathway(5, y, 1);
    }

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    Pathfinding pf;
    PathResult result = pf.find_path({5, 0}, {5, 9}, grid, graph);

    ASSERT(result.found);
    ASSERT_EQ(result.path.size(), 10u);
    ASSERT_EQ(result.total_cost, 90u);
    // Verify path is ordered correctly
    ASSERT_EQ(result.path.front().x, 5);
    ASSERT_EQ(result.path.front().y, 0);
    ASSERT_EQ(result.path.back().x, 5);
    ASSERT_EQ(result.path.back().y, 9);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Pathfinding Unit Tests (Ticket E7-023) ===\n\n");

    RUN_TEST(trivial_same_position);
    RUN_TEST(straight_line_path);
    RUN_TEST(path_around_obstacle);
    RUN_TEST(no_path_disconnected);
    RUN_TEST(start_not_on_pathway);
    RUN_TEST(end_not_on_pathway);
    RUN_TEST(adjacent_tiles);
    RUN_TEST(shortest_route_chosen);
    RUN_TEST(larger_grid_path);
    RUN_TEST(early_exit_different_network_ids);
    RUN_TEST(vertical_path);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
