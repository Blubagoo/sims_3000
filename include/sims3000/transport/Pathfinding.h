/**
 * @file Pathfinding.h
 * @brief A* pathfinding for transport network (Epic 7, Ticket E7-023)
 *
 * Provides A* pathfinding on the PathwayGrid with Manhattan distance heuristic.
 * Uses NetworkGraph for early exit via connected component (network_id) check.
 *
 * Performance target: <5ms per path on 512x512 grid.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/NetworkGraph.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace transport {

class PathwayGrid;  // forward declaration

/**
 * @struct PathResult
 * @brief Result of a pathfinding query.
 */
struct PathResult {
    bool found = false;                  ///< true if a path was found
    std::vector<GridPosition> path;      ///< Sequence of positions from start to end
    uint32_t total_cost = 0;             ///< Total path cost
};

/**
 * @class Pathfinding
 * @brief A* pathfinding on the transport pathway grid.
 *
 * Finds shortest paths between pathway tiles using A* with Manhattan
 * distance heuristic and basic edge cost (10 per step).
 *
 * Early exit: if start and end are on different network_ids (connected
 * components), returns immediately with found=false.
 */
class Pathfinding {
public:
    Pathfinding() = default;

    /**
     * @brief Find shortest path using A* with Manhattan heuristic.
     *
     * Algorithm:
     * 1. Validate start/end are pathway tiles
     * 2. Early exit if different network_id (not connected)
     * 3. A* search with open set (priority queue) and closed set
     * 4. Reconstruct path on success
     *
     * @param start Starting grid position (must be a pathway tile).
     * @param end   Ending grid position (must be a pathway tile).
     * @param grid  PathwayGrid for neighbor lookups.
     * @param graph NetworkGraph for connectivity (network_id) checks.
     * @return PathResult with found flag, path, and total cost.
     */
    PathResult find_path(
        const GridPosition& start,
        const GridPosition& end,
        const PathwayGrid& grid,
        const NetworkGraph& graph
    );

private:
    /**
     * @brief Manhattan distance heuristic.
     * @param a First position.
     * @param b Second position.
     * @return Manhattan distance between a and b.
     */
    static uint32_t heuristic(const GridPosition& a, const GridPosition& b);

    /**
     * @brief Edge cost between adjacent tiles.
     *
     * Base cost = 10. Will be enhanced by E7-024.
     *
     * @param from Source position.
     * @param to   Destination position.
     * @return Edge traversal cost.
     */
    static uint32_t edge_cost(const GridPosition& from, const GridPosition& to);
};

} // namespace transport
} // namespace sims3000
