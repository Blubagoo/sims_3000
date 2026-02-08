/**
 * @file test_neighbor_generation.cpp
 * @brief Unit tests for NPC neighbor generation (Epic 8, Ticket E8-015)
 *
 * Tests cover:
 * - Generates 1-4 neighbors based on map edges with connections
 * - Each neighbor has unique name and economic factors
 * - Deterministic generation (same seed = same output)
 * - Edge cases: no connections, all edges, single edge
 * - Factor bounds (0.5-1.5)
 * - Neighbor IDs are sequential (1-based)
 */

#include <sims3000/port/NeighborGeneration.h>
#include <sims3000/port/ExternalConnectionComponent.h>
#include <sims3000/port/PortTypes.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <set>
#include <string>

using namespace sims3000::port;

// =============================================================================
// Helper: create an ExternalConnectionComponent on a given edge
// =============================================================================

static ExternalConnectionComponent make_connection(MapEdge edge, uint16_t pos = 10) {
    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Pathway;
    conn.edge_side = edge;
    conn.edge_position = pos;
    conn.is_active = true;
    conn.trade_capacity = 100;
    conn.migration_capacity = 50;
    conn.position = {static_cast<int16_t>(pos), 0};
    return conn;
}

// =============================================================================
// Test: No connections produces no neighbors
// =============================================================================

void test_no_connections() {
    printf("Testing no connections produces no neighbors...\n");

    std::vector<ExternalConnectionComponent> empty;
    auto neighbors = generate_neighbors(empty, 12345);

    assert(neighbors.empty());
    printf("  PASS: 0 connections -> 0 neighbors\n");
}

// =============================================================================
// Test: Single edge connection produces 1 neighbor
// =============================================================================

void test_single_edge() {
    printf("Testing single edge connection produces 1 neighbor...\n");

    std::vector<ExternalConnectionComponent> connections;
    connections.push_back(make_connection(MapEdge::North));

    auto neighbors = generate_neighbors(connections, 42);

    assert(neighbors.size() == 1);
    assert(neighbors[0].neighbor_id == 1);
    assert(neighbors[0].edge == MapEdge::North);
    assert(!neighbors[0].name.empty());
    assert(neighbors[0].relationship == TradeAgreementType::None);

    printf("  PASS: 1 edge -> 1 neighbor (name: \"%s\")\n", neighbors[0].name.c_str());
}

// =============================================================================
// Test: All four edges produces 4 neighbors
// =============================================================================

void test_all_four_edges() {
    printf("Testing all four edges produces 4 neighbors...\n");

    std::vector<ExternalConnectionComponent> connections;
    connections.push_back(make_connection(MapEdge::North));
    connections.push_back(make_connection(MapEdge::East));
    connections.push_back(make_connection(MapEdge::South));
    connections.push_back(make_connection(MapEdge::West));

    auto neighbors = generate_neighbors(connections, 999);

    assert(neighbors.size() == 4);

    // Check IDs are sequential
    for (uint8_t i = 0; i < 4; ++i) {
        assert(neighbors[i].neighbor_id == i + 1);
    }

    // Check edges are assigned (N, E, S, W order)
    assert(neighbors[0].edge == MapEdge::North);
    assert(neighbors[1].edge == MapEdge::East);
    assert(neighbors[2].edge == MapEdge::South);
    assert(neighbors[3].edge == MapEdge::West);

    printf("  PASS: 4 edges -> 4 neighbors with sequential IDs\n");
}

// =============================================================================
// Test: Multiple connections on same edge produce only 1 neighbor
// =============================================================================

void test_multiple_connections_same_edge() {
    printf("Testing multiple connections on same edge produce 1 neighbor...\n");

    std::vector<ExternalConnectionComponent> connections;
    connections.push_back(make_connection(MapEdge::South, 5));
    connections.push_back(make_connection(MapEdge::South, 10));
    connections.push_back(make_connection(MapEdge::South, 15));

    auto neighbors = generate_neighbors(connections, 777);

    assert(neighbors.size() == 1);
    assert(neighbors[0].edge == MapEdge::South);

    printf("  PASS: 3 connections on South -> 1 neighbor\n");
}

// =============================================================================
// Test: Two edges with connections
// =============================================================================

void test_two_edges() {
    printf("Testing two edges with connections...\n");

    std::vector<ExternalConnectionComponent> connections;
    connections.push_back(make_connection(MapEdge::East));
    connections.push_back(make_connection(MapEdge::West));

    auto neighbors = generate_neighbors(connections, 555);

    assert(neighbors.size() == 2);
    assert(neighbors[0].neighbor_id == 1);
    assert(neighbors[0].edge == MapEdge::East);
    assert(neighbors[1].neighbor_id == 2);
    assert(neighbors[1].edge == MapEdge::West);

    printf("  PASS: 2 edges -> 2 neighbors\n");
}

// =============================================================================
// Test: Unique names across all neighbors
// =============================================================================

void test_unique_names() {
    printf("Testing neighbors have unique names...\n");

    std::vector<ExternalConnectionComponent> connections;
    connections.push_back(make_connection(MapEdge::North));
    connections.push_back(make_connection(MapEdge::East));
    connections.push_back(make_connection(MapEdge::South));
    connections.push_back(make_connection(MapEdge::West));

    auto neighbors = generate_neighbors(connections, 12345);

    std::set<std::string> names;
    for (const auto& n : neighbors) {
        assert(!n.name.empty());
        names.insert(n.name);
    }

    // All 4 names must be distinct
    assert(names.size() == 4);

    printf("  PASS: All 4 neighbor names are unique\n");
}

// =============================================================================
// Test: Names come from the pool
// =============================================================================

void test_names_from_pool() {
    printf("Testing neighbor names come from the name pool...\n");

    const std::string* pool = get_neighbor_name_pool();

    std::set<std::string> pool_set;
    for (uint8_t i = 0; i < NEIGHBOR_NAME_POOL_SIZE; ++i) {
        pool_set.insert(pool[i]);
    }

    // Test with several seeds
    for (uint32_t seed = 0; seed < 100; ++seed) {
        std::vector<ExternalConnectionComponent> connections;
        connections.push_back(make_connection(MapEdge::North));
        connections.push_back(make_connection(MapEdge::East));

        auto neighbors = generate_neighbors(connections, seed);
        for (const auto& n : neighbors) {
            assert(pool_set.count(n.name) > 0);
        }
    }

    printf("  PASS: All generated names are from the pool\n");
}

// =============================================================================
// Test: Factor bounds
// =============================================================================

void test_factor_bounds() {
    printf("Testing demand/supply factors are within [0.5, 1.5]...\n");

    // Test with many seeds to exercise the RNG
    for (uint32_t seed = 0; seed < 500; ++seed) {
        std::vector<ExternalConnectionComponent> connections;
        connections.push_back(make_connection(MapEdge::North));
        connections.push_back(make_connection(MapEdge::South));

        auto neighbors = generate_neighbors(connections, seed);
        for (const auto& n : neighbors) {
            assert(n.demand_factor >= NEIGHBOR_FACTOR_MIN - 0.001f);
            assert(n.demand_factor <= NEIGHBOR_FACTOR_MAX + 0.001f);
            assert(n.supply_factor >= NEIGHBOR_FACTOR_MIN - 0.001f);
            assert(n.supply_factor <= NEIGHBOR_FACTOR_MAX + 0.001f);
        }
    }

    printf("  PASS: All factors within [0.5, 1.5] across 500 seeds\n");
}

// =============================================================================
// Test: Deterministic generation (same seed = same result)
// =============================================================================

void test_deterministic() {
    printf("Testing deterministic generation (same seed = same result)...\n");

    std::vector<ExternalConnectionComponent> connections;
    connections.push_back(make_connection(MapEdge::North));
    connections.push_back(make_connection(MapEdge::East));
    connections.push_back(make_connection(MapEdge::South));

    uint32_t seed = 54321;

    auto run1 = generate_neighbors(connections, seed);
    auto run2 = generate_neighbors(connections, seed);

    assert(run1.size() == run2.size());
    for (size_t i = 0; i < run1.size(); ++i) {
        assert(run1[i].neighbor_id == run2[i].neighbor_id);
        assert(run1[i].edge == run2[i].edge);
        assert(run1[i].name == run2[i].name);
        assert(run1[i].demand_factor == run2[i].demand_factor);
        assert(run1[i].supply_factor == run2[i].supply_factor);
        assert(run1[i].relationship == run2[i].relationship);
    }

    printf("  PASS: Two runs with seed %u produce identical results\n", seed);
}

// =============================================================================
// Test: Different seeds produce different results
// =============================================================================

void test_different_seeds() {
    printf("Testing different seeds produce different results...\n");

    std::vector<ExternalConnectionComponent> connections;
    connections.push_back(make_connection(MapEdge::North));
    connections.push_back(make_connection(MapEdge::East));

    auto run1 = generate_neighbors(connections, 100);
    auto run2 = generate_neighbors(connections, 200);

    // At least one field should differ (overwhelmingly likely)
    bool any_diff = false;
    for (size_t i = 0; i < run1.size() && i < run2.size(); ++i) {
        if (run1[i].name != run2[i].name ||
            run1[i].demand_factor != run2[i].demand_factor ||
            run1[i].supply_factor != run2[i].supply_factor) {
            any_diff = true;
            break;
        }
    }
    assert(any_diff);

    printf("  PASS: Seeds 100 and 200 produce different output\n");
}

// =============================================================================
// Test: Initial relationship is None
// =============================================================================

void test_initial_relationship() {
    printf("Testing initial relationship is None...\n");

    std::vector<ExternalConnectionComponent> connections;
    connections.push_back(make_connection(MapEdge::North));
    connections.push_back(make_connection(MapEdge::East));
    connections.push_back(make_connection(MapEdge::South));
    connections.push_back(make_connection(MapEdge::West));

    auto neighbors = generate_neighbors(connections, 11111);

    for (const auto& n : neighbors) {
        assert(n.relationship == TradeAgreementType::None);
    }

    printf("  PASS: All neighbors start with relationship = None\n");
}

// =============================================================================
// Test: Name pool has expected size and content
// =============================================================================

void test_name_pool() {
    printf("Testing name pool...\n");

    const std::string* pool = get_neighbor_name_pool();

    assert(pool[0] == "Settlement Alpha");
    assert(pool[1] == "Nexus Prime");
    assert(pool[2] == "Forge Delta");
    assert(pool[3] == "Haven Epsilon");
    assert(pool[4] == "Citadel Omega");
    assert(pool[5] == "Outpost Sigma");
    assert(pool[6] == "Colony Zeta");
    assert(pool[7] == "Bastion Theta");

    printf("  PASS: Name pool has 8 expected entries\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== NPC Neighbor Generation Tests (Epic 8, Ticket E8-015) ===\n\n");

    test_no_connections();
    test_single_edge();
    test_all_four_edges();
    test_multiple_connections_same_edge();
    test_two_edges();
    test_unique_names();
    test_names_from_pool();
    test_factor_bounds();
    test_deterministic();
    test_different_seeds();
    test_initial_relationship();
    test_name_pool();

    printf("\n=== All NPC Neighbor Generation Tests Passed ===\n");
    return 0;
}
