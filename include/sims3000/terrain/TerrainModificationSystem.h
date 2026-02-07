/**
 * @file TerrainModificationSystem.h
 * @brief System implementing terrain modification operations
 *
 * TerrainModificationSystem provides the concrete implementation of the
 * ITerrainModifier interface. It handles terrain clearing (purging) and
 * leveling operations with proper validation and event dispatch.
 *
 * Operations:
 * - clear_terrain: Instant (single tick) clearing of vegetation/crystals
 * - level_terrain: Multi-tick terrain leveling (not implemented in this ticket)
 *
 * Server-authoritative - validation checks are performed on all operations.
 *
 * @see ITerrainModifier for interface contract
 * @see TerrainEvents.h for TerrainModifiedEvent
 * @see TerrainTypeInfo.h for clearable/cost properties
 */

#ifndef SIMS3000_TERRAIN_TERRAINMODIFICATIONSYSTEM_H
#define SIMS3000_TERRAIN_TERRAINMODIFICATIONSYSTEM_H

#include <sims3000/terrain/ITerrainModifier.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainEvents.h>
#include <sims3000/terrain/ChunkDirtyTracker.h>
#include <functional>
#include <cstdint>

namespace sims3000 {
namespace terrain {

/**
 * @brief Callback type for terrain modification events.
 *
 * Systems interested in terrain changes can register a callback to receive
 * TerrainModifiedEvent notifications. This enables decoupled event handling.
 */
using TerrainEventCallback = std::function<void(const TerrainModifiedEvent&)>;

/**
 * @class TerrainModificationSystem
 * @brief Implements terrain modification operations (ITerrainModifier).
 *
 * This system owns the logic for terrain modifications. It operates on a
 * TerrainGrid reference and notifies listeners via event callbacks.
 *
 * Usage pattern:
 * 1. Construct with TerrainGrid and ChunkDirtyTracker references
 * 2. Optionally set event callback for TerrainModifiedEvent
 * 3. Call clear_terrain() or level_terrain() to modify terrain
 * 4. Query costs with get_clear_cost() or get_level_cost()
 *
 * All modification operations are instant (single tick).
 */
class TerrainModificationSystem : public ITerrainModifier {
public:
    /// Base cost per elevation level change for leveling operations
    static constexpr std::int64_t LEVEL_BASE_COST = 10;

    /**
     * @brief Construct the system with required dependencies.
     *
     * @param grid Reference to the TerrainGrid to modify.
     * @param dirty_tracker Reference to the ChunkDirtyTracker for render updates.
     */
    TerrainModificationSystem(TerrainGrid& grid, ChunkDirtyTracker& dirty_tracker);

    /**
     * @brief Set the callback for terrain modification events.
     *
     * The callback is invoked after each successful modification operation.
     * Only one callback can be set; subsequent calls replace the previous.
     *
     * @param callback Function to call when terrain is modified.
     */
    void setEventCallback(TerrainEventCallback callback);

    // =========================================================================
    // ITerrainModifier Implementation
    // =========================================================================

    /**
     * @brief Clear vegetation/crystals at a tile to allow building.
     *
     * Validates:
     * - Tile is within bounds
     * - Terrain type is clearable (BiolumeGrove, PrismaFields, SporeFlats)
     * - Tile is not already cleared
     * - Player has authority (for now, any valid player_id is accepted)
     *
     * On success:
     * - Sets IS_CLEARED flag on the tile
     * - Marks the containing chunk as dirty
     * - Fires TerrainModifiedEvent with ModificationType::Cleared
     *
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     * @param player_id ID of the player requesting the operation.
     * @return true if clearing succeeded, false if validation failed.
     *
     * @note Does NOT deduct cost from player treasury - caller is responsible.
     */
    bool clear_terrain(std::int32_t x, std::int32_t y, PlayerID player_id) override;

    /**
     * @brief Level terrain to a target elevation.
     *
     * @note This operation is not fully implemented in ticket 3-019.
     *       Currently returns false for all calls.
     *
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     * @param target_elevation Desired elevation (0-31).
     * @param player_id ID of the player requesting the operation.
     * @return false (not implemented in this ticket).
     */
    bool level_terrain(std::int32_t x, std::int32_t y,
                       std::uint8_t target_elevation,
                       PlayerID player_id) override;

    /**
     * @brief Get the cost to clear terrain at a position.
     *
     * Returns:
     * - Positive value: cost to clear
     * - Negative value: revenue from clearing (e.g., PrismaFields crystals)
     * - 0: already cleared
     * - -1: terrain is not clearable or out of bounds
     *
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     * @return Cost in credits, or -1 if clearing is not possible.
     */
    std::int64_t get_clear_cost(std::int32_t x, std::int32_t y) const override;

    /**
     * @brief Get the cost to level terrain to a target elevation.
     *
     * @param x X coordinate of the tile.
     * @param y Y coordinate of the tile.
     * @param target_elevation Desired elevation (0-31).
     * @return Cost in credits, or -1 if leveling is not possible.
     */
    std::int64_t get_level_cost(std::int32_t x, std::int32_t y,
                                std::uint8_t target_elevation) const override;

private:
    /**
     * @brief Fire a terrain modified event.
     *
     * Invokes the event callback (if set) and marks the affected chunk dirty.
     *
     * @param x X coordinate of the modified tile.
     * @param y Y coordinate of the modified tile.
     * @param type The type of modification that occurred.
     */
    void fireEvent(std::int32_t x, std::int32_t y, ModificationType type);

    /**
     * @brief Check if a player has authority to modify a tile.
     *
     * For now, any valid player_id (0-255) is accepted.
     * Future implementations will check tile ownership.
     *
     * @param player_id The player requesting the operation.
     * @return true if player has authority.
     */
    bool checkPlayerAuthority(PlayerID player_id) const;

    TerrainGrid& grid_;                     ///< Reference to terrain data
    ChunkDirtyTracker& dirty_tracker_;      ///< Reference to dirty tracker
    TerrainEventCallback event_callback_;   ///< Optional event callback
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINMODIFICATIONSYSTEM_H
