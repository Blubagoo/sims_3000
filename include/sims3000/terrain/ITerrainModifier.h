/**
 * @file ITerrainModifier.h
 * @brief Interface for terrain modification operations
 *
 * ITerrainModifier provides the contract for terrain modification requests.
 * This interface is separate from ITerrainQueryable because modifications
 * are only available server-side in multiplayer, while queries are available
 * to all clients.
 *
 * Cost query methods are const and can be called client-side for UI preview
 * without actually modifying terrain state.
 *
 * Modification operations:
 * - clear_terrain: Remove vegetation/crystals for building (sets IS_CLEARED flag)
 * - level_terrain: Flatten terrain to a target elevation
 *
 * @see ITerrainQueryable for read-only terrain queries
 * @see TerrainTypeInfo for clearable/buildable properties per terrain type
 * @see /docs/canon/interfaces.yaml (terrain.ITerrainModifier)
 */

#ifndef SIMS3000_TERRAIN_ITERRAINMODIFIER_H
#define SIMS3000_TERRAIN_ITERRAINMODIFIER_H

#include <cstdint>

namespace sims3000 {
namespace terrain {

/**
 * @brief Player identifier type (matches data contract in interfaces.yaml)
 *
 * Special values:
 * - 0: GAME_MASTER (virtual entity owning unclaimed tiles)
 * - 1-4: Player IDs
 */
using PlayerID = std::uint8_t;

/**
 * @interface ITerrainModifier
 * @brief Interface for terrain modification operations
 *
 * This interface is implemented by TerrainSystem to provide terrain
 * modification capabilities. In multiplayer, only the server can call
 * modification methods; clients must send requests through the network.
 *
 * Usage pattern:
 * 1. Client calls get_clear_cost() or get_level_cost() for UI preview
 * 2. Client sends modification request to server
 * 3. Server validates and calls clear_terrain() or level_terrain()
 * 4. Server broadcasts TerrainModifiedEvent to all clients
 *
 * Cost calculation:
 * - clear_terrain cost comes from TerrainTypeInfo::clear_cost
 * - level_terrain cost scales with elevation difference
 */
class ITerrainModifier {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~ITerrainModifier() = default;

    // =========================================================================
    // Modification Methods (Server-side only in multiplayer)
    // =========================================================================

    /**
     * @brief Clear vegetation/crystals at a tile to allow building.
     *
     * Clearing sets the IS_CLEARED flag on the terrain tile. Only terrain
     * types marked as clearable in TerrainTypeInfo can be cleared.
     *
     * Preconditions:
     * - Tile must be within bounds
     * - Terrain type must be clearable (TerrainTypeInfo::clearable == true)
     * - Tile must not already be cleared
     * - Player must have ownership or tile must be purchasable (GAME_MASTER owned)
     *
     * @param x X coordinate of the tile
     * @param y Y coordinate of the tile
     * @param player_id ID of the player requesting the operation
     * @return true if clearing succeeded, false if preconditions not met
     *
     * @note Does NOT deduct cost from player treasury - caller is responsible
     *       for checking cost via get_clear_cost() and deducting credits.
     */
    virtual bool clear_terrain(std::int32_t x, std::int32_t y, PlayerID player_id) = 0;

    /**
     * @brief Level terrain to a target elevation.
     *
     * Changes the elevation of a tile to match a target value. This is used
     * for flattening terrain before building or creating slopes.
     *
     * Preconditions:
     * - Tile must be within bounds
     * - Target elevation must be valid (0-31)
     * - Terrain type must be modifiable (not water types)
     * - Player must have ownership
     *
     * @param x X coordinate of the tile
     * @param y Y coordinate of the tile
     * @param target_elevation Desired elevation (0-31)
     * @param player_id ID of the player requesting the operation
     * @return true if leveling succeeded, false if preconditions not met
     *
     * @note Does NOT deduct cost from player treasury - caller is responsible
     *       for checking cost via get_level_cost() and deducting credits.
     * @note This may be a multi-tick operation for large elevation changes.
     */
    virtual bool level_terrain(std::int32_t x, std::int32_t y,
                               std::uint8_t target_elevation,
                               PlayerID player_id) = 0;

    // =========================================================================
    // Cost Query Methods (Safe for client-side, const)
    // =========================================================================

    /**
     * @brief Get the cost to clear terrain at a position.
     *
     * Returns the cost in credits to clear the terrain at the specified
     * position. Cost is determined by TerrainTypeInfo::clear_cost.
     *
     * Special cases:
     * - Returns 0 if already cleared
     * - Returns -1 (invalid) if terrain is not clearable
     * - Returns negative values for terrain that yields resources (e.g., PrismaFields)
     *
     * @param x X coordinate of the tile
     * @param y Y coordinate of the tile
     * @return Cost in credits, or -1 if clearing is not possible
     *
     * @note This is a const method - safe to call from any thread.
     * @note Negative cost means clearing yields credits (resource harvesting).
     */
    virtual std::int64_t get_clear_cost(std::int32_t x, std::int32_t y) const = 0;

    /**
     * @brief Get the cost to level terrain to a target elevation.
     *
     * Returns the cost in credits to change the terrain elevation at the
     * specified position to the target elevation. Cost scales with the
     * absolute elevation difference.
     *
     * Cost formula: base_cost * |current_elevation - target_elevation|
     *
     * Special cases:
     * - Returns 0 if already at target elevation
     * - Returns -1 (invalid) if leveling is not possible (water, toxic)
     * - Returns -1 if coordinates are out of bounds
     *
     * @param x X coordinate of the tile
     * @param y Y coordinate of the tile
     * @param target_elevation Desired elevation (0-31)
     * @return Cost in credits, or -1 if leveling is not possible
     *
     * @note This is a const method - safe to call from any thread.
     */
    virtual std::int64_t get_level_cost(std::int32_t x, std::int32_t y,
                                        std::uint8_t target_elevation) const = 0;
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_ITERRAINMODIFIER_H
