/**
 * @file ContaminationSourceQuery.h
 * @brief Terrain-based contamination source queries for ContaminationSystem
 *
 * Exposes terrain tiles that generate contamination (BlightMires) as
 * queryable sources for Epic 10's ContaminationSystem. Since terrain tiles
 * are not individual entities, this provides a bulk query interface that
 * returns all contamination-producing tile positions and their output rates.
 *
 * The query result is cached for O(1) access and automatically invalidated
 * when terrain is modified (via TerrainModifiedEvent).
 *
 * @see TerrainTypeInfo for contamination generation flag
 * @see TerrainModifiedEvent for cache invalidation trigger
 */

#ifndef SIMS3000_TERRAIN_CONTAMINATIONSOURCEQUERY_H
#define SIMS3000_TERRAIN_CONTAMINATIONSOURCEQUERY_H

#include <sims3000/core/types.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainEvents.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace terrain {

/**
 * @struct ContaminationSource
 * @brief Data describing a single contamination-producing terrain tile.
 *
 * Returned by terrain contamination queries to describe each tile
 * that generates contamination. Contains position, output rate, and
 * the terrain type responsible for contamination.
 */
struct ContaminationSource {
    GridPosition position;               ///< Tile position on the grid
    std::uint32_t contamination_per_tick; ///< Units generated per simulation tick
    TerrainType source_type;              ///< Terrain type generating contamination
    std::uint8_t _padding[3] = {0, 0, 0}; ///< Alignment padding
};

// Verify ContaminationSource is properly sized and trivially copyable
static_assert(sizeof(ContaminationSource) == 12,
    "ContaminationSource should be 12 bytes (4 + 4 + 1 + 3 padding)");
static_assert(std::is_trivially_copyable<ContaminationSource>::value,
    "ContaminationSource must be trivially copyable");

/**
 * @class ContaminationSourceQuery
 * @brief Cached query interface for terrain-based contamination sources.
 *
 * Provides efficient access to all terrain tiles that generate contamination.
 * The cache is built once and invalidated only when terrain is modified.
 *
 * Usage:
 * @code
 *   ContaminationSourceQuery query(terrain_grid);
 *   const auto& sources = query.get_terrain_contamination_sources();
 *   for (const auto& source : sources) {
 *       // Process each contamination source
 *   }
 * @endcode
 *
 * Thread Safety:
 * - NOT thread-safe. Cache operations are not synchronized.
 * - Call from simulation thread only.
 */
class ContaminationSourceQuery {
public:
    /**
     * @brief Construct a contamination source query for a terrain grid.
     *
     * Does not immediately build the cache. Cache is built lazily on first
     * query or explicitly via rebuild_cache().
     *
     * @param grid Reference to the terrain grid to query. Must remain valid
     *             for the lifetime of this query object.
     */
    explicit ContaminationSourceQuery(const TerrainGrid& grid);

    /**
     * @brief Get all terrain contamination sources.
     *
     * Returns cached data in O(1). If cache is invalid, triggers rebuild
     * in O(blight_mire_count) where blight_mire_count is the number of
     * BlightMires tiles on the map.
     *
     * @return Vector of ContaminationSource entries for all contamination-
     *         producing tiles.
     */
    const std::vector<ContaminationSource>& get_terrain_contamination_sources();

    /**
     * @brief Process a terrain modified event for cache invalidation.
     *
     * When terrain is modified, this method checks if the modification
     * affects any contamination-producing terrain and invalidates the
     * cache if necessary. Specifically:
     * - Terraformed events always invalidate (terrain type changes)
     * - Generated events always invalidate (new map)
     * - Other events are checked for BlightMires overlap
     *
     * Call this method when processing TerrainModifiedEvent.
     *
     * @param event The terrain modification event.
     */
    void on_terrain_modified(const TerrainModifiedEvent& event);

    /**
     * @brief Force cache invalidation.
     *
     * Marks the cache as invalid. Next call to get_terrain_contamination_sources()
     * will trigger a full rebuild.
     */
    void invalidate_cache();

    /**
     * @brief Force immediate cache rebuild.
     *
     * Scans the entire terrain grid for contamination-producing tiles
     * and populates the cache. This is O(n) where n is total tile count.
     *
     * Prefer using get_terrain_contamination_sources() which rebuilds
     * only when necessary.
     */
    void rebuild_cache();

    /**
     * @brief Check if the cache is currently valid.
     *
     * @return true if cache is valid and can be returned in O(1).
     */
    bool is_cache_valid() const { return m_cache_valid; }

    /**
     * @brief Get the number of cached contamination sources.
     *
     * @return Number of sources in cache, or 0 if cache is invalid.
     */
    std::size_t source_count() const { return m_sources.size(); }

private:
    const TerrainGrid& m_grid;                  ///< Reference to terrain grid
    std::vector<ContaminationSource> m_sources; ///< Cached contamination sources
    bool m_cache_valid = false;                 ///< Cache validity flag
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_CONTAMINATIONSOURCEQUERY_H
