/**
 * @file test_cross_ownership.cpp
 * @brief Tests for cross-ownership connectivity in NetworkGraph (Epic 7, Ticket E7-020)
 *
 * Verifies that NetworkGraph connects pathways regardless of ownership per CCR-002.
 * The graph rebuild (E7-009) connects adjacent tiles without checking owner.
 *
 * Tests cover:
 * - Player A and Player B adjacent pathways: same network
 * - Multiple players sharing pathway network: single network_id
 * - Separate networks for disconnected players: different network_ids
 * - Ownership preserved in RoadComponent (not in graph)
 * - Mixed ownership topologies
 */

#include <sims3000/transport/NetworkGraph.h>
#include <sims3000/transport/PathwayGrid.h>
#include <sims3000/transport/RoadComponent.h>
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

// Simulated player entity IDs (different "owners")
static constexpr uint32_t PLAYER_A_ENTITY_BASE = 1000;
static constexpr uint32_t PLAYER_B_ENTITY_BASE = 2000;
static constexpr uint32_t PLAYER_C_ENTITY_BASE = 3000;
static constexpr uint32_t PLAYER_D_ENTITY_BASE = 4000;

// ============================================================================
// Player A and Player B adjacent pathways: same network
// ============================================================================

TEST(two_players_horizontal_adjacent) {
    PathwayGrid grid(16, 16);

    // Player A's pathway at (5,5)
    grid.set_pathway(5, 5, PLAYER_A_ENTITY_BASE + 1);
    // Player B's pathway at (6,5) - adjacent horizontally
    grid.set_pathway(6, 5, PLAYER_B_ENTITY_BASE + 1);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 2u);
    ASSERT_EQ(graph.get_network_count(), 1);

    // They should be in the same network despite different "owners"
    ASSERT(graph.is_connected(GridPosition{5, 5}, GridPosition{6, 5}));

    uint16_t nid_a = graph.get_network_id(GridPosition{5, 5});
    uint16_t nid_b = graph.get_network_id(GridPosition{6, 5});
    ASSERT(nid_a != 0);
    ASSERT_EQ(nid_a, nid_b);
}

TEST(two_players_vertical_adjacent) {
    PathwayGrid grid(16, 16);

    // Player A's pathway at (5,5)
    grid.set_pathway(5, 5, PLAYER_A_ENTITY_BASE + 1);
    // Player B's pathway at (5,6) - adjacent vertically
    grid.set_pathway(5, 6, PLAYER_B_ENTITY_BASE + 1);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 2u);
    ASSERT(graph.is_connected(GridPosition{5, 5}, GridPosition{5, 6}));
}

TEST(two_players_alternating_tiles) {
    PathwayGrid grid(16, 16);

    // A-B-A-B-A pattern at y=3
    grid.set_pathway(0, 3, PLAYER_A_ENTITY_BASE + 1);
    grid.set_pathway(1, 3, PLAYER_B_ENTITY_BASE + 1);
    grid.set_pathway(2, 3, PLAYER_A_ENTITY_BASE + 2);
    grid.set_pathway(3, 3, PLAYER_B_ENTITY_BASE + 2);
    grid.set_pathway(4, 3, PLAYER_A_ENTITY_BASE + 3);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 5u);
    ASSERT_EQ(graph.get_network_count(), 1);

    // First and last should be connected despite alternating ownership
    ASSERT(graph.is_connected(GridPosition{0, 3}, GridPosition{4, 3}));
}

// ============================================================================
// Multiple players sharing pathway network: single network_id
// ============================================================================

TEST(three_players_single_network) {
    PathwayGrid grid(16, 16);

    // Player A's segment
    grid.set_pathway(2, 5, PLAYER_A_ENTITY_BASE + 1);
    grid.set_pathway(3, 5, PLAYER_A_ENTITY_BASE + 2);

    // Player B's segment (adjacent to A)
    grid.set_pathway(4, 5, PLAYER_B_ENTITY_BASE + 1);
    grid.set_pathway(5, 5, PLAYER_B_ENTITY_BASE + 2);

    // Player C's segment (adjacent to B)
    grid.set_pathway(6, 5, PLAYER_C_ENTITY_BASE + 1);
    grid.set_pathway(7, 5, PLAYER_C_ENTITY_BASE + 2);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 6u);
    ASSERT_EQ(graph.get_network_count(), 1);

    // All three players' tiles share the same network
    uint16_t nid_a = graph.get_network_id(GridPosition{2, 5});
    uint16_t nid_b = graph.get_network_id(GridPosition{4, 5});
    uint16_t nid_c = graph.get_network_id(GridPosition{6, 5});

    ASSERT(nid_a != 0);
    ASSERT_EQ(nid_a, nid_b);
    ASSERT_EQ(nid_b, nid_c);

    ASSERT(graph.is_connected(GridPosition{2, 5}, GridPosition{7, 5}));
}

TEST(four_players_cross_intersection) {
    PathwayGrid grid(16, 16);

    // Player A: west arm
    grid.set_pathway(3, 5, PLAYER_A_ENTITY_BASE + 1);
    grid.set_pathway(4, 5, PLAYER_A_ENTITY_BASE + 2);

    // Player B: east arm
    grid.set_pathway(6, 5, PLAYER_B_ENTITY_BASE + 1);
    grid.set_pathway(7, 5, PLAYER_B_ENTITY_BASE + 2);

    // Player C: north arm
    grid.set_pathway(5, 3, PLAYER_C_ENTITY_BASE + 1);
    grid.set_pathway(5, 4, PLAYER_C_ENTITY_BASE + 2);

    // Player D: south arm
    grid.set_pathway(5, 6, PLAYER_D_ENTITY_BASE + 1);
    grid.set_pathway(5, 7, PLAYER_D_ENTITY_BASE + 2);

    // Shared center tile (could be any player)
    grid.set_pathway(5, 5, PLAYER_A_ENTITY_BASE + 99);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 9u);
    ASSERT_EQ(graph.get_network_count(), 1);

    // All four arms connected through center
    ASSERT(graph.is_connected(GridPosition{3, 5}, GridPosition{7, 5}));
    ASSERT(graph.is_connected(GridPosition{5, 3}, GridPosition{5, 7}));
    ASSERT(graph.is_connected(GridPosition{3, 5}, GridPosition{5, 3}));
}

TEST(many_players_large_network) {
    PathwayGrid grid(32, 32);

    // 8 different "players" each contribute 4 tiles in a connected chain
    for (int player = 0; player < 8; ++player) {
        uint32_t base = static_cast<uint32_t>((player + 1) * 1000);
        int32_t x_start = player * 4;
        for (int32_t i = 0; i < 4; ++i) {
            grid.set_pathway(x_start + i, 10, base + static_cast<uint32_t>(i));
        }
    }

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 32u);
    ASSERT_EQ(graph.get_network_count(), 1);

    // First tile of first player to last tile of last player
    ASSERT(graph.is_connected(GridPosition{0, 10}, GridPosition{31, 10}));
}

// ============================================================================
// Separate networks for disconnected players: different network_ids
// ============================================================================

TEST(two_players_disconnected) {
    PathwayGrid grid(32, 32);

    // Player A's segment at top
    grid.set_pathway(2, 2, PLAYER_A_ENTITY_BASE + 1);
    grid.set_pathway(3, 2, PLAYER_A_ENTITY_BASE + 2);
    grid.set_pathway(4, 2, PLAYER_A_ENTITY_BASE + 3);

    // Player B's segment at bottom (no adjacency to A)
    grid.set_pathway(20, 20, PLAYER_B_ENTITY_BASE + 1);
    grid.set_pathway(21, 20, PLAYER_B_ENTITY_BASE + 2);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 5u);
    ASSERT_EQ(graph.get_network_count(), 2);

    uint16_t nid_a = graph.get_network_id(GridPosition{2, 2});
    uint16_t nid_b = graph.get_network_id(GridPosition{20, 20});

    ASSERT(nid_a != 0);
    ASSERT(nid_b != 0);
    ASSERT(nid_a != nid_b);

    ASSERT(!graph.is_connected(GridPosition{2, 2}, GridPosition{20, 20}));
}

TEST(same_player_two_disconnected_segments) {
    PathwayGrid grid(32, 32);

    // Same player, two separate segments
    grid.set_pathway(0, 0, PLAYER_A_ENTITY_BASE + 1);
    grid.set_pathway(1, 0, PLAYER_A_ENTITY_BASE + 2);

    grid.set_pathway(20, 20, PLAYER_A_ENTITY_BASE + 3);
    grid.set_pathway(21, 20, PLAYER_A_ENTITY_BASE + 4);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.get_network_count(), 2);

    // Same player but disconnected segments = different networks
    ASSERT(!graph.is_connected(GridPosition{0, 0}, GridPosition{20, 20}));
}

TEST(three_players_mixed_connectivity) {
    PathwayGrid grid(32, 32);

    // Player A + Player B connected
    grid.set_pathway(0, 5, PLAYER_A_ENTITY_BASE + 1);
    grid.set_pathway(1, 5, PLAYER_B_ENTITY_BASE + 1);

    // Player C isolated
    grid.set_pathway(20, 20, PLAYER_C_ENTITY_BASE + 1);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.get_network_count(), 2);

    // A and B connected
    ASSERT(graph.is_connected(GridPosition{0, 5}, GridPosition{1, 5}));

    // C is separate
    ASSERT(!graph.is_connected(GridPosition{0, 5}, GridPosition{20, 20}));
    ASSERT(!graph.is_connected(GridPosition{1, 5}, GridPosition{20, 20}));
}

// ============================================================================
// Ownership preserved in RoadComponent (not in graph)
// ============================================================================

TEST(ownership_preserved_in_grid) {
    // The PathwayGrid stores entity IDs (which encode ownership)
    // NetworkGraph doesn't store ownership - only connectivity
    PathwayGrid grid(16, 16);

    grid.set_pathway(5, 5, PLAYER_A_ENTITY_BASE + 1);
    grid.set_pathway(6, 5, PLAYER_B_ENTITY_BASE + 1);

    // Verify entity IDs are preserved in the grid
    ASSERT_EQ(grid.get_pathway_at(5, 5), PLAYER_A_ENTITY_BASE + 1);
    ASSERT_EQ(grid.get_pathway_at(6, 5), PLAYER_B_ENTITY_BASE + 1);

    // Build graph - should connect them
    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT(graph.is_connected(GridPosition{5, 5}, GridPosition{6, 5}));

    // Entity IDs still preserved in grid after graph rebuild
    ASSERT_EQ(grid.get_pathway_at(5, 5), PLAYER_A_ENTITY_BASE + 1);
    ASSERT_EQ(grid.get_pathway_at(6, 5), PLAYER_B_ENTITY_BASE + 1);
}

TEST(road_component_network_id_independent_of_ownership) {
    // RoadComponent stores both network_id and ownership independently
    RoadComponent comp_a;
    comp_a.network_id = 1;

    RoadComponent comp_b;
    comp_b.network_id = 1;  // Same network

    // Both have the same network_id (connected) but could have different
    // ownership represented by their entity associations
    ASSERT_EQ(comp_a.network_id, comp_b.network_id);
    ASSERT_EQ(sizeof(RoadComponent), 16u);  // Verify struct size
}

TEST(entity_ids_remain_distinct_in_grid) {
    // Even after graph rebuild, each tile retains its original entity ID
    PathwayGrid grid(8, 8);

    uint32_t entities[] = {
        PLAYER_A_ENTITY_BASE + 1,
        PLAYER_B_ENTITY_BASE + 1,
        PLAYER_C_ENTITY_BASE + 1,
        PLAYER_D_ENTITY_BASE + 1
    };

    grid.set_pathway(0, 0, entities[0]);
    grid.set_pathway(1, 0, entities[1]);
    grid.set_pathway(2, 0, entities[2]);
    grid.set_pathway(3, 0, entities[3]);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    // All connected
    ASSERT_EQ(graph.get_network_count(), 1);

    // But entity IDs preserved
    ASSERT_EQ(grid.get_pathway_at(0, 0), entities[0]);
    ASSERT_EQ(grid.get_pathway_at(1, 0), entities[1]);
    ASSERT_EQ(grid.get_pathway_at(2, 0), entities[2]);
    ASSERT_EQ(grid.get_pathway_at(3, 0), entities[3]);
}

// ============================================================================
// Complex cross-ownership topologies
// ============================================================================

TEST(checkerboard_ownership_connected) {
    // Checkerboard pattern: A B A B / B A B A / ...
    // All adjacent (4-directionally), so all should be one network
    PathwayGrid grid(8, 8);

    for (int32_t y = 0; y < 8; ++y) {
        for (int32_t x = 0; x < 8; ++x) {
            uint32_t base = ((x + y) % 2 == 0) ? PLAYER_A_ENTITY_BASE : PLAYER_B_ENTITY_BASE;
            grid.set_pathway(x, y, base + static_cast<uint32_t>(y * 8 + x));
        }
    }

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 64u);
    ASSERT_EQ(graph.get_network_count(), 1);

    // Corner to corner
    ASSERT(graph.is_connected(GridPosition{0, 0}, GridPosition{7, 7}));
}

TEST(two_player_parallel_lines_with_bridge) {
    PathwayGrid grid(16, 16);

    // Player A: horizontal line at y=3
    for (int32_t x = 0; x < 10; ++x) {
        grid.set_pathway(x, 3, PLAYER_A_ENTITY_BASE + static_cast<uint32_t>(x));
    }

    // Player B: horizontal line at y=5
    for (int32_t x = 0; x < 10; ++x) {
        grid.set_pathway(x, 5, PLAYER_B_ENTITY_BASE + static_cast<uint32_t>(x));
    }

    // Initially two separate networks
    NetworkGraph graph;
    graph.rebuild_from_grid(grid);
    ASSERT_EQ(graph.get_network_count(), 2);

    // Player A builds a bridge tile at (5,4) connecting the two lines
    grid.set_pathway(5, 4, PLAYER_A_ENTITY_BASE + 100);
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.get_network_count(), 1);
    ASSERT(graph.is_connected(GridPosition{0, 3}, GridPosition{9, 5}));
}

// ============================================================================
// Verify rebuild_from_grid doesn't check ownership
// ============================================================================

TEST(rebuild_no_owner_check_wildly_different_ids) {
    // Use wildly different entity IDs to prove no owner-based filtering
    PathwayGrid grid(8, 8);

    grid.set_pathway(0, 0, 1);
    grid.set_pathway(1, 0, 999999);
    grid.set_pathway(2, 0, 42);
    grid.set_pathway(3, 0, UINT32_MAX);

    NetworkGraph graph;
    graph.rebuild_from_grid(grid);

    ASSERT_EQ(graph.node_count(), 4u);
    ASSERT_EQ(graph.get_network_count(), 1);

    // All connected regardless of entity ID values
    ASSERT(graph.is_connected(GridPosition{0, 0}, GridPosition{3, 0}));
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Cross-Ownership Connectivity Tests (Ticket E7-020) ===\n\n");

    // Player A and Player B adjacent pathways: same network
    RUN_TEST(two_players_horizontal_adjacent);
    RUN_TEST(two_players_vertical_adjacent);
    RUN_TEST(two_players_alternating_tiles);

    // Multiple players sharing pathway network: single network_id
    RUN_TEST(three_players_single_network);
    RUN_TEST(four_players_cross_intersection);
    RUN_TEST(many_players_large_network);

    // Separate networks for disconnected players: different network_ids
    RUN_TEST(two_players_disconnected);
    RUN_TEST(same_player_two_disconnected_segments);
    RUN_TEST(three_players_mixed_connectivity);

    // Ownership preserved in RoadComponent (not in graph)
    RUN_TEST(ownership_preserved_in_grid);
    RUN_TEST(road_component_network_id_independent_of_ownership);
    RUN_TEST(entity_ids_remain_distinct_in_grid);

    // Complex cross-ownership topologies
    RUN_TEST(checkerboard_ownership_connected);
    RUN_TEST(two_player_parallel_lines_with_bridge);

    // Verify rebuild doesn't check ownership
    RUN_TEST(rebuild_no_owner_check_wildly_different_ids);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
