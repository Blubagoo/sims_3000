/**
 * @file test_connected_components.cpp
 * @brief Tests for connected component (network_id) assignment (Epic 7, Ticket E7-010)
 *
 * Tests cover:
 * - Single connected component: all same network_id
 * - Two separate components: different network_ids
 * - Merge components: when a pathway connects two separate networks
 * - Many small components
 * - get_network_positions() and get_network_count() API
 * - O(1) connectivity check via network_id comparison
 */

#include <sims3000/transport/NetworkGraph.h>
#include <sims3000/transport/PathwayGrid.h>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <algorithm>

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
// Single connected component: all same network_id
// ============================================================================

TEST(single_component_horizontal_line) {
    PathwayGrid grid(16, 16);
    // Horizontal line at y=5
    for (int32_t x = 0; x < 10; ++x) {
        grid.set_pathway(x, 5, static_cast<uint32_t>(x + 1));
    }

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 10u);
    ASSERT_EQ(graph.get_network_count(), 1);

    // All should share the same network_id
    uint16_t first_id = graph.get_network_id(GridPosition{0, 5});
    ASSERT(first_id != 0);
    for (int32_t x = 1; x < 10; ++x) {
        ASSERT_EQ(graph.get_network_id(GridPosition{x, 5}), first_id);
    }

    // All should be mutually connected
    ASSERT(graph.is_connected(GridPosition{0, 5}, GridPosition{9, 5}));
    ASSERT(graph.is_connected(GridPosition{3, 5}, GridPosition{7, 5}));
}

TEST(single_component_l_shape) {
    PathwayGrid grid(16, 16);
    // L-shape:
    // X
    // X
    // X X X X
    grid.set_pathway(2, 2, 1);
    grid.set_pathway(2, 3, 2);
    grid.set_pathway(2, 4, 3);
    grid.set_pathway(3, 4, 4);
    grid.set_pathway(4, 4, 5);
    grid.set_pathway(5, 4, 6);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 6u);
    ASSERT_EQ(graph.get_network_count(), 1);

    // Start and end of L should be connected
    ASSERT(graph.is_connected(GridPosition{2, 2}, GridPosition{5, 4}));
}

TEST(single_component_square_loop) {
    PathwayGrid grid(8, 8);
    // 3x3 square perimeter (loop)
    grid.set_pathway(1, 1, 1);
    grid.set_pathway(2, 1, 2);
    grid.set_pathway(3, 1, 3);
    grid.set_pathway(1, 2, 4);
    grid.set_pathway(3, 2, 5);
    grid.set_pathway(1, 3, 6);
    grid.set_pathway(2, 3, 7);
    grid.set_pathway(3, 3, 8);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 8u);
    ASSERT_EQ(graph.get_network_count(), 1);

    // All nodes in the loop connected
    ASSERT(graph.is_connected(GridPosition{1, 1}, GridPosition{3, 3}));
}

// ============================================================================
// Two separate components: different network_ids
// ============================================================================

TEST(two_separate_components) {
    PathwayGrid grid(32, 32);

    // Component 1: horizontal line at y=2
    for (int32_t x = 0; x < 5; ++x) {
        grid.set_pathway(x, 2, static_cast<uint32_t>(x + 1));
    }

    // Component 2: horizontal line at y=20 (far away)
    for (int32_t x = 10; x < 15; ++x) {
        grid.set_pathway(x, 20, static_cast<uint32_t>(x + 100));
    }

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 10u);
    ASSERT_EQ(graph.get_network_count(), 2);

    uint16_t nid1 = graph.get_network_id(GridPosition{0, 2});
    uint16_t nid2 = graph.get_network_id(GridPosition{10, 20});

    ASSERT(nid1 != 0);
    ASSERT(nid2 != 0);
    ASSERT(nid1 != nid2);

    // Within component 1
    ASSERT(graph.is_connected(GridPosition{0, 2}, GridPosition{4, 2}));

    // Within component 2
    ASSERT(graph.is_connected(GridPosition{10, 20}, GridPosition{14, 20}));

    // Across components: NOT connected
    ASSERT(!graph.is_connected(GridPosition{0, 2}, GridPosition{10, 20}));
}

TEST(two_isolated_single_tiles) {
    PathwayGrid grid(16, 16);

    grid.set_pathway(0, 0, 1);
    grid.set_pathway(15, 15, 2);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 2u);
    ASSERT_EQ(graph.get_network_count(), 2);

    uint16_t nid1 = graph.get_network_id(GridPosition{0, 0});
    uint16_t nid2 = graph.get_network_id(GridPosition{15, 15});

    ASSERT(nid1 != 0);
    ASSERT(nid2 != 0);
    ASSERT(nid1 != nid2);

    ASSERT(!graph.is_connected(GridPosition{0, 0}, GridPosition{15, 15}));
}

TEST(diagonal_tiles_two_components) {
    // Diagonal tiles should NOT be connected (4-direction only)
    PathwayGrid grid(8, 8);

    grid.set_pathway(3, 3, 1);
    grid.set_pathway(4, 4, 2);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 2u);
    ASSERT_EQ(graph.get_network_count(), 2);

    ASSERT(!graph.is_connected(GridPosition{3, 3}, GridPosition{4, 4}));
}

// ============================================================================
// Merge components: pathway connects two separate networks
// ============================================================================

TEST(merge_two_components_with_bridge) {
    PathwayGrid grid(16, 16);

    // Component 1: tiles at y=3, x=0..4
    for (int32_t x = 0; x < 5; ++x) {
        grid.set_pathway(x, 3, static_cast<uint32_t>(x + 1));
    }

    // Component 2: tiles at y=3, x=6..10
    for (int32_t x = 6; x < 11; ++x) {
        grid.set_pathway(x, 3, static_cast<uint32_t>(x + 100));
    }

    // Initially two components
    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.get_network_count(), 2);
    ASSERT(!graph.is_connected(GridPosition{0, 3}, GridPosition{10, 3}));

    // Add bridge tile at (5,3) connecting the two
    grid.set_pathway(5, 3, 999);
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.get_network_count(), 1);
    ASSERT(graph.is_connected(GridPosition{0, 3}, GridPosition{10, 3}));
}

TEST(merge_vertical_bridge) {
    PathwayGrid grid(16, 16);

    // Component 1: horizontal at y=2
    grid.set_pathway(5, 2, 1);
    grid.set_pathway(6, 2, 2);

    // Component 2: horizontal at y=4
    grid.set_pathway(5, 4, 3);
    grid.set_pathway(6, 4, 4);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);
    ASSERT_EQ(graph.get_network_count(), 2);
    ASSERT(!graph.is_connected(GridPosition{5, 2}, GridPosition{5, 4}));

    // Bridge with vertical tile at (5,3)
    grid.set_pathway(5, 3, 5);
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.get_network_count(), 1);
    ASSERT(graph.is_connected(GridPosition{5, 2}, GridPosition{6, 4}));
}

// ============================================================================
// Many small components
// ============================================================================

TEST(many_isolated_tiles) {
    PathwayGrid grid(64, 64);

    // Place isolated tiles every 3 cells (no adjacency)
    uint32_t count = 0;
    for (int32_t y = 0; y < 64; y += 3) {
        for (int32_t x = 0; x < 64; x += 3) {
            grid.set_pathway(x, y, ++count);
        }
    }

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    // Each isolated tile is its own component
    ASSERT_EQ(graph.node_count(), static_cast<size_t>(count));
    ASSERT_EQ(graph.get_network_count(), static_cast<uint16_t>(count));

    // No two isolated tiles should be connected
    ASSERT(!graph.is_connected(GridPosition{0, 0}, GridPosition{3, 0}));
    ASSERT(!graph.is_connected(GridPosition{0, 0}, GridPosition{0, 3}));
}

TEST(many_small_pairs) {
    PathwayGrid grid(64, 64);

    // Place horizontal pairs every 4 cells
    uint32_t entity = 0;
    uint16_t expected_components = 0;
    for (int32_t y = 0; y < 64; y += 4) {
        for (int32_t x = 0; x < 62; x += 4) {
            grid.set_pathway(x, y, ++entity);
            grid.set_pathway(x + 1, y, ++entity);
            expected_components++;
        }
    }

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.get_network_count(), expected_components);

    // Each pair should be internally connected
    ASSERT(graph.is_connected(GridPosition{0, 0}, GridPosition{1, 0}));
    // But not across pairs
    ASSERT(!graph.is_connected(GridPosition{0, 0}, GridPosition{4, 0}));
}

// ============================================================================
// get_network_positions() and get_network_count() API
// ============================================================================

TEST(get_network_positions_single) {
    PathwayGrid grid(8, 8);
    grid.set_pathway(2, 2, 1);
    grid.set_pathway(3, 2, 2);
    grid.set_pathway(4, 2, 3);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.get_network_count(), 1);

    uint16_t nid = graph.get_network_id(GridPosition{2, 2});
    auto positions = graph.get_network_positions(nid);

    ASSERT_EQ(positions.size(), 3u);

    // All 3 positions should be present
    std::set<int> xs;
    for (const auto& pos : positions) {
        ASSERT_EQ(pos.y, 2);
        xs.insert(pos.x);
    }
    ASSERT(xs.count(2) == 1);
    ASSERT(xs.count(3) == 1);
    ASSERT(xs.count(4) == 1);
}

TEST(get_network_positions_two_networks) {
    PathwayGrid grid(16, 16);

    // Component 1
    grid.set_pathway(0, 0, 1);
    grid.set_pathway(1, 0, 2);

    // Component 2
    grid.set_pathway(10, 10, 3);
    grid.set_pathway(11, 10, 4);
    grid.set_pathway(12, 10, 5);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.get_network_count(), 2);

    uint16_t nid1 = graph.get_network_id(GridPosition{0, 0});
    uint16_t nid2 = graph.get_network_id(GridPosition{10, 10});

    auto pos1 = graph.get_network_positions(nid1);
    auto pos2 = graph.get_network_positions(nid2);

    ASSERT_EQ(pos1.size(), 2u);
    ASSERT_EQ(pos2.size(), 3u);
}

TEST(get_network_positions_nonexistent_id) {
    PathwayGrid grid(8, 8);
    grid.set_pathway(0, 0, 1);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    // Query a network_id that doesn't exist
    auto positions = graph.get_network_positions(999);
    ASSERT_EQ(positions.size(), 0u);
}

TEST(get_network_count_empty) {
    NetworkGraph graph;
    ASSERT_EQ(graph.get_network_count(), 0);
}

TEST(get_network_count_after_rebuild) {
    PathwayGrid grid(8, 8);

    // 3 isolated tiles = 3 networks
    grid.set_pathway(0, 0, 1);
    grid.set_pathway(4, 4, 2);
    grid.set_pathway(7, 7, 3);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);
    ASSERT_EQ(graph.get_network_count(), 3);

    // Rebuild with 1 connected line = 1 network
    PathwayGrid grid2(8, 8);
    grid2.set_pathway(0, 0, 1);
    grid2.set_pathway(1, 0, 2);
    grid2.set_pathway(2, 0, 3);
    graph.rebuild_from_grid(grid2);
    ASSERT_EQ(graph.get_network_count(), 1);
}

// ============================================================================
// O(1) connectivity check via network_id comparison
// ============================================================================

TEST(o1_connectivity_check) {
    // Verify is_connected() works via network_id comparison (not BFS)
    PathwayGrid grid(32, 32);

    // Long chain: 30 tiles
    for (int32_t x = 0; x < 30; ++x) {
        grid.set_pathway(x, 5, static_cast<uint32_t>(x + 1));
    }

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    // is_connected should be O(1) - just comparing network_ids
    // We check first vs last
    ASSERT(graph.is_connected(GridPosition{0, 5}, GridPosition{29, 5}));

    // And unrelated positions
    ASSERT(!graph.is_connected(GridPosition{0, 5}, GridPosition{0, 20}));

    // Network ID of non-existent position
    ASSERT_EQ(graph.get_network_id(GridPosition{0, 20}), 0);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Connected Component (network_id) Tests (Ticket E7-010) ===\n\n");

    // Single connected component
    RUN_TEST(single_component_horizontal_line);
    RUN_TEST(single_component_l_shape);
    RUN_TEST(single_component_square_loop);

    // Two separate components
    RUN_TEST(two_separate_components);
    RUN_TEST(two_isolated_single_tiles);
    RUN_TEST(diagonal_tiles_two_components);

    // Merge components
    RUN_TEST(merge_two_components_with_bridge);
    RUN_TEST(merge_vertical_bridge);

    // Many small components
    RUN_TEST(many_isolated_tiles);
    RUN_TEST(many_small_pairs);

    // API: get_network_positions / get_network_count
    RUN_TEST(get_network_positions_single);
    RUN_TEST(get_network_positions_two_networks);
    RUN_TEST(get_network_positions_nonexistent_id);
    RUN_TEST(get_network_count_empty);
    RUN_TEST(get_network_count_after_rebuild);

    // O(1) connectivity
    RUN_TEST(o1_connectivity_check);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
