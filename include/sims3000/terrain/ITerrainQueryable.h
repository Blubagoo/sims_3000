/**
 * @file ITerrainQueryable.h
 * @brief Primary read-only terrain data query interface for downstream systems
 *
 * ITerrainQueryable is the primary way all downstream systems interact with terrain:
 * - ZoneSystem: Checks buildability for zone placement
 * - EnergySystem: Queries terrain for conduit placement costs
 * - FluidSystem: Queries water distances and terrain types
 * - TransportSystem: Queries terrain for road placement costs
 * - PortSystem: Queries water proximity for port placement
 * - LandValueSystem: Queries terrain bonuses for land value calculation
 *
 * Interface stability is paramount - this API will be called by at least 6 major systems.
 *
 * All queries are O(1) or use pre-computed data:
 * - Direct grid lookups: O(1)
 * - Pre-computed fields: O(1) (water distance, slope, etc.)
 *
 * Batch queries (get_tiles_in_rect, etc.) iterate row-major for cache efficiency.
 *
 * Out-of-bounds coordinates return safe defaults (not UB).
 *
 * @see /docs/canon/interfaces.yaml (terrain.ITerrainQueryable)
 * @see /plans/epics/epic-3-terrain.yaml
 */

#ifndef SIMS3000_TERRAIN_ITERRAINQUERYABLE_H
#define SIMS3000_TERRAIN_ITERRAINQUERYABLE_H

#include <sims3000/terrain/TerrainTypes.h>
#include <sims3000/terrain/TerrainEvents.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace terrain {

/**
 * @interface ITerrainQueryable
 * @brief Read-only terrain data queries for gameplay systems.
 *
 * Abstract interface that TerrainSystem implements. All methods are const
 * to ensure read-only access. Thread-safe for read access during render.
 *
 * Buildability logic:
 * - type.buildable OR (type.clearable AND is_cleared) AND NOT is_underwater
 *
 * Out-of-bounds behavior:
 * - Coordinate queries return safe defaults (TerrainType::Substrate, elevation 0, etc.)
 * - Boolean queries return false for out-of-bounds
 */
class ITerrainQueryable {
public:
    /// Virtual destructor for proper polymorphic cleanup
    virtual ~ITerrainQueryable() = default;

    // =========================================================================
    // Core Terrain Property Queries - O(1) grid lookups
    // =========================================================================

    /**
     * @brief Get terrain type at grid position.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return TerrainType at position. Returns Substrate for out-of-bounds.
     */
    virtual TerrainType get_terrain_type(std::int32_t x, std::int32_t y) const = 0;

    /**
     * @brief Get elevation at grid position.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Elevation (0-31). Returns 0 for out-of-bounds.
     */
    virtual std::uint8_t get_elevation(std::int32_t x, std::int32_t y) const = 0;

    // =========================================================================
    // Buildability Queries - Core game logic
    // =========================================================================

    /**
     * @brief Check if position can be built on.
     *
     * Buildability logic:
     * - (type.buildable OR (type.clearable AND is_cleared)) AND NOT is_underwater
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if structures can be placed at position.
     *         Returns false for out-of-bounds.
     */
    virtual bool is_buildable(std::int32_t x, std::int32_t y) const = 0;

    // =========================================================================
    // Slope and Elevation Analysis - O(1) precomputed or trivial calculation
    // =========================================================================

    /**
     * @brief Get slope between two adjacent tiles.
     *
     * Slope is the absolute difference in elevation between two tiles.
     * Used for road building costs and aesthetic placement.
     *
     * @param x1 First tile X coordinate.
     * @param y1 First tile Y coordinate.
     * @param x2 Second tile X coordinate.
     * @param y2 Second tile Y coordinate.
     * @return Absolute elevation difference (0-31). Returns 0 if either is out-of-bounds.
     */
    virtual std::uint8_t get_slope(std::int32_t x1, std::int32_t y1,
                                    std::int32_t x2, std::int32_t y2) const = 0;

    /**
     * @brief Get average elevation within a radius.
     *
     * Used for large building placement and terrain smoothness checks.
     *
     * @param x Center X coordinate.
     * @param y Center Y coordinate.
     * @param radius Radius in tiles (0 = single tile).
     * @return Average elevation as float. Returns 0.0f if center is out-of-bounds.
     */
    virtual float get_average_elevation(std::int32_t x, std::int32_t y,
                                         std::uint32_t radius) const = 0;

    // =========================================================================
    // Water Proximity Queries - O(1) precomputed distance field
    // =========================================================================

    /**
     * @brief Get distance to nearest water tile.
     *
     * Uses pre-computed water distance field for O(1) lookup.
     * Water tiles (DeepVoid, FlowChannel, StillBasin) have distance 0.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Manhattan distance to nearest water (0-255, capped).
     *         Returns 255 (max distance) for out-of-bounds.
     */
    virtual std::uint32_t get_water_distance(std::int32_t x, std::int32_t y) const = 0;

    // =========================================================================
    // Land Value and Harmony Queries - O(1) lookup from TERRAIN_INFO
    // =========================================================================

    /**
     * @brief Get land value bonus for terrain at position.
     *
     * Used by LandValueSystem to calculate sector desirability.
     * Values come from TERRAIN_INFO lookup table.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Value bonus (can be negative for toxic terrain).
     *         Returns 0.0f for out-of-bounds.
     */
    virtual float get_value_bonus(std::int32_t x, std::int32_t y) const = 0;

    /**
     * @brief Get harmony (happiness) bonus for terrain at position.
     *
     * Used by PopulationSystem to calculate resident satisfaction.
     * Values come from TERRAIN_INFO lookup table.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Harmony bonus (can be negative for toxic terrain).
     *         Returns 0.0f for out-of-bounds.
     */
    virtual float get_harmony_bonus(std::int32_t x, std::int32_t y) const = 0;

    // =========================================================================
    // Construction Cost Queries - O(1) lookup from TERRAIN_INFO
    // =========================================================================

    /**
     * @brief Get build cost modifier for terrain at position.
     *
     * Multiplier applied to construction costs for buildings on/near this terrain.
     * 100 = normal cost (1.0x), 150 = 50% more expensive (1.5x).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Cost modifier as percentage (100 = 1.0x).
     *         Returns 100 for out-of-bounds.
     */
    virtual std::int32_t get_build_cost_modifier(std::int32_t x, std::int32_t y) const = 0;

    // =========================================================================
    // Contamination Queries - O(1) lookup from TERRAIN_INFO
    // =========================================================================

    /**
     * @brief Get contamination output for terrain at position.
     *
     * Some terrain types (BlightMires) naturally generate contamination.
     * Used by ContaminationSystem for pollution spread calculation.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Contamination units produced per tick (0 for most terrain).
     *         Returns 0 for out-of-bounds.
     */
    virtual std::uint32_t get_contamination_output(std::int32_t x, std::int32_t y) const = 0;

    // =========================================================================
    // Map Metadata Queries - O(1) constant values
    // =========================================================================

    /**
     * @brief Get map width in tiles.
     * @return Map width (128, 256, or 512).
     */
    virtual std::uint32_t get_map_width() const = 0;

    /**
     * @brief Get map height in tiles.
     * @return Map height (128, 256, or 512).
     */
    virtual std::uint32_t get_map_height() const = 0;

    /**
     * @brief Get sea level elevation.
     *
     * Tiles at or below this elevation are considered underwater.
     *
     * @return Sea level (default: 8).
     */
    virtual std::uint8_t get_sea_level() const = 0;

    // =========================================================================
    // Batch Queries - Efficient rectangular region operations
    // =========================================================================

    /**
     * @brief Get all terrain components within a rectangular region.
     *
     * Fills the output vector with TerrainComponent data for all tiles
     * in the specified rectangle, iterating in row-major order for
     * cache efficiency.
     *
     * Tiles outside map bounds are skipped (not included in output).
     * The rect is clipped to map bounds before iteration.
     *
     * Performance target: 10,000 tile rect query < 10 microseconds.
     *
     * @param rect The rectangular region to query (clipped to map bounds).
     * @param out_tiles Output vector to fill with terrain components.
     *                  Vector is cleared before filling.
     */
    virtual void get_tiles_in_rect(const GridRect& rect,
                                    std::vector<TerrainComponent>& out_tiles) const = 0;

    /**
     * @brief Count buildable tiles within a rectangular region.
     *
     * Returns the number of tiles in the specified rectangle that
     * are currently buildable (per is_buildable() logic).
     *
     * Tiles outside map bounds are counted as not buildable.
     * The rect is clipped to map bounds before counting.
     *
     * Iteration is row-major for cache efficiency.
     *
     * @param rect The rectangular region to query.
     * @return Number of buildable tiles in the region.
     */
    virtual std::uint32_t get_buildable_tiles_in_rect(const GridRect& rect) const = 0;

    /**
     * @brief Count tiles of a specific terrain type within a rectangular region.
     *
     * Returns the number of tiles matching the specified TerrainType
     * within the given rectangle.
     *
     * Tiles outside map bounds are not counted.
     * The rect is clipped to map bounds before counting.
     *
     * Iteration is row-major for cache efficiency.
     *
     * @param rect The rectangular region to query.
     * @param type The terrain type to count.
     * @return Number of tiles matching the specified type.
     */
    virtual std::uint32_t count_terrain_type_in_rect(const GridRect& rect,
                                                      TerrainType type) const = 0;

protected:
    /// Protected default constructor (interface cannot be instantiated directly)
    ITerrainQueryable() = default;

    /// Non-copyable (interfaces should be used via pointers/references)
    ITerrainQueryable(const ITerrainQueryable&) = delete;
    ITerrainQueryable& operator=(const ITerrainQueryable&) = delete;

    /// Movable for flexibility
    ITerrainQueryable(ITerrainQueryable&&) = default;
    ITerrainQueryable& operator=(ITerrainQueryable&&) = default;
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_ITERRAINQUERYABLE_H
