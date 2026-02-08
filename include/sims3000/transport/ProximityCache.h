/**
 * @file ProximityCache.h
 * @brief ProximityCache for O(1) distance-to-nearest-pathway queries (Ticket E7-006)
 *
 * ProximityCache stores the Manhattan distance (4-directional: N/S/E/W)
 * from each tile to the nearest pathway tile, computed via multi-source BFS.
 * Distance values are stored as uint8_t, capped at 255 (meaning no pathway
 * within 255 tiles).
 *
 * Memory: 1 byte per tile
 * - 128x128:   16KB
 * - 256x256:   64KB
 * - 512x512:  256KB
 *
 * @see /docs/canon/decisions/CCR-007 (Manhattan distance specification)
 */

#pragma once
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace transport {

class PathwayGrid;  // forward declare

/**
 * @class ProximityCache
 * @brief Caches Manhattan distance from each tile to the nearest pathway.
 *
 * The cache is rebuilt from a PathwayGrid using multi-source BFS.
 * After a rebuild, get_distance() returns O(1) lookups.
 *
 * The dirty flag indicates when the cache needs rebuilding (e.g., after
 * pathways are added or removed from the grid).
 */
class ProximityCache {
public:
    /**
     * @brief Default constructor. Creates an empty 0x0 cache.
     */
    ProximityCache() = default;

    /**
     * @brief Construct a proximity cache with the specified dimensions.
     *
     * All distances are initialized to 255 (no pathway in range).
     * The cache starts in the dirty state.
     *
     * @param width Cache width in tiles.
     * @param height Cache height in tiles.
     */
    ProximityCache(uint32_t width, uint32_t height);

    // ========================================================================
    // Distance queries
    // ========================================================================

    /**
     * @brief Get the Manhattan distance to the nearest pathway tile.
     *
     * O(1) lookup after rebuild. Returns 255 if no pathway is within range,
     * or if coordinates are out of bounds.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Distance to nearest pathway (0 = on a pathway, 255 = none in range).
     */
    uint8_t get_distance(int32_t x, int32_t y) const;

    // ========================================================================
    // Rebuild
    // ========================================================================

    /**
     * @brief Rebuild the cache from the pathway grid if dirty.
     *
     * Performs multi-source BFS from all pathway tiles to compute
     * Manhattan distances. No-op if the cache is not dirty.
     *
     * @param grid The PathwayGrid to compute distances from.
     */
    void rebuild_if_dirty(const PathwayGrid& grid);

    // ========================================================================
    // Dirty tracking
    // ========================================================================

    /**
     * @brief Mark the cache as needing a rebuild.
     */
    void mark_dirty();

    /**
     * @brief Check if the cache needs rebuilding.
     * @return true if the cache is dirty and needs a rebuild.
     */
    bool is_dirty() const;

    // ========================================================================
    // Dimensions
    // ========================================================================

    /**
     * @brief Get cache width in tiles.
     * @return Width of the cache.
     */
    uint32_t width() const;

    /**
     * @brief Get cache height in tiles.
     * @return Height of the cache.
     */
    uint32_t height() const;

private:
    /**
     * @brief Perform the actual multi-source BFS rebuild.
     *
     * Algorithm:
     * 1. Initialize all distances to 255
     * 2. Seed BFS queue with all pathway tiles at distance 0
     * 3. Expand 4-directionally (N/S/E/W), incrementing distance
     * 4. Cap at 255
     *
     * @param grid The PathwayGrid to compute distances from.
     */
    void rebuild(const PathwayGrid& grid);

    std::vector<uint8_t> distance_cache_;  ///< 1 byte per tile (Manhattan distance)
    bool dirty_ = true;                    ///< True if cache needs rebuilding
    uint32_t width_ = 0;                   ///< Cache width in tiles
    uint32_t height_ = 0;                  ///< Cache height in tiles
};

} // namespace transport
} // namespace sims3000
