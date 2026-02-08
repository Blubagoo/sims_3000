/**
 * @file test_connectivity_query.cpp
 * @brief Unit tests for ConnectivityQuery O(1) connectivity checks (Epic 7, Ticket E7-011)
 *
 * Tests cover:
 * - O(1) connectivity query after graph rebuild
 * - Missing pathways return false
 * - Positions on different networks are not connected
 * - Positions on the same network are connected
 * - is_on_network checks
 * - get_network_id_at lookups
 * - Out-of-bounds positions
 * - Empty grid (no pathways)
 */

#include <sims3000/transport/ConnectivityQuery.h>
#include <sims3000/transport/PathwayGrid.h>
#include <sims3000/transport/NetworkGraph.h>
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
        printf("\n  FAILED: %s == %s (line %d, got %d vs %d)\n", #a, #b, __LINE__, (int)(a), (int)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// ============================================================================
// Helper: build a grid+graph with pathways and rebuild
// ============================================================================

static void setup_grid_and_graph(PathwayGrid& grid, NetworkGraph& graph) {
    graph.rebuild_from_grid(grid);
}

// ============================================================================
// Empty grid tests
// ============================================================================

TEST(empty_grid_not_connected) {
    PathwayGrid grid(16, 16);
    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    ASSERT(!query.is_connected(grid, graph, 0, 0, 5, 5));
}

TEST(empty_grid_not_on_network) {
    PathwayGrid grid(16, 16);
    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    ASSERT(!query.is_on_network(grid, graph, 0, 0));
    ASSERT(!query.is_on_network(grid, graph, 8, 8));
}

TEST(empty_grid_network_id_zero) {
    PathwayGrid grid(16, 16);
    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    ASSERT_EQ(query.get_network_id_at(grid, graph, 0, 0), 0);
    ASSERT_EQ(query.get_network_id_at(grid, graph, 15, 15), 0);
}

// ============================================================================
// Single connected component tests
// ============================================================================

TEST(single_tile_connected_to_self) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);

    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    ASSERT(query.is_connected(grid, graph, 5, 5, 5, 5));
}

TEST(adjacent_tiles_connected) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);
    grid.set_pathway(6, 5, 2);
    grid.set_pathway(7, 5, 3);

    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    ASSERT(query.is_connected(grid, graph, 5, 5, 7, 5));
    ASSERT(query.is_connected(grid, graph, 5, 5, 6, 5));
    ASSERT(query.is_connected(grid, graph, 6, 5, 7, 5));
}

TEST(l_shaped_road_connected) {
    PathwayGrid grid(16, 16);
    // Horizontal segment
    grid.set_pathway(2, 5, 1);
    grid.set_pathway(3, 5, 2);
    grid.set_pathway(4, 5, 3);
    // Vertical segment from (4,5) down
    grid.set_pathway(4, 6, 4);
    grid.set_pathway(4, 7, 5);

    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    // All tiles in the L should be connected
    ASSERT(query.is_connected(grid, graph, 2, 5, 4, 7));
    ASSERT(query.is_connected(grid, graph, 3, 5, 4, 6));
}

TEST(single_network_same_id) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(0, 0, 1);
    grid.set_pathway(1, 0, 2);
    grid.set_pathway(2, 0, 3);

    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    uint16_t id0 = query.get_network_id_at(grid, graph, 0, 0);
    uint16_t id1 = query.get_network_id_at(grid, graph, 1, 0);
    uint16_t id2 = query.get_network_id_at(grid, graph, 2, 0);

    ASSERT(id0 != 0);
    ASSERT_EQ(id0, id1);
    ASSERT_EQ(id1, id2);
}

// ============================================================================
// Two disconnected networks
// ============================================================================

TEST(two_networks_not_connected) {
    PathwayGrid grid(16, 16);
    // Network A
    grid.set_pathway(0, 0, 1);
    grid.set_pathway(1, 0, 2);
    // Network B (separated, not adjacent)
    grid.set_pathway(10, 10, 3);
    grid.set_pathway(11, 10, 4);

    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    // Within same network: connected
    ASSERT(query.is_connected(grid, graph, 0, 0, 1, 0));
    ASSERT(query.is_connected(grid, graph, 10, 10, 11, 10));

    // Across networks: not connected
    ASSERT(!query.is_connected(grid, graph, 0, 0, 10, 10));
    ASSERT(!query.is_connected(grid, graph, 1, 0, 11, 10));
}

TEST(two_networks_different_ids) {
    PathwayGrid grid(16, 16);
    // Network A
    grid.set_pathway(0, 0, 1);
    grid.set_pathway(1, 0, 2);
    // Network B
    grid.set_pathway(10, 10, 3);
    grid.set_pathway(11, 10, 4);

    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    uint16_t id_a = query.get_network_id_at(grid, graph, 0, 0);
    uint16_t id_b = query.get_network_id_at(grid, graph, 10, 10);

    ASSERT(id_a != 0);
    ASSERT(id_b != 0);
    ASSERT(id_a != id_b);
}

// ============================================================================
// Missing pathways
// ============================================================================

TEST(missing_pathway_returns_false) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);

    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    // One position has pathway, other doesn't
    ASSERT(!query.is_connected(grid, graph, 5, 5, 6, 5));
    ASSERT(!query.is_connected(grid, graph, 6, 5, 5, 5));
    // Neither has pathway
    ASSERT(!query.is_connected(grid, graph, 0, 0, 1, 1));
}

TEST(missing_pathway_not_on_network) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);

    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    ASSERT(query.is_on_network(grid, graph, 5, 5));
    ASSERT(!query.is_on_network(grid, graph, 6, 5));
}

TEST(missing_pathway_network_id_zero) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);

    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    ASSERT(query.get_network_id_at(grid, graph, 5, 5) != 0);
    ASSERT_EQ(query.get_network_id_at(grid, graph, 6, 5), 0);
}

// ============================================================================
// Out-of-bounds positions
// ============================================================================

TEST(out_of_bounds_not_connected) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);

    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    ASSERT(!query.is_connected(grid, graph, 5, 5, -1, 0));
    ASSERT(!query.is_connected(grid, graph, 5, 5, 16, 0));
    ASSERT(!query.is_connected(grid, graph, -1, -1, 5, 5));
}

TEST(out_of_bounds_not_on_network) {
    PathwayGrid grid(16, 16);
    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    ASSERT(!query.is_on_network(grid, graph, -1, 0));
    ASSERT(!query.is_on_network(grid, graph, 0, -1));
    ASSERT(!query.is_on_network(grid, graph, 16, 0));
    ASSERT(!query.is_on_network(grid, graph, 0, 16));
}

TEST(out_of_bounds_network_id_zero) {
    PathwayGrid grid(16, 16);
    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    ASSERT_EQ(query.get_network_id_at(grid, graph, -1, 0), 0);
    ASSERT_EQ(query.get_network_id_at(grid, graph, 100, 100), 0);
}

// ============================================================================
// Cross-ownership connectivity (CCR-002)
// ============================================================================

TEST(cross_ownership_connected) {
    PathwayGrid grid(16, 16);
    // Player 1's road
    grid.set_pathway(3, 3, 100);  // entity_id 100, player 1
    // Player 2's road (adjacent)
    grid.set_pathway(4, 3, 200);  // entity_id 200, player 2

    NetworkGraph graph;
    setup_grid_and_graph(grid, graph);
    ConnectivityQuery query;

    // Cross-ownership: adjacent tiles are connected regardless of owner
    ASSERT(query.is_connected(grid, graph, 3, 3, 4, 3));
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== ConnectivityQuery Unit Tests (Ticket E7-011) ===\n\n");

    // Empty grid
    RUN_TEST(empty_grid_not_connected);
    RUN_TEST(empty_grid_not_on_network);
    RUN_TEST(empty_grid_network_id_zero);

    // Single connected component
    RUN_TEST(single_tile_connected_to_self);
    RUN_TEST(adjacent_tiles_connected);
    RUN_TEST(l_shaped_road_connected);
    RUN_TEST(single_network_same_id);

    // Two disconnected networks
    RUN_TEST(two_networks_not_connected);
    RUN_TEST(two_networks_different_ids);

    // Missing pathways
    RUN_TEST(missing_pathway_returns_false);
    RUN_TEST(missing_pathway_not_on_network);
    RUN_TEST(missing_pathway_network_id_zero);

    // Out-of-bounds
    RUN_TEST(out_of_bounds_not_connected);
    RUN_TEST(out_of_bounds_not_on_network);
    RUN_TEST(out_of_bounds_network_id_zero);

    // Cross-ownership
    RUN_TEST(cross_ownership_connected);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
