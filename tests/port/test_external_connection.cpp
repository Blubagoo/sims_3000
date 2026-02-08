/**
 * @file test_external_connection.cpp
 * @brief Unit tests for ExternalConnectionComponent (Epic 8, Ticket E8-004)
 *
 * Tests cover:
 * - Component size (16 bytes)
 * - Trivially copyable requirement
 * - Default initialization values
 * - Custom value assignment
 * - All four map edges
 * - All connection types
 * - Trade and migration capacity tracking
 * - GridPosition integration
 * - Copy semantics
 */

#include <sims3000/port/ExternalConnectionComponent.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::port;
using namespace sims3000;

void test_external_connection_size() {
    printf("Testing ExternalConnectionComponent size...\n");

    assert(sizeof(ExternalConnectionComponent) == 16);

    printf("  PASS: ExternalConnectionComponent is 16 bytes\n");
}

void test_external_connection_trivially_copyable() {
    printf("Testing ExternalConnectionComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<ExternalConnectionComponent>::value);

    printf("  PASS: ExternalConnectionComponent is trivially copyable\n");
}

void test_external_connection_default_initialization() {
    printf("Testing default initialization...\n");

    ExternalConnectionComponent conn{};
    assert(conn.connection_type == ConnectionType::Pathway);
    assert(conn.edge_side == MapEdge::North);
    assert(conn.edge_position == 0);
    assert(conn.is_active == false);
    assert(conn.padding1 == 0);
    assert(conn.trade_capacity == 0);
    assert(conn.migration_capacity == 0);
    assert(conn.padding2 == 0);
    assert(conn.position.x == 0);
    assert(conn.position.y == 0);

    printf("  PASS: Default initialization works correctly\n");
}

void test_external_connection_custom_values() {
    printf("Testing custom value assignment...\n");

    ExternalConnectionComponent conn{};
    conn.connection_type = ConnectionType::Rail;
    conn.edge_side = MapEdge::East;
    conn.edge_position = 128;
    conn.is_active = true;
    conn.trade_capacity = 500;
    conn.migration_capacity = 200;
    conn.position = {10, 20};

    assert(conn.connection_type == ConnectionType::Rail);
    assert(conn.edge_side == MapEdge::East);
    assert(conn.edge_position == 128);
    assert(conn.is_active == true);
    assert(conn.trade_capacity == 500);
    assert(conn.migration_capacity == 200);
    assert(conn.position.x == 10);
    assert(conn.position.y == 20);

    printf("  PASS: Custom value assignment works correctly\n");
}

void test_external_connection_all_edges() {
    printf("Testing all four map edges...\n");

    ExternalConnectionComponent conn{};

    conn.edge_side = MapEdge::North;
    assert(conn.edge_side == MapEdge::North);

    conn.edge_side = MapEdge::East;
    assert(conn.edge_side == MapEdge::East);

    conn.edge_side = MapEdge::South;
    assert(conn.edge_side == MapEdge::South);

    conn.edge_side = MapEdge::West;
    assert(conn.edge_side == MapEdge::West);

    printf("  PASS: All four map edges can be assigned\n");
}

void test_external_connection_all_types() {
    printf("Testing all connection types...\n");

    ExternalConnectionComponent conn{};

    conn.connection_type = ConnectionType::Pathway;
    assert(conn.connection_type == ConnectionType::Pathway);

    conn.connection_type = ConnectionType::Rail;
    assert(conn.connection_type == ConnectionType::Rail);

    conn.connection_type = ConnectionType::Energy;
    assert(conn.connection_type == ConnectionType::Energy);

    conn.connection_type = ConnectionType::Fluid;
    assert(conn.connection_type == ConnectionType::Fluid);

    printf("  PASS: All connection types can be assigned\n");
}

void test_external_connection_capacity_tracking() {
    printf("Testing trade and migration capacity tracking...\n");

    ExternalConnectionComponent conn{};

    // Zero capacity (default)
    assert(conn.trade_capacity == 0);
    assert(conn.migration_capacity == 0);

    // Set trade capacity
    conn.trade_capacity = 1000;
    assert(conn.trade_capacity == 1000);

    // Set migration capacity
    conn.migration_capacity = 500;
    assert(conn.migration_capacity == 500);

    // Max capacity (uint16_t max)
    conn.trade_capacity = 65535;
    conn.migration_capacity = 65535;
    assert(conn.trade_capacity == 65535);
    assert(conn.migration_capacity == 65535);

    printf("  PASS: Trade and migration capacity tracking works correctly\n");
}

void test_external_connection_edge_position() {
    printf("Testing edge position values...\n");

    ExternalConnectionComponent conn{};

    // Position at start of edge
    conn.edge_position = 0;
    assert(conn.edge_position == 0);

    // Position along edge
    conn.edge_position = 255;
    assert(conn.edge_position == 255);

    // Max position (uint16_t max)
    conn.edge_position = 65535;
    assert(conn.edge_position == 65535);

    printf("  PASS: Edge position values work correctly\n");
}

void test_external_connection_grid_position() {
    printf("Testing GridPosition integration...\n");

    ExternalConnectionComponent conn{};

    // Positive coordinates
    conn.position = {100, 200};
    assert(conn.position.x == 100);
    assert(conn.position.y == 200);

    // Negative coordinates (supported by int16_t)
    conn.position = {-1, -1};
    assert(conn.position.x == -1);
    assert(conn.position.y == -1);

    // Edge of map
    conn.position = {511, 511};
    assert(conn.position.x == 511);
    assert(conn.position.y == 511);

    printf("  PASS: GridPosition integration works correctly\n");
}

void test_external_connection_copy() {
    printf("Testing copy semantics...\n");

    ExternalConnectionComponent original{};
    original.connection_type = ConnectionType::Energy;
    original.edge_side = MapEdge::South;
    original.edge_position = 64;
    original.is_active = true;
    original.trade_capacity = 750;
    original.migration_capacity = 300;
    original.position = {42, 84};

    ExternalConnectionComponent copy = original;
    assert(copy.connection_type == ConnectionType::Energy);
    assert(copy.edge_side == MapEdge::South);
    assert(copy.edge_position == 64);
    assert(copy.is_active == true);
    assert(copy.trade_capacity == 750);
    assert(copy.migration_capacity == 300);
    assert(copy.position.x == 42);
    assert(copy.position.y == 84);

    printf("  PASS: Copy semantics work correctly\n");
}

void test_external_connection_activation() {
    printf("Testing activation state...\n");

    ExternalConnectionComponent conn{};
    assert(conn.is_active == false);

    conn.is_active = true;
    assert(conn.is_active == true);

    conn.is_active = false;
    assert(conn.is_active == false);

    printf("  PASS: Activation state works correctly\n");
}

int main() {
    printf("=== ExternalConnectionComponent Unit Tests (Epic 8, Ticket E8-004) ===\n\n");

    test_external_connection_size();
    test_external_connection_trivially_copyable();
    test_external_connection_default_initialization();
    test_external_connection_custom_values();
    test_external_connection_all_edges();
    test_external_connection_all_types();
    test_external_connection_capacity_tracking();
    test_external_connection_edge_position();
    test_external_connection_grid_position();
    test_external_connection_copy();
    test_external_connection_activation();

    printf("\n=== All ExternalConnectionComponent Tests Passed ===\n");
    return 0;
}
