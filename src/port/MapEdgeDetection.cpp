/**
 * @file MapEdgeDetection.cpp
 * @brief Map edge detection implementation (Epic 8, Ticket E8-013)
 *
 * Scans all four map edges for pathway and rail tiles, creating
 * ExternalConnectionComponent entries for each detected connection.
 */

#include <sims3000/port/MapEdgeDetection.h>
#include <sims3000/transport/PathwayGrid.h>
#include <sims3000/transport/RailSystem.h>

namespace sims3000 {
namespace port {

// =============================================================================
// Helper: create an ExternalConnectionComponent for a detected edge tile
// =============================================================================

static ExternalConnectionComponent make_connection(
    ConnectionType type, MapEdge edge, uint16_t edge_position,
    int16_t grid_x, int16_t grid_y)
{
    ExternalConnectionComponent conn{};
    conn.connection_type = type;
    conn.edge_side = edge;
    conn.edge_position = edge_position;
    conn.is_active = true;
    conn.trade_capacity = 0;
    conn.migration_capacity = 0;
    conn.position = { grid_x, grid_y };
    return conn;
}

// =============================================================================
// Helper: scan a single edge tile for pathway and rail presence
// =============================================================================

static void check_tile(const transport::PathwayGrid& pathway_grid,
                        const transport::RailSystem& rail_system,
                        int32_t x, int32_t y,
                        MapEdge edge, uint16_t edge_position,
                        std::vector<ExternalConnectionComponent>& out)
{
    if (pathway_grid.has_pathway(x, y)) {
        out.push_back(make_connection(
            ConnectionType::Pathway, edge, edge_position,
            static_cast<int16_t>(x), static_cast<int16_t>(y)));
    }

    if (rail_system.has_rail_at(x, y)) {
        out.push_back(make_connection(
            ConnectionType::Rail, edge, edge_position,
            static_cast<int16_t>(x), static_cast<int16_t>(y)));
    }
}

// =============================================================================
// scan_map_edges_for_connections
// =============================================================================

void scan_map_edges_for_connections(const transport::PathwayGrid& pathway_grid,
                                    const transport::RailSystem& rail_system,
                                    int32_t map_width, int32_t map_height,
                                    std::vector<ExternalConnectionComponent>& out_connections)
{
    if (map_width <= 0 || map_height <= 0) {
        return;
    }

    // North edge: y=0, x=0..width-1
    for (int32_t x = 0; x < map_width; ++x) {
        check_tile(pathway_grid, rail_system, x, 0,
                   MapEdge::North, static_cast<uint16_t>(x), out_connections);
    }

    // South edge: y=height-1, x=0..width-1
    for (int32_t x = 0; x < map_width; ++x) {
        check_tile(pathway_grid, rail_system, x, map_height - 1,
                   MapEdge::South, static_cast<uint16_t>(x), out_connections);
    }

    // West edge: x=0, y=1..height-2 (corners already covered by North/South)
    for (int32_t y = 1; y < map_height - 1; ++y) {
        check_tile(pathway_grid, rail_system, 0, y,
                   MapEdge::West, static_cast<uint16_t>(y), out_connections);
    }

    // East edge: x=width-1, y=1..height-2 (corners already covered by North/South)
    for (int32_t y = 1; y < map_height - 1; ++y) {
        check_tile(pathway_grid, rail_system, map_width - 1, y,
                   MapEdge::East, static_cast<uint16_t>(y), out_connections);
    }
}

// =============================================================================
// is_map_edge
// =============================================================================

bool is_map_edge(int32_t x, int32_t y, int32_t map_width, int32_t map_height) {
    if (map_width <= 0 || map_height <= 0) {
        return false;
    }
    return (x == 0 || x == map_width - 1 || y == 0 || y == map_height - 1);
}

// =============================================================================
// get_edge
// =============================================================================

MapEdge get_edge(int32_t x, int32_t y, int32_t map_width, int32_t map_height) {
    // Priority: North > South > West > East
    if (y == 0) {
        return MapEdge::North;
    }
    if (y == map_height - 1) {
        return MapEdge::South;
    }
    if (x == 0) {
        return MapEdge::West;
    }
    // x == map_width - 1 (or fallback)
    (void)map_width;
    return MapEdge::East;
}

} // namespace port
} // namespace sims3000
