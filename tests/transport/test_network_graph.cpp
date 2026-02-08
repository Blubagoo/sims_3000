/**
 * @file test_network_graph.cpp
 * @brief Unit tests for NetworkGraph (Epic 7, Ticket E7-008)
 *
 * Standalone tests using assert macros (no test framework).
 */

#include <sims3000/transport/NetworkGraph.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

// ============================================================================
// GridPosition Tests
// ============================================================================

void test_grid_position_equality() {
    printf("Testing GridPosition equality...\n");

    GridPosition a{5, 10};
    GridPosition b{5, 10};
    GridPosition c{5, 11};
    GridPosition d{6, 10};

    assert(a == b);
    assert(a != c);
    assert(a != d);
    assert(c != d);

    printf("  PASS: GridPosition equality works correctly\n");
}

void test_grid_position_default() {
    printf("Testing GridPosition defaults...\n");

    GridPosition pos;
    assert(pos.x == 0);
    assert(pos.y == 0);

    printf("  PASS: GridPosition defaults to (0,0)\n");
}

void test_grid_position_hash() {
    printf("Testing GridPositionHash...\n");

    GridPositionHash hasher;
    GridPosition a{1, 2};
    GridPosition b{1, 2};
    GridPosition c{2, 1};

    // Same position should hash the same
    assert(hasher(a) == hasher(b));

    // Different positions should (likely) hash differently
    // This is not guaranteed but is expected for simple cases
    assert(hasher(a) != hasher(c));

    printf("  PASS: GridPositionHash works correctly\n");
}

// ============================================================================
// NetworkNode Tests
// ============================================================================

void test_network_node_defaults() {
    printf("Testing NetworkNode defaults...\n");

    NetworkNode node;
    assert(node.position.x == 0);
    assert(node.position.y == 0);
    assert(node.neighbor_indices.empty());
    assert(node.network_id == 0);

    printf("  PASS: NetworkNode defaults correct\n");
}

// ============================================================================
// NetworkGraph - Basic Operations
// ============================================================================

void test_graph_clear() {
    printf("Testing NetworkGraph::clear()...\n");

    NetworkGraph graph;
    graph.add_node({0, 0});
    graph.add_node({1, 0});
    assert(graph.node_count() == 2);

    graph.clear();
    assert(graph.node_count() == 0);
    assert(graph.get_node_index({0, 0}) == UINT16_MAX);

    printf("  PASS: clear() resets graph\n");
}

void test_graph_add_node() {
    printf("Testing NetworkGraph::add_node()...\n");

    NetworkGraph graph;

    uint16_t idx0 = graph.add_node({5, 10});
    assert(idx0 == 0);
    assert(graph.node_count() == 1);

    uint16_t idx1 = graph.add_node({6, 10});
    assert(idx1 == 1);
    assert(graph.node_count() == 2);

    // Adding duplicate position should return existing index
    uint16_t idx_dup = graph.add_node({5, 10});
    assert(idx_dup == 0);
    assert(graph.node_count() == 2);

    // Verify node data
    const NetworkNode& node0 = graph.get_node(0);
    assert(node0.position.x == 5);
    assert(node0.position.y == 10);
    assert(node0.network_id == 0);

    printf("  PASS: add_node() works correctly\n");
}

void test_graph_add_edge() {
    printf("Testing NetworkGraph::add_edge()...\n");

    NetworkGraph graph;
    uint16_t a = graph.add_node({0, 0});
    uint16_t b = graph.add_node({1, 0});
    uint16_t c = graph.add_node({2, 0});

    graph.add_edge(a, b);

    // Check bidirectional edge
    const NetworkNode& node_a = graph.get_node(a);
    const NetworkNode& node_b = graph.get_node(b);
    assert(node_a.neighbor_indices.size() == 1);
    assert(node_a.neighbor_indices[0] == b);
    assert(node_b.neighbor_indices.size() == 1);
    assert(node_b.neighbor_indices[0] == a);

    // Node c should have no neighbors
    const NetworkNode& node_c = graph.get_node(c);
    assert(node_c.neighbor_indices.empty());

    // Adding duplicate edge should not create duplicates
    graph.add_edge(a, b);
    assert(graph.get_node(a).neighbor_indices.size() == 1);
    assert(graph.get_node(b).neighbor_indices.size() == 1);

    // Self-edge should be ignored
    graph.add_edge(a, a);
    assert(graph.get_node(a).neighbor_indices.size() == 1);

    printf("  PASS: add_edge() works correctly\n");
}

void test_graph_add_edge_invalid() {
    printf("Testing NetworkGraph::add_edge() with invalid indices...\n");

    NetworkGraph graph;
    graph.add_node({0, 0});

    // Out-of-range indices should be ignored gracefully
    graph.add_edge(0, 99);
    assert(graph.get_node(0).neighbor_indices.empty());

    graph.add_edge(99, 0);
    assert(graph.get_node(0).neighbor_indices.empty());

    printf("  PASS: add_edge() handles invalid indices gracefully\n");
}

// ============================================================================
// NetworkGraph - Node Index Queries
// ============================================================================

void test_graph_get_node_index() {
    printf("Testing NetworkGraph::get_node_index()...\n");

    NetworkGraph graph;
    graph.add_node({3, 7});
    graph.add_node({4, 8});

    assert(graph.get_node_index({3, 7}) == 0);
    assert(graph.get_node_index({4, 8}) == 1);
    assert(graph.get_node_index({99, 99}) == UINT16_MAX);

    printf("  PASS: get_node_index() works correctly\n");
}

// ============================================================================
// NetworkGraph - Network ID Assignment (BFS)
// ============================================================================

void test_graph_assign_network_ids_single_component() {
    printf("Testing assign_network_ids() with single component...\n");

    NetworkGraph graph;
    uint16_t a = graph.add_node({0, 0});
    uint16_t b = graph.add_node({1, 0});
    uint16_t c = graph.add_node({2, 0});

    graph.add_edge(a, b);
    graph.add_edge(b, c);

    graph.assign_network_ids();

    assert(graph.get_node(a).network_id != 0);
    assert(graph.get_node(a).network_id == graph.get_node(b).network_id);
    assert(graph.get_node(b).network_id == graph.get_node(c).network_id);

    printf("  PASS: Single connected component assigned same network_id\n");
}

void test_graph_assign_network_ids_multiple_components() {
    printf("Testing assign_network_ids() with multiple components...\n");

    NetworkGraph graph;

    // Component 1: A-B
    uint16_t a = graph.add_node({0, 0});
    uint16_t b = graph.add_node({1, 0});
    graph.add_edge(a, b);

    // Component 2: C-D
    uint16_t c = graph.add_node({10, 10});
    uint16_t d = graph.add_node({11, 10});
    graph.add_edge(c, d);

    // Component 3: E (isolated)
    uint16_t e = graph.add_node({50, 50});
    (void)e;

    graph.assign_network_ids();

    // A and B should share same network_id
    uint16_t id_ab = graph.get_node(a).network_id;
    assert(id_ab != 0);
    assert(graph.get_node(b).network_id == id_ab);

    // C and D should share same network_id (different from A-B)
    uint16_t id_cd = graph.get_node(c).network_id;
    assert(id_cd != 0);
    assert(graph.get_node(d).network_id == id_cd);
    assert(id_cd != id_ab);

    // E should have its own network_id
    uint16_t id_e = graph.get_node(e).network_id;
    assert(id_e != 0);
    assert(id_e != id_ab);
    assert(id_e != id_cd);

    printf("  PASS: Multiple components assigned different network_ids\n");
}

void test_graph_assign_network_ids_empty() {
    printf("Testing assign_network_ids() on empty graph...\n");

    NetworkGraph graph;
    graph.assign_network_ids(); // Should not crash

    assert(graph.node_count() == 0);

    printf("  PASS: assign_network_ids() handles empty graph\n");
}

void test_graph_assign_network_ids_reassignment() {
    printf("Testing assign_network_ids() reassignment after topology change...\n");

    NetworkGraph graph;
    uint16_t a = graph.add_node({0, 0});
    uint16_t b = graph.add_node({1, 0});
    uint16_t c = graph.add_node({2, 0});

    // Initially two separate components: A-B and C
    graph.add_edge(a, b);
    graph.assign_network_ids();

    assert(graph.get_node(a).network_id == graph.get_node(b).network_id);
    assert(graph.get_node(a).network_id != graph.get_node(c).network_id);

    // Connect C to B, making one component
    graph.add_edge(b, c);
    graph.assign_network_ids();

    assert(graph.get_node(a).network_id == graph.get_node(b).network_id);
    assert(graph.get_node(b).network_id == graph.get_node(c).network_id);

    printf("  PASS: assign_network_ids() correctly reassigns after topology change\n");
}

// ============================================================================
// NetworkGraph - Connectivity Queries
// ============================================================================

void test_graph_is_connected() {
    printf("Testing NetworkGraph::is_connected()...\n");

    NetworkGraph graph;
    uint16_t a = graph.add_node({0, 0});
    uint16_t b = graph.add_node({1, 0});
    uint16_t c = graph.add_node({10, 10});

    graph.add_edge(a, b);
    graph.assign_network_ids();

    // A and B are connected
    assert(graph.is_connected({0, 0}, {1, 0}));

    // A and C are NOT connected
    assert(!graph.is_connected({0, 0}, {10, 10}));

    // Non-existent positions
    assert(!graph.is_connected({0, 0}, {99, 99}));
    assert(!graph.is_connected({99, 99}, {0, 0}));
    assert(!graph.is_connected({99, 99}, {88, 88}));

    printf("  PASS: is_connected() works correctly\n");
}

void test_graph_get_network_id() {
    printf("Testing NetworkGraph::get_network_id()...\n");

    NetworkGraph graph;
    graph.add_node({5, 5});
    graph.assign_network_ids();

    uint16_t id = graph.get_network_id({5, 5});
    assert(id != 0);

    // Non-existent position
    assert(graph.get_network_id({99, 99}) == 0);

    printf("  PASS: get_network_id() works correctly\n");
}

// ============================================================================
// NetworkGraph - Larger Graph Test
// ============================================================================

void test_graph_linear_chain() {
    printf("Testing linear chain of 100 nodes...\n");

    NetworkGraph graph;
    for (int i = 0; i < 100; ++i) {
        graph.add_node({i, 0});
    }
    for (int i = 0; i < 99; ++i) {
        uint16_t a = graph.get_node_index({i, 0});
        uint16_t b = graph.get_node_index({i + 1, 0});
        graph.add_edge(a, b);
    }

    graph.assign_network_ids();

    // All should be in same component
    uint16_t first_id = graph.get_network_id({0, 0});
    assert(first_id != 0);
    for (int i = 1; i < 100; ++i) {
        assert(graph.get_network_id({i, 0}) == first_id);
    }

    // First and last should be connected
    assert(graph.is_connected({0, 0}, {99, 0}));

    printf("  PASS: Linear chain connectivity correct\n");
}

void test_graph_grid_network() {
    printf("Testing 10x10 grid network...\n");

    NetworkGraph graph;

    // Create a 10x10 grid of nodes
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            graph.add_node({x, y});
        }
    }

    // Connect horizontal neighbors
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 9; ++x) {
            uint16_t a = graph.get_node_index({x, y});
            uint16_t b = graph.get_node_index({x + 1, y});
            graph.add_edge(a, b);
        }
    }

    // Connect vertical neighbors
    for (int y = 0; y < 9; ++y) {
        for (int x = 0; x < 10; ++x) {
            uint16_t a = graph.get_node_index({x, y});
            uint16_t b = graph.get_node_index({x, y + 1});
            graph.add_edge(a, b);
        }
    }

    assert(graph.node_count() == 100);

    graph.assign_network_ids();

    // All nodes in same component
    uint16_t first_id = graph.get_network_id({0, 0});
    assert(first_id != 0);
    assert(graph.is_connected({0, 0}, {9, 9}));
    assert(graph.is_connected({0, 9}, {9, 0}));

    printf("  PASS: 10x10 grid network connectivity correct\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== NetworkGraph Unit Tests (Epic 7, Ticket E7-008) ===\n\n");

    // GridPosition tests
    test_grid_position_equality();
    test_grid_position_default();
    test_grid_position_hash();

    // NetworkNode tests
    test_network_node_defaults();

    // Graph basic operations
    test_graph_clear();
    test_graph_add_node();
    test_graph_add_edge();
    test_graph_add_edge_invalid();
    test_graph_get_node_index();

    // Network ID assignment
    test_graph_assign_network_ids_single_component();
    test_graph_assign_network_ids_multiple_components();
    test_graph_assign_network_ids_empty();
    test_graph_assign_network_ids_reassignment();

    // Connectivity queries
    test_graph_is_connected();
    test_graph_get_network_id();

    // Larger graph tests
    test_graph_linear_chain();
    test_graph_grid_network();

    printf("\n=== All NetworkGraph Tests Passed ===\n");
    return 0;
}
