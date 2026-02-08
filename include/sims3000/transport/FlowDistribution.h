/**
 * @file FlowDistribution.h
 * @brief Flow distribution from buildings to pathways (Epic 7, Ticket E7-013)
 *
 * Distributes traffic flow from building sources to their nearest pathway tiles.
 * Buildings within max_distance of a pathway contribute flow to that pathway's
 * accumulator. Buildings without pathway access are skipped.
 *
 * Cross-ownership: flow distributes regardless of pathway owner (CCR-002).
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>

namespace sims3000 {
namespace transport {

// Forward declarations
class PathwayGrid;
class ProximityCache;

/**
 * @struct BuildingTrafficSource
 * @brief A building that generates traffic flow.
 *
 * Represents a single building's position, flow contribution,
 * and owning player for traffic distribution.
 */
struct BuildingTrafficSource {
    int32_t x, y;           ///< Building position (grid coordinates)
    uint32_t flow_amount;   ///< Calculated traffic contribution (from TrafficContribution)
    uint8_t owner;          ///< Building owner (player ID 0-3)
};

/**
 * @class FlowDistribution
 * @brief Distributes traffic flow from buildings to nearest pathways.
 *
 * For each building source, finds the nearest pathway tile within
 * max_distance and adds the building's flow to that pathway's accumulator.
 * Uses ProximityCache for fast distance checks and PathwayGrid for
 * pathway tile lookups.
 */
class FlowDistribution {
public:
    FlowDistribution() = default;

    /**
     * @brief Distribute flow from building sources to nearest pathways.
     *
     * Algorithm:
     * 1. For each building source, check proximity cache distance
     * 2. If within max_distance, find nearest pathway tile
     * 3. Add flow to that pathway's accumulator (keyed by packed position)
     * 4. Buildings without pathway access are skipped
     *
     * Cross-ownership: flow distributes regardless of pathway owner (CCR-002).
     *
     * @param sources       Vector of building traffic sources.
     * @param grid          PathwayGrid for pathway tile lookup.
     * @param cache         ProximityCache for fast distance queries.
     * @param flow_accumulator Output map: packed position -> accumulated flow.
     * @param max_distance  Maximum distance to search for pathways (default 3).
     * @return Number of buildings that found pathway access.
     */
    uint32_t distribute_flow(
        const std::vector<BuildingTrafficSource>& sources,
        const PathwayGrid& grid,
        const ProximityCache& cache,
        std::unordered_map<uint64_t, uint32_t>& flow_accumulator,
        uint8_t max_distance = 3
    );

    /**
     * @brief Pack a 2D position into a uint64_t key.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return Packed position key.
     */
    static uint64_t pack_position(int32_t x, int32_t y);

    /**
     * @brief Unpack X coordinate from a packed position key.
     * @param key Packed position.
     * @return X coordinate.
     */
    static int32_t unpack_x(uint64_t key);

    /**
     * @brief Unpack Y coordinate from a packed position key.
     * @param key Packed position.
     * @return Y coordinate.
     */
    static int32_t unpack_y(uint64_t key);

private:
    /**
     * @brief Find the nearest pathway tile within max_distance.
     *
     * Searches in a spiral pattern (N/S/E/W BFS) from the given position
     * to find the closest pathway tile within the specified distance.
     *
     * @param x            Starting X coordinate.
     * @param y            Starting Y coordinate.
     * @param grid         PathwayGrid for pathway tile lookup.
     * @param out_px       Output: X coordinate of found pathway.
     * @param out_py       Output: Y coordinate of found pathway.
     * @param max_distance Maximum search distance.
     * @return true if a pathway was found within max_distance.
     */
    bool find_nearest_pathway(int32_t x, int32_t y, const PathwayGrid& grid,
                             int32_t& out_px, int32_t& out_py, uint8_t max_distance);
};

} // namespace transport
} // namespace sims3000
