/**
 * @file IBuildingQueryable.h
 * @brief Pure abstract interface for building system queries (Ticket 4-036)
 *
 * Defines the IBuildingQueryable interface providing read-only queries
 * into the building system. All methods are O(1) or O(entity_count).
 *
 * Implemented by BuildingSystem.
 *
 * @see BuildingSystem.h
 * @see /docs/epics/epic-4/tickets.md (ticket 4-036)
 */

#ifndef SIMS3000_BUILDING_IBUILDINGQUERYABLE_H
#define SIMS3000_BUILDING_IBUILDINGQUERYABLE_H

#include <sims3000/building/BuildingTypes.h>
#include <cstdint>
#include <vector>

#ifdef __has_include
#if __has_include(<optional>)
#include <optional>
#endif
#endif

namespace sims3000 {
namespace building {

/**
 * @class IBuildingQueryable
 * @brief Pure abstract interface for building system queries.
 *
 * Provides read-only access to building data for external systems
 * (e.g., rendering, UI, economy). All methods should be efficient
 * (O(1) for grid lookups, O(entity_count) for iterations).
 */
class IBuildingQueryable {
public:
    virtual ~IBuildingQueryable() = default;

    /**
     * @brief Get building entity ID at grid position.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return Entity ID of building at position, or 0 if no building.
     */
    virtual uint32_t get_building_at(int32_t x, int32_t y) const = 0;

    /**
     * @brief Check if tile is occupied by a building.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return true if a building occupies the tile.
     */
    virtual bool is_tile_occupied(int32_t x, int32_t y) const = 0;

    /**
     * @brief Check if a rectangular footprint is available (all tiles empty).
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param w Width in tiles.
     * @param h Height in tiles.
     * @return true if all tiles in footprint are unoccupied.
     */
    virtual bool is_footprint_available(int32_t x, int32_t y, uint8_t w, uint8_t h) const = 0;

    /**
     * @brief Get all building entity IDs within a rectangular area.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param w Width in tiles.
     * @param h Height in tiles.
     * @return Vector of unique entity IDs found in the rectangle.
     */
    virtual std::vector<uint32_t> get_buildings_in_rect(int32_t x, int32_t y, int32_t w, int32_t h) const = 0;

    /**
     * @brief Get all building entity IDs owned by a specific player.
     * @param player_id Overseer player ID.
     * @return Vector of entity IDs owned by the player.
     */
    virtual std::vector<uint32_t> get_buildings_by_owner(uint8_t player_id) const = 0;

    /**
     * @brief Get total number of building entities.
     * @return Count of all entities.
     */
    virtual uint32_t get_building_count() const = 0;

    /**
     * @brief Get number of buildings in a specific state.
     * @param state The BuildingState to count.
     * @return Count of entities in the specified state.
     */
    virtual uint32_t get_building_count_by_state(BuildingState state) const = 0;

    /**
     * @brief Get building state for an entity.
     * @param entity_id The entity ID to query.
     * @return Optional containing the state, or empty if entity not found.
     */
    virtual std::optional<BuildingState> get_building_state(uint32_t entity_id) const = 0;

    /**
     * @brief Get total capacity for a zone building type and player.
     * @param type Zone building type.
     * @param player_id Overseer player ID.
     * @return Sum of capacity across matching entities.
     */
    virtual uint32_t get_total_capacity(ZoneBuildingType type, uint8_t player_id) const = 0;

    /**
     * @brief Get total occupancy for a zone building type and player.
     * @param type Zone building type.
     * @param player_id Overseer player ID.
     * @return Sum of current_occupancy across matching entities.
     */
    virtual uint32_t get_total_occupancy(ZoneBuildingType type, uint8_t player_id) const = 0;
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_IBUILDINGQUERYABLE_H
