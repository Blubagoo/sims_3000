/**
 * @file test_connection_capacity.cpp
 * @brief Unit tests for external connection capacity calculation (Epic 8, Ticket E8-014)
 *
 * Tests cover:
 * - Pathway trade and migration capacity
 * - Rail trade and migration capacity (with bonus)
 * - Energy and Fluid connections produce zero capacity
 * - Rail adjacency bonus applied to pathway connections
 * - Edge cases (multiple calls, field preservation)
 */

#include <sims3000/port/ConnectionCapacity.h>
#include <sims3000/port/ExternalConnectionComponent.h>
#include <sims3000/port/PortTypes.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::port;

// =============================================================================
// Pathway connection tests
// =============================================================================

void test_pathway_trade_capacity() {
    printf("Testing pathway trade capacity...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Pathway;

    calculate_connection_capacity(conn);

    assert(conn.trade_capacity == 100);

    printf("  PASS: Pathway trade capacity = %u (expected 100)\n",
           conn.trade_capacity);
}

void test_pathway_migration_capacity() {
    printf("Testing pathway migration capacity...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Pathway;

    calculate_connection_capacity(conn);

    assert(conn.migration_capacity == 50);

    printf("  PASS: Pathway migration capacity = %u (expected 50)\n",
           conn.migration_capacity);
}

void test_pathway_both_capacities() {
    printf("Testing pathway both capacities together...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Pathway;
    conn.edge_side = MapEdge::East;
    conn.edge_position = 42;

    calculate_connection_capacity(conn);

    assert(conn.trade_capacity == PATHWAY_TRADE_CAPACITY_PER_TILE);
    assert(conn.migration_capacity == PATHWAY_MIGRATION_CAPACITY_PER_TILE);

    printf("  PASS: Pathway trade=%u, migration=%u\n",
           conn.trade_capacity, conn.migration_capacity);
}

// =============================================================================
// Rail connection tests
// =============================================================================

void test_rail_trade_capacity() {
    printf("Testing rail trade capacity...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Rail;

    calculate_connection_capacity(conn);

    // Rail: 100 (base) + 200 (bonus) = 300
    assert(conn.trade_capacity == 300);

    printf("  PASS: Rail trade capacity = %u (expected 300)\n",
           conn.trade_capacity);
}

void test_rail_migration_capacity() {
    printf("Testing rail migration capacity...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Rail;

    calculate_connection_capacity(conn);

    // Rail: 50 (base) + 25 (bonus) = 75
    assert(conn.migration_capacity == 75);

    printf("  PASS: Rail migration capacity = %u (expected 75)\n",
           conn.migration_capacity);
}

void test_rail_both_capacities() {
    printf("Testing rail both capacities together...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Rail;
    conn.edge_side = MapEdge::South;
    conn.edge_position = 100;

    calculate_connection_capacity(conn);

    uint16_t expected_trade = PATHWAY_TRADE_CAPACITY_PER_TILE
                            + RAIL_TRADE_CAPACITY_BONUS;
    uint16_t expected_migration = PATHWAY_MIGRATION_CAPACITY_PER_TILE
                                + RAIL_MIGRATION_CAPACITY_BONUS;

    assert(conn.trade_capacity == expected_trade);
    assert(conn.migration_capacity == expected_migration);
    assert(conn.trade_capacity == 300);
    assert(conn.migration_capacity == 75);

    printf("  PASS: Rail trade=%u, migration=%u\n",
           conn.trade_capacity, conn.migration_capacity);
}

// =============================================================================
// Energy and Fluid connection tests (zero capacity)
// =============================================================================

void test_energy_zero_capacity() {
    printf("Testing energy connection has zero trade/migration capacity...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Energy;
    // Set non-zero values to ensure they get overwritten
    conn.trade_capacity = 999;
    conn.migration_capacity = 999;

    calculate_connection_capacity(conn);

    assert(conn.trade_capacity == 0);
    assert(conn.migration_capacity == 0);

    printf("  PASS: Energy trade=%u, migration=%u\n",
           conn.trade_capacity, conn.migration_capacity);
}

void test_fluid_zero_capacity() {
    printf("Testing fluid connection has zero trade/migration capacity...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Fluid;
    conn.trade_capacity = 999;
    conn.migration_capacity = 999;

    calculate_connection_capacity(conn);

    assert(conn.trade_capacity == 0);
    assert(conn.migration_capacity == 0);

    printf("  PASS: Fluid trade=%u, migration=%u\n",
           conn.trade_capacity, conn.migration_capacity);
}

// =============================================================================
// Rail adjacency bonus tests
// =============================================================================

void test_rail_adjacency_bonus_on_pathway() {
    printf("Testing rail adjacency bonus on pathway connection...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Pathway;

    // First calculate base capacity
    calculate_connection_capacity(conn);
    assert(conn.trade_capacity == 100);
    assert(conn.migration_capacity == 50);

    // Apply rail adjacency bonus
    apply_rail_adjacency_bonus(conn);

    // Pathway + rail bonus: trade = 100 + 200 = 300, migration = 50 + 25 = 75
    assert(conn.trade_capacity == 300);
    assert(conn.migration_capacity == 75);

    printf("  PASS: After rail bonus: trade=%u, migration=%u\n",
           conn.trade_capacity, conn.migration_capacity);
}

void test_rail_adjacency_bonus_ignored_for_rail() {
    printf("Testing rail adjacency bonus ignored for Rail connection...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Rail;

    calculate_connection_capacity(conn);
    uint16_t trade_before = conn.trade_capacity;
    uint16_t migration_before = conn.migration_capacity;

    // Should be a no-op for non-Pathway connections
    apply_rail_adjacency_bonus(conn);

    assert(conn.trade_capacity == trade_before);
    assert(conn.migration_capacity == migration_before);

    printf("  PASS: Rail type unchanged by adjacency bonus\n");
}

void test_rail_adjacency_bonus_ignored_for_energy() {
    printf("Testing rail adjacency bonus ignored for Energy connection...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Energy;

    calculate_connection_capacity(conn);

    apply_rail_adjacency_bonus(conn);

    assert(conn.trade_capacity == 0);
    assert(conn.migration_capacity == 0);

    printf("  PASS: Energy type unchanged by adjacency bonus\n");
}

void test_rail_adjacency_bonus_ignored_for_fluid() {
    printf("Testing rail adjacency bonus ignored for Fluid connection...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Fluid;

    calculate_connection_capacity(conn);

    apply_rail_adjacency_bonus(conn);

    assert(conn.trade_capacity == 0);
    assert(conn.migration_capacity == 0);

    printf("  PASS: Fluid type unchanged by adjacency bonus\n");
}

// =============================================================================
// Field preservation tests
// =============================================================================

void test_calculate_preserves_other_fields() {
    printf("Testing calculate_connection_capacity preserves other fields...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Pathway;
    conn.edge_side = MapEdge::West;
    conn.edge_position = 255;
    conn.is_active = true;
    conn.position = {10, 20};

    calculate_connection_capacity(conn);

    // Only trade_capacity and migration_capacity should change
    assert(conn.connection_type == ConnectionType::Pathway);
    assert(conn.edge_side == MapEdge::West);
    assert(conn.edge_position == 255);
    assert(conn.is_active == true);
    assert(conn.position.x == 10);
    assert(conn.position.y == 20);

    printf("  PASS: Other fields preserved\n");
}

void test_recalculation_overwrites_previous() {
    printf("Testing recalculation overwrites previous values...\n");

    ExternalConnectionComponent conn;
    conn.connection_type = ConnectionType::Rail;

    calculate_connection_capacity(conn);
    assert(conn.trade_capacity == 300);
    assert(conn.migration_capacity == 75);

    // Change type and recalculate
    conn.connection_type = ConnectionType::Pathway;
    calculate_connection_capacity(conn);

    assert(conn.trade_capacity == 100);
    assert(conn.migration_capacity == 50);

    printf("  PASS: Recalculation correctly overwrites\n");
}

// =============================================================================
// Constants verification
// =============================================================================

void test_capacity_constants() {
    printf("Testing capacity constants...\n");

    assert(PATHWAY_TRADE_CAPACITY_PER_TILE == 100);
    assert(PATHWAY_MIGRATION_CAPACITY_PER_TILE == 50);
    assert(RAIL_TRADE_CAPACITY_BONUS == 200);
    assert(RAIL_MIGRATION_CAPACITY_BONUS == 25);

    printf("  PASS: All capacity constants correct\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== External Connection Capacity Tests (Epic 8, Ticket E8-014) ===\n\n");

    // Pathway tests
    test_pathway_trade_capacity();
    test_pathway_migration_capacity();
    test_pathway_both_capacities();

    // Rail tests
    test_rail_trade_capacity();
    test_rail_migration_capacity();
    test_rail_both_capacities();

    // Energy and Fluid tests
    test_energy_zero_capacity();
    test_fluid_zero_capacity();

    // Rail adjacency bonus tests
    test_rail_adjacency_bonus_on_pathway();
    test_rail_adjacency_bonus_ignored_for_rail();
    test_rail_adjacency_bonus_ignored_for_energy();
    test_rail_adjacency_bonus_ignored_for_fluid();

    // Field preservation tests
    test_calculate_preserves_other_fields();
    test_recalculation_overwrites_previous();

    // Constants tests
    test_capacity_constants();

    printf("\n=== All External Connection Capacity Tests Passed ===\n");
    return 0;
}
