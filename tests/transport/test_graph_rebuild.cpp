/**
 * @file test_graph_rebuild.cpp
 * @brief Unit tests for NetworkGraph::rebuild_from_grid (Epic 7, Ticket E7-009)
 *
 * Tests cover:
 * - Basic rebuild from PathwayGrid
 * - Cross-ownership connectivity (no owner check per CCR-002)
 * - Network ID assignment to connected components
 * - Multiple disconnected components
 * - Single-tile networks
 * - Empty grid (no nodes)
 * - L-shaped and complex topologies
 * - Performance target: <50ms on 256x256 with 15,000 segments
 */

#include <sims3000/transport/NetworkGraph.h>
#include <sims3000/transport/PathwayGrid.h>
#include <cstdio>
#include <cstdlib>
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
// Empty grid test
// ============================================================================

TEST(empty_grid) {
    PathwayGrid grid(32, 32);
    NetworkGraph graph;

    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 0u);
}

// ============================================================================
// Single tile tests
// ============================================================================

TEST(single_tile) {
    PathwayGrid grid(8, 8);
    grid.set_pathway(3, 3, 1);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 1u);

    uint16_t nid = graph.get_network_id(GridPosition{3, 3});
    ASSERT(nid != 0);
}

TEST(single_tile_no_neighbors) {
    PathwayGrid grid(8, 8);
    grid.set_pathway(4, 4, 1);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 1u);

    uint16_t idx = graph.get_node_index(GridPosition{4, 4});
    ASSERT(idx != UINT16_MAX);

    const NetworkNode& node = graph.get_node(idx);
    ASSERT_EQ(node.neighbor_indices.size(), 0u);
}

// ============================================================================
// Adjacent tiles connectivity
// ============================================================================

TEST(two_adjacent_horizontal) {
    PathwayGrid grid(8, 8);
    grid.set_pathway(3, 3, 1);
    grid.set_pathway(4, 3, 2);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 2u);

    // Should be in the same network
    ASSERT(graph.is_connected(GridPosition{3, 3}, GridPosition{4, 3}));
}

TEST(two_adjacent_vertical) {
    PathwayGrid grid(8, 8);
    grid.set_pathway(3, 3, 1);
    grid.set_pathway(3, 4, 2);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 2u);
    ASSERT(graph.is_connected(GridPosition{3, 3}, GridPosition{3, 4}));
}

TEST(two_diagonal_not_connected) {
    PathwayGrid grid(8, 8);
    grid.set_pathway(3, 3, 1);
    grid.set_pathway(4, 4, 2);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 2u);

    // Diagonal tiles should NOT be connected (4-directional only)
    ASSERT(!graph.is_connected(GridPosition{3, 3}, GridPosition{4, 4}));
}

// ============================================================================
// Cross-ownership connectivity (CCR-002)
// ============================================================================

TEST(cross_ownership_connection) {
    // Two tiles with different entity IDs (simulating different owners)
    // should still be connected - no owner check per CCR-002
    PathwayGrid grid(8, 8);
    grid.set_pathway(3, 3, 100);  // "player A's" pathway
    grid.set_pathway(4, 3, 200);  // "player B's" pathway

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 2u);
    ASSERT(graph.is_connected(GridPosition{3, 3}, GridPosition{4, 3}));
}

// ============================================================================
// Connected components
// ============================================================================

TEST(two_disconnected_components) {
    PathwayGrid grid(16, 16);

    // Component 1: horizontal line at y=2
    grid.set_pathway(0, 2, 1);
    grid.set_pathway(1, 2, 2);
    grid.set_pathway(2, 2, 3);

    // Component 2: horizontal line at y=10 (far away, no connection)
    grid.set_pathway(10, 10, 4);
    grid.set_pathway(11, 10, 5);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 5u);

    // Within component 1: all connected
    ASSERT(graph.is_connected(GridPosition{0, 2}, GridPosition{1, 2}));
    ASSERT(graph.is_connected(GridPosition{0, 2}, GridPosition{2, 2}));
    ASSERT(graph.is_connected(GridPosition{1, 2}, GridPosition{2, 2}));

    // Within component 2: connected
    ASSERT(graph.is_connected(GridPosition{10, 10}, GridPosition{11, 10}));

    // Across components: NOT connected
    ASSERT(!graph.is_connected(GridPosition{0, 2}, GridPosition{10, 10}));
    ASSERT(!graph.is_connected(GridPosition{2, 2}, GridPosition{11, 10}));

    // Different network IDs
    uint16_t nid1 = graph.get_network_id(GridPosition{0, 2});
    uint16_t nid2 = graph.get_network_id(GridPosition{10, 10});
    ASSERT(nid1 != 0);
    ASSERT(nid2 != 0);
    ASSERT(nid1 != nid2);
}

TEST(three_components) {
    PathwayGrid grid(16, 16);

    // Component 1: single tile
    grid.set_pathway(0, 0, 1);

    // Component 2: vertical line
    grid.set_pathway(8, 0, 2);
    grid.set_pathway(8, 1, 3);
    grid.set_pathway(8, 2, 4);

    // Component 3: L-shape
    grid.set_pathway(0, 10, 5);
    grid.set_pathway(1, 10, 6);
    grid.set_pathway(1, 11, 7);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 7u);

    uint16_t nid1 = graph.get_network_id(GridPosition{0, 0});
    uint16_t nid2 = graph.get_network_id(GridPosition{8, 0});
    uint16_t nid3 = graph.get_network_id(GridPosition{0, 10});

    ASSERT(nid1 != 0);
    ASSERT(nid2 != 0);
    ASSERT(nid3 != 0);
    ASSERT(nid1 != nid2);
    ASSERT(nid1 != nid3);
    ASSERT(nid2 != nid3);

    // L-shape internal connectivity
    ASSERT(graph.is_connected(GridPosition{0, 10}, GridPosition{1, 10}));
    ASSERT(graph.is_connected(GridPosition{1, 10}, GridPosition{1, 11}));
    ASSERT(graph.is_connected(GridPosition{0, 10}, GridPosition{1, 11}));
}

// ============================================================================
// Network ID assignment
// ============================================================================

TEST(network_ids_non_zero) {
    PathwayGrid grid(8, 8);
    grid.set_pathway(0, 0, 1);
    grid.set_pathway(1, 0, 2);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    uint16_t nid = graph.get_network_id(GridPosition{0, 0});
    ASSERT(nid != 0);
}

TEST(same_component_same_id) {
    PathwayGrid grid(8, 8);
    // 3-tile horizontal line
    grid.set_pathway(2, 4, 1);
    grid.set_pathway(3, 4, 2);
    grid.set_pathway(4, 4, 3);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    uint16_t nid_a = graph.get_network_id(GridPosition{2, 4});
    uint16_t nid_b = graph.get_network_id(GridPosition{3, 4});
    uint16_t nid_c = graph.get_network_id(GridPosition{4, 4});

    ASSERT_EQ(nid_a, nid_b);
    ASSERT_EQ(nid_b, nid_c);
}

// ============================================================================
// Rebuild clears previous state
// ============================================================================

TEST(rebuild_clears_previous) {
    PathwayGrid grid(8, 8);
    grid.set_pathway(0, 0, 1);
    grid.set_pathway(1, 0, 2);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);
    ASSERT_EQ(graph.node_count(), 2u);

    // Second rebuild with different grid
    PathwayGrid grid2(8, 8);
    grid2.set_pathway(5, 5, 3);

    graph.rebuild_from_grid(grid2);
    ASSERT_EQ(graph.node_count(), 1u);

    // Old positions should no longer exist
    ASSERT_EQ(graph.get_node_index(GridPosition{0, 0}), UINT16_MAX);
    ASSERT_EQ(graph.get_node_index(GridPosition{1, 0}), UINT16_MAX);

    // New position should exist
    ASSERT(graph.get_node_index(GridPosition{5, 5}) != UINT16_MAX);
}

// ============================================================================
// Edge connectivity (neighbor counts)
// ============================================================================

TEST(neighbor_counts_straight_line) {
    PathwayGrid grid(8, 8);
    // Straight horizontal line: 0,1,2,3 at y=0
    grid.set_pathway(0, 0, 1);
    grid.set_pathway(1, 0, 2);
    grid.set_pathway(2, 0, 3);
    grid.set_pathway(3, 0, 4);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    // End tiles have 1 neighbor, middle tiles have 2
    uint16_t idx0 = graph.get_node_index(GridPosition{0, 0});
    uint16_t idx1 = graph.get_node_index(GridPosition{1, 0});
    uint16_t idx2 = graph.get_node_index(GridPosition{2, 0});
    uint16_t idx3 = graph.get_node_index(GridPosition{3, 0});

    ASSERT_EQ(graph.get_node(idx0).neighbor_indices.size(), 1u);
    ASSERT_EQ(graph.get_node(idx1).neighbor_indices.size(), 2u);
    ASSERT_EQ(graph.get_node(idx2).neighbor_indices.size(), 2u);
    ASSERT_EQ(graph.get_node(idx3).neighbor_indices.size(), 1u);
}

TEST(cross_intersection_4_neighbors) {
    PathwayGrid grid(8, 8);
    //     X
    //   X X X
    //     X
    grid.set_pathway(3, 3, 1);  // center
    grid.set_pathway(2, 3, 2);  // west
    grid.set_pathway(4, 3, 3);  // east
    grid.set_pathway(3, 2, 4);  // north
    grid.set_pathway(3, 4, 5);  // south

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 5u);

    uint16_t center_idx = graph.get_node_index(GridPosition{3, 3});
    ASSERT_EQ(graph.get_node(center_idx).neighbor_indices.size(), 4u);
}

// ============================================================================
// L-shape topology
// ============================================================================

TEST(l_shape) {
    PathwayGrid grid(8, 8);
    // L-shape:
    // X
    // X
    // X X X
    grid.set_pathway(0, 0, 1);
    grid.set_pathway(0, 1, 2);
    grid.set_pathway(0, 2, 3);
    grid.set_pathway(1, 2, 4);
    grid.set_pathway(2, 2, 5);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 5u);

    // All should be connected
    ASSERT(graph.is_connected(GridPosition{0, 0}, GridPosition{2, 2}));

    // Corner tile (0,2) should have 2 neighbors
    uint16_t corner_idx = graph.get_node_index(GridPosition{0, 2});
    ASSERT_EQ(graph.get_node(corner_idx).neighbor_indices.size(), 2u);
}

// ============================================================================
// Performance test
// ============================================================================

TEST(performance_256x256_15k_segments) {
    PathwayGrid grid(256, 256);

    // Place ~15,000 pathway segments in a grid pattern
    // Every 4th column, every 4th row creates a cross-hatch
    uint32_t count = 0;
    for (int32_t y = 0; y < 256 && count < 15000; ++y) {
        for (int32_t x = 0; x < 256 && count < 15000; ++x) {
            if (x % 4 == 0 || y % 4 == 0) {
                grid.set_pathway(x, y, count + 1);
                count++;
            }
        }
    }

    printf(" (%u segments placed) ", count);

    NetworkGraph graph;

    auto start = std::chrono::high_resolution_clock::now();
    graph.rebuild_from_grid(grid);
    auto end = std::chrono::high_resolution_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    printf("[%lldms] ", (long long)ms);

    // Verify we got nodes
    ASSERT(graph.node_count() > 0);
    ASSERT(graph.node_count() <= count);

    // Performance target: <50ms
    ASSERT(ms < 50);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== NetworkGraph rebuild_from_grid Unit Tests (Ticket E7-009) ===\n\n");

    // Empty grid
    RUN_TEST(empty_grid);

    // Single tile
    RUN_TEST(single_tile);
    RUN_TEST(single_tile_no_neighbors);

    // Adjacent tiles
    RUN_TEST(two_adjacent_horizontal);
    RUN_TEST(two_adjacent_vertical);
    RUN_TEST(two_diagonal_not_connected);

    // Cross-ownership
    RUN_TEST(cross_ownership_connection);

    // Connected components
    RUN_TEST(two_disconnected_components);
    RUN_TEST(three_components);

    // Network IDs
    RUN_TEST(network_ids_non_zero);
    RUN_TEST(same_component_same_id);

    // Rebuild clears state
    RUN_TEST(rebuild_clears_previous);

    // Edge connectivity
    RUN_TEST(neighbor_counts_straight_line);
    RUN_TEST(cross_intersection_4_neighbors);

    // Topology
    RUN_TEST(l_shape);

    // Performance
    RUN_TEST(performance_256x256_15k_segments);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
