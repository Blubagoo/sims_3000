/**
 * @file ConnectivityQuery.h
 * @brief O(1) connectivity query utility (Epic 7, Ticket E7-011)
 *
 * Provides O(1) connectivity checks between positions using the
 * network_id assigned by NetworkGraph's connected component BFS.
 *
 * Query flow:
 * 1. Look up position in PathwayGrid to check if pathway exists
 * 2. Look up network_id in NetworkGraph for each position
 * 3. Compare network_ids (same non-zero ID = connected)
 *
 * @see NetworkGraph.h (network_id assignment via BFS)
 * @see PathwayGrid.h (spatial pathway lookup)
 * @see /docs/epics/epic-7/tickets.md (ticket E7-011)
 */

#pragma once

#include <sims3000/transport/PathwayGrid.h>
#include <sims3000/transport/NetworkGraph.h>
#include <cstdint>

namespace sims3000 {
namespace transport {

/**
 * @class ConnectivityQuery
 * @brief Utility for O(1) connectivity checks using NetworkGraph network_ids.
 *
 * After NetworkGraph::assign_network_ids() or rebuild_from_grid(), this class
 * provides O(1) queries to determine if two positions are on the same
 * connected road network component.
 */
class ConnectivityQuery {
public:
    ConnectivityQuery() = default;

    /**
     * @brief O(1) check: are two positions on the same network?
     *
     * Looks up both positions in PathwayGrid to verify they have pathways,
     * then compares their network_ids from NetworkGraph.
     *
     * @param grid The PathwayGrid for spatial pathway lookup.
     * @param graph The NetworkGraph with assigned network_ids.
     * @param x1 X coordinate of first position.
     * @param y1 Y coordinate of first position.
     * @param x2 X coordinate of second position.
     * @param y2 Y coordinate of second position.
     * @return true if both positions have pathways and share the same non-zero network_id.
     */
    bool is_connected(const PathwayGrid& grid, const NetworkGraph& graph,
                      int32_t x1, int32_t y1, int32_t x2, int32_t y2) const;

    /**
     * @brief Check if position has any pathway network.
     *
     * @param grid The PathwayGrid for spatial pathway lookup.
     * @param graph The NetworkGraph with assigned network_ids.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return true if position has a pathway with a non-zero network_id.
     */
    bool is_on_network(const PathwayGrid& grid, const NetworkGraph& graph,
                       int32_t x, int32_t y) const;

    /**
     * @brief Get network_id at position (0 = no network).
     *
     * @param grid The PathwayGrid for spatial pathway lookup.
     * @param graph The NetworkGraph with assigned network_ids.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return The network_id at the position, or 0 if no pathway exists there.
     */
    uint16_t get_network_id_at(const PathwayGrid& grid, const NetworkGraph& graph,
                                int32_t x, int32_t y) const;
};

} // namespace transport
} // namespace sims3000
