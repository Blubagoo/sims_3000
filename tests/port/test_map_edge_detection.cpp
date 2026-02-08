/**
 * @file test_map_edge_detection.cpp
 * @brief Unit tests for MapEdgeDetection (Epic 8, Ticket E8-013)
 *
 * Tests cover:
 * - is_map_edge: edge and non-edge tiles, corners, degenerate maps
 * - get_edge: all four edges, corner priority
 * - scan_map_edges_for_connections: pathway detection, rail detection,
 *   both on same tile, empty map, all edges populated, edge_position values,
 *   updates when infrastructure changes
 */

#include <sims3000/port/MapEdgeDetection.h>
#include <sims3000/transport/PathwayGrid.h>
#include <sims3000/transport/RailSystem.h>
#include <cassert>
#include <cstdio>
#include <algorithm>

using namespace sims3000::port;
using namespace sims3000::transport;
using namespace sims3000;

// =============================================================================
// is_map_edge tests
// =============================================================================

void test_is_map_edge_corners() {
    printf("Testing is_map_edge corners...\n");

    // 10x10 map
    assert(is_map_edge(0, 0, 10, 10) == true);   // top-left
    assert(is_map_edge(9, 0, 10, 10) == true);   // top-right
    assert(is_map_edge(0, 9, 10, 10) == true);   // bottom-left
    assert(is_map_edge(9, 9, 10, 10) == true);   // bottom-right

    printf("  PASS: Corners are map edges\n");
}

void test_is_map_edge_edges() {
    printf("Testing is_map_edge edges...\n");

    // 10x10 map
    assert(is_map_edge(5, 0, 10, 10) == true);   // north
    assert(is_map_edge(5, 9, 10, 10) == true);   // south
    assert(is_map_edge(0, 5, 10, 10) == true);   // west
    assert(is_map_edge(9, 5, 10, 10) == true);   // east

    printf("  PASS: Edge tiles are map edges\n");
}

void test_is_map_edge_interior() {
    printf("Testing is_map_edge interior...\n");

    // 10x10 map
    assert(is_map_edge(1, 1, 10, 10) == false);
    assert(is_map_edge(5, 5, 10, 10) == false);
    assert(is_map_edge(8, 8, 10, 10) == false);
    assert(is_map_edge(3, 7, 10, 10) == false);

    printf("  PASS: Interior tiles are not map edges\n");
}

void test_is_map_edge_1x1() {
    printf("Testing is_map_edge 1x1 map...\n");

    // 1x1 map: the only tile is an edge
    assert(is_map_edge(0, 0, 1, 1) == true);

    printf("  PASS: Single tile in 1x1 map is an edge\n");
}

void test_is_map_edge_2x2() {
    printf("Testing is_map_edge 2x2 map...\n");

    // 2x2 map: all tiles are edges
    assert(is_map_edge(0, 0, 2, 2) == true);
    assert(is_map_edge(1, 0, 2, 2) == true);
    assert(is_map_edge(0, 1, 2, 2) == true);
    assert(is_map_edge(1, 1, 2, 2) == true);

    printf("  PASS: All tiles in 2x2 map are edges\n");
}

void test_is_map_edge_degenerate() {
    printf("Testing is_map_edge degenerate maps...\n");

    // 0-width or 0-height
    assert(is_map_edge(0, 0, 0, 0) == false);
    assert(is_map_edge(0, 0, 0, 10) == false);
    assert(is_map_edge(0, 0, 10, 0) == false);

    printf("  PASS: Degenerate maps return false\n");
}

// =============================================================================
// get_edge tests
// =============================================================================

void test_get_edge_north() {
    printf("Testing get_edge north...\n");

    assert(get_edge(5, 0, 10, 10) == MapEdge::North);

    printf("  PASS: y=0 returns North\n");
}

void test_get_edge_south() {
    printf("Testing get_edge south...\n");

    assert(get_edge(5, 9, 10, 10) == MapEdge::South);

    printf("  PASS: y=height-1 returns South\n");
}

void test_get_edge_west() {
    printf("Testing get_edge west...\n");

    assert(get_edge(0, 5, 10, 10) == MapEdge::West);

    printf("  PASS: x=0 (non-corner) returns West\n");
}

void test_get_edge_east() {
    printf("Testing get_edge east...\n");

    assert(get_edge(9, 5, 10, 10) == MapEdge::East);

    printf("  PASS: x=width-1 (non-corner) returns East\n");
}

void test_get_edge_corner_priority() {
    printf("Testing get_edge corner priority...\n");

    // North corners: y=0 takes priority
    assert(get_edge(0, 0, 10, 10) == MapEdge::North);   // NW corner
    assert(get_edge(9, 0, 10, 10) == MapEdge::North);   // NE corner

    // South corners: y=height-1 takes priority over x edges
    assert(get_edge(0, 9, 10, 10) == MapEdge::South);   // SW corner
    assert(get_edge(9, 9, 10, 10) == MapEdge::South);   // SE corner

    printf("  PASS: Corner priority is North > South > West > East\n");
}

// =============================================================================
// scan_map_edges_for_connections tests
// =============================================================================

void test_scan_empty_map() {
    printf("Testing scan on empty map (no infrastructure)...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    assert(connections.empty());

    printf("  PASS: No connections on empty map\n");
}

void test_scan_pathway_north_edge() {
    printf("Testing scan: pathway on north edge...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // Place pathway at (3, 0) - north edge
    grid.set_pathway(3, 0, 100);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    assert(connections.size() == 1);
    assert(connections[0].connection_type == ConnectionType::Pathway);
    assert(connections[0].edge_side == MapEdge::North);
    assert(connections[0].edge_position == 3);
    assert(connections[0].is_active == true);
    assert(connections[0].position.x == 3);
    assert(connections[0].position.y == 0);

    printf("  PASS: Pathway detected on north edge\n");
}

void test_scan_pathway_south_edge() {
    printf("Testing scan: pathway on south edge...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // Place pathway at (5, 7) - south edge
    grid.set_pathway(5, 7, 200);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    assert(connections.size() == 1);
    assert(connections[0].connection_type == ConnectionType::Pathway);
    assert(connections[0].edge_side == MapEdge::South);
    assert(connections[0].edge_position == 5);
    assert(connections[0].position.x == 5);
    assert(connections[0].position.y == 7);

    printf("  PASS: Pathway detected on south edge\n");
}

void test_scan_pathway_west_edge() {
    printf("Testing scan: pathway on west edge...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // Place pathway at (0, 4) - west edge (not corner)
    grid.set_pathway(0, 4, 300);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    assert(connections.size() == 1);
    assert(connections[0].connection_type == ConnectionType::Pathway);
    assert(connections[0].edge_side == MapEdge::West);
    assert(connections[0].edge_position == 4);
    assert(connections[0].position.x == 0);
    assert(connections[0].position.y == 4);

    printf("  PASS: Pathway detected on west edge\n");
}

void test_scan_pathway_east_edge() {
    printf("Testing scan: pathway on east edge...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // Place pathway at (7, 3) - east edge (not corner)
    grid.set_pathway(7, 3, 400);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    assert(connections.size() == 1);
    assert(connections[0].connection_type == ConnectionType::Pathway);
    assert(connections[0].edge_side == MapEdge::East);
    assert(connections[0].edge_position == 3);
    assert(connections[0].position.x == 7);
    assert(connections[0].position.y == 3);

    printf("  PASS: Pathway detected on east edge\n");
}

void test_scan_rail_north_edge() {
    printf("Testing scan: rail on north edge...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // Place rail at (2, 0) - north edge
    rail.place_rail(2, 0, RailType::SurfaceRail, 0);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    assert(connections.size() == 1);
    assert(connections[0].connection_type == ConnectionType::Rail);
    assert(connections[0].edge_side == MapEdge::North);
    assert(connections[0].edge_position == 2);
    assert(connections[0].is_active == true);
    assert(connections[0].position.x == 2);
    assert(connections[0].position.y == 0);

    printf("  PASS: Rail detected on north edge\n");
}

void test_scan_rail_south_edge() {
    printf("Testing scan: rail on south edge...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // Place rail at (6, 7) - south edge
    rail.place_rail(6, 7, RailType::SurfaceRail, 0);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    assert(connections.size() == 1);
    assert(connections[0].connection_type == ConnectionType::Rail);
    assert(connections[0].edge_side == MapEdge::South);
    assert(connections[0].edge_position == 6);
    assert(connections[0].position.x == 6);
    assert(connections[0].position.y == 7);

    printf("  PASS: Rail detected on south edge\n");
}

void test_scan_both_pathway_and_rail_same_tile() {
    printf("Testing scan: both pathway and rail on same edge tile...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // Place both at (4, 0) - north edge
    grid.set_pathway(4, 0, 500);
    rail.place_rail(4, 0, RailType::SurfaceRail, 0);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    assert(connections.size() == 2);

    // Should have one pathway and one rail
    bool has_pathway = false;
    bool has_rail = false;
    for (const auto& conn : connections) {
        if (conn.connection_type == ConnectionType::Pathway) has_pathway = true;
        if (conn.connection_type == ConnectionType::Rail) has_rail = true;
        assert(conn.edge_side == MapEdge::North);
        assert(conn.edge_position == 4);
        assert(conn.position.x == 4);
        assert(conn.position.y == 0);
    }
    assert(has_pathway);
    assert(has_rail);

    printf("  PASS: Both pathway and rail detected on same tile\n");
}

void test_scan_interior_pathway_ignored() {
    printf("Testing scan: interior pathway not detected...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // Place pathway in interior
    grid.set_pathway(4, 4, 600);
    grid.set_pathway(2, 3, 601);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    assert(connections.empty());

    printf("  PASS: Interior pathways are not detected\n");
}

void test_scan_interior_rail_ignored() {
    printf("Testing scan: interior rail not detected...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // Place rail in interior
    rail.place_rail(3, 3, RailType::SurfaceRail, 0);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    assert(connections.empty());

    printf("  PASS: Interior rails are not detected\n");
}

void test_scan_multiple_edges() {
    printf("Testing scan: connections on multiple edges...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // North pathway at (1, 0)
    grid.set_pathway(1, 0, 100);
    // South rail at (3, 7)
    rail.place_rail(3, 7, RailType::SurfaceRail, 0);
    // West pathway at (0, 4)
    grid.set_pathway(0, 4, 200);
    // East rail at (7, 5)
    rail.place_rail(7, 5, RailType::SurfaceRail, 1);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    assert(connections.size() == 4);

    // Verify each edge is represented
    int north_count = 0, south_count = 0, west_count = 0, east_count = 0;
    for (const auto& conn : connections) {
        switch (conn.edge_side) {
            case MapEdge::North: ++north_count; break;
            case MapEdge::South: ++south_count; break;
            case MapEdge::West: ++west_count; break;
            case MapEdge::East: ++east_count; break;
        }
    }
    assert(north_count == 1);
    assert(south_count == 1);
    assert(west_count == 1);
    assert(east_count == 1);

    printf("  PASS: Connections detected on all four edges\n");
}

void test_scan_corner_tile_classification() {
    printf("Testing scan: corner tile edge classification...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // Place pathway at (0, 0) - NW corner
    grid.set_pathway(0, 0, 700);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    // Corner (0,0) is scanned as part of North edge (y=0),
    // so it should appear exactly once with MapEdge::North
    assert(connections.size() == 1);
    assert(connections[0].edge_side == MapEdge::North);
    assert(connections[0].position.x == 0);
    assert(connections[0].position.y == 0);

    printf("  PASS: Corner tile classified correctly (no duplicates)\n");
}

void test_scan_appends_to_vector() {
    printf("Testing scan: appends to existing vector...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    grid.set_pathway(2, 0, 100);

    // Pre-populate the vector
    std::vector<ExternalConnectionComponent> connections;
    ExternalConnectionComponent existing{};
    existing.connection_type = ConnectionType::Energy;
    connections.push_back(existing);

    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    assert(connections.size() == 2);
    assert(connections[0].connection_type == ConnectionType::Energy); // original
    assert(connections[1].connection_type == ConnectionType::Pathway); // scanned

    printf("  PASS: Scan appends to existing vector contents\n");
}

void test_scan_updates_when_infrastructure_changes() {
    printf("Testing scan: updates when infrastructure changes...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // Initial: one pathway on north edge
    grid.set_pathway(3, 0, 100);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);
    assert(connections.size() == 1);

    // Add a rail on south edge
    rail.place_rail(5, 7, RailType::SurfaceRail, 0);

    // Re-scan (fresh vector)
    connections.clear();
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);
    assert(connections.size() == 2);

    // Remove the pathway
    grid.clear_pathway(3, 0);

    // Re-scan
    connections.clear();
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);
    assert(connections.size() == 1);
    assert(connections[0].connection_type == ConnectionType::Rail);

    printf("  PASS: Scan reflects infrastructure changes\n");
}

void test_scan_zero_size_map() {
    printf("Testing scan: zero-size map...\n");

    PathwayGrid grid;  // default 0x0
    RailSystem rail(0, 0);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 0, 0, connections);
    assert(connections.empty());

    printf("  PASS: Zero-size map produces no connections\n");
}

void test_scan_all_edge_tiles_pathway() {
    printf("Testing scan: all edge tiles have pathways on 4x4 map...\n");

    PathwayGrid grid(4, 4);
    RailSystem rail(4, 4);

    uint32_t entity = 1;
    // North: y=0, x=0..3
    for (int32_t x = 0; x < 4; ++x) grid.set_pathway(x, 0, entity++);
    // South: y=3, x=0..3
    for (int32_t x = 0; x < 4; ++x) grid.set_pathway(x, 3, entity++);
    // West: x=0, y=1..2
    for (int32_t y = 1; y < 3; ++y) grid.set_pathway(0, y, entity++);
    // East: x=3, y=1..2
    for (int32_t y = 1; y < 3; ++y) grid.set_pathway(3, y, entity++);

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 4, 4, connections);

    // 4x4 map perimeter = 4+4+2+2 = 12 tiles
    assert(connections.size() == 12);

    // All should be pathways and active
    for (const auto& conn : connections) {
        assert(conn.connection_type == ConnectionType::Pathway);
        assert(conn.is_active == true);
    }

    printf("  PASS: All 12 edge tiles detected on 4x4 map\n");
}

void test_scan_edge_position_values() {
    printf("Testing scan: edge_position values correct...\n");

    PathwayGrid grid(8, 8);
    RailSystem rail(8, 8);

    // Place pathways at specific positions on each edge
    grid.set_pathway(0, 0, 1);  // North, edge_position=0
    grid.set_pathway(7, 0, 2);  // North, edge_position=7
    grid.set_pathway(0, 3, 3);  // West, edge_position=3
    grid.set_pathway(7, 6, 4);  // East, edge_position=6

    std::vector<ExternalConnectionComponent> connections;
    scan_map_edges_for_connections(grid, rail, 8, 8, connections);

    // Find each connection and verify edge_position
    for (const auto& conn : connections) {
        if (conn.edge_side == MapEdge::North && conn.position.x == 0) {
            assert(conn.edge_position == 0);
        }
        if (conn.edge_side == MapEdge::North && conn.position.x == 7) {
            assert(conn.edge_position == 7);
        }
        if (conn.edge_side == MapEdge::West && conn.position.y == 3) {
            assert(conn.edge_position == 3);
        }
        if (conn.edge_side == MapEdge::East && conn.position.y == 6) {
            assert(conn.edge_position == 6);
        }
    }

    printf("  PASS: Edge position values are correct\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== MapEdgeDetection Unit Tests (Epic 8, Ticket E8-013) ===\n\n");

    // is_map_edge tests
    test_is_map_edge_corners();
    test_is_map_edge_edges();
    test_is_map_edge_interior();
    test_is_map_edge_1x1();
    test_is_map_edge_2x2();
    test_is_map_edge_degenerate();

    // get_edge tests
    test_get_edge_north();
    test_get_edge_south();
    test_get_edge_west();
    test_get_edge_east();
    test_get_edge_corner_priority();

    // scan tests
    test_scan_empty_map();
    test_scan_pathway_north_edge();
    test_scan_pathway_south_edge();
    test_scan_pathway_west_edge();
    test_scan_pathway_east_edge();
    test_scan_rail_north_edge();
    test_scan_rail_south_edge();
    test_scan_both_pathway_and_rail_same_tile();
    test_scan_interior_pathway_ignored();
    test_scan_interior_rail_ignored();
    test_scan_multiple_edges();
    test_scan_corner_tile_classification();
    test_scan_appends_to_vector();
    test_scan_updates_when_infrastructure_changes();
    test_scan_zero_size_map();
    test_scan_all_edge_tiles_pathway();
    test_scan_edge_position_values();

    printf("\n=== All MapEdgeDetection Tests Passed ===\n");
    return 0;
}
