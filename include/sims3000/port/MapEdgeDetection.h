/**
 * @file MapEdgeDetection.h
 * @brief Map edge detection for external connections (Epic 8, Ticket E8-013)
 *
 * Detects when pathways or rails reach the map edge and creates
 * ExternalConnectionComponent entries for each detected connection point.
 *
 * Scans all four map edges (North, East, South, West) for pathway tiles
 * (via PathwayGrid) and rail tiles (via RailSystem), creating properly
 * configured ExternalConnectionComponent instances for each.
 */

#pragma once

#include <sims3000/port/ExternalConnectionComponent.h>
#include <sims3000/port/PortTypes.h>
#include <cstdint>
#include <vector>

// Forward declarations
namespace sims3000 {
namespace transport {
class PathwayGrid;
class RailSystem;
} // namespace transport
} // namespace sims3000

namespace sims3000 {
namespace port {

/**
 * @brief Scan all four map edges for pathway and rail connections.
 *
 * Iterates over the perimeter tiles of the map (x=0, x=width-1,
 * y=0, y=height-1) and checks for pathway tiles via PathwayGrid
 * and rail tiles via RailSystem. For each detected infrastructure
 * tile at a map edge, creates an ExternalConnectionComponent with:
 * - Correct ConnectionType (Pathway or Rail)
 * - Correct MapEdge (North, East, South, West)
 * - edge_position set to the position along that edge
 * - GridPosition set to the tile coordinates
 * - is_active = true
 *
 * Results are appended to out_connections (vector is NOT cleared first).
 *
 * @param pathway_grid The pathway grid to query for pathway tiles.
 * @param rail_system The rail system to query for rail tiles.
 * @param map_width Map width in tiles.
 * @param map_height Map height in tiles.
 * @param out_connections Output vector; detected connections are appended.
 */
void scan_map_edges_for_connections(const transport::PathwayGrid& pathway_grid,
                                    const transport::RailSystem& rail_system,
                                    int32_t map_width, int32_t map_height,
                                    std::vector<ExternalConnectionComponent>& out_connections);

/**
 * @brief Check if a tile coordinate is on the map edge.
 *
 * A tile is on the map edge if x=0, x=width-1, y=0, or y=height-1.
 *
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param map_width Map width in tiles.
 * @param map_height Map height in tiles.
 * @return true if the tile is on any map edge.
 */
bool is_map_edge(int32_t x, int32_t y, int32_t map_width, int32_t map_height);

/**
 * @brief Determine which map edge a tile is on.
 *
 * For corner tiles, priority order is: North > South > West > East.
 * Behavior is undefined if the tile is not on a map edge;
 * call is_map_edge() first.
 *
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param map_width Map width in tiles.
 * @param map_height Map height in tiles.
 * @return The MapEdge the tile is on.
 */
MapEdge get_edge(int32_t x, int32_t y, int32_t map_width, int32_t map_height);

} // namespace port
} // namespace sims3000
