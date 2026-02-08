/**
 * @file BuildingEvents.h
 * @brief Building-related event definitions for Epic 4
 *
 * Defines all events emitted by BuildingSystem:
 * - BuildingConstructedEvent: Structure construction completed
 * - BuildingAbandonedEvent: Structure abandoned (decay starting)
 * - BuildingRestoredEvent: Abandoned structure restored to Active
 * - BuildingDerelictEvent: Structure fully decayed (non-functional)
 * - BuildingDeconstructedEvent: Structure demolished (debris created)
 * - DebrisClearedEvent: Debris auto-cleared (sector available)
 * - BuildingUpgradedEvent: Structure upgraded to higher level
 * - BuildingDowngradedEvent: Structure downgraded to lower level
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#ifndef SIMS3000_BUILDING_BUILDINGEVENTS_H
#define SIMS3000_BUILDING_BUILDINGEVENTS_H

#include <sims3000/building/BuildingTypes.h>
#include <cstdint>

namespace sims3000 {
namespace building {

/**
 * @struct BuildingConstructedEvent
 * @brief Event emitted when a structure completes construction.
 *
 * Emitted when BuildingState transitions from Materializing to Active.
 * ConstructionComponent is removed at this time.
 *
 * Consumed by:
 * - RenderingSystem: Remove construction visual, show final model
 * - UISystem: Update building count statistics
 * - AudioSystem: Play construction complete sound
 * - EconomySystem: Deduct final construction cost
 * - ZoneSystem: Update zone state to Occupied
 */
struct BuildingConstructedEvent {
    std::uint32_t entity_id;      ///< Building entity ID
    std::uint8_t owner_id;        ///< Owning overseer PlayerID
    ZoneBuildingType zone_type;   ///< Zone type (Habitation/Exchange/Fabrication)
    std::int32_t grid_x;          ///< Grid X coordinate
    std::int32_t grid_y;          ///< Grid Y coordinate
    std::uint32_t template_id;    ///< Building template ID

    BuildingConstructedEvent()
        : entity_id(0)
        , owner_id(0)
        , zone_type(ZoneBuildingType::Habitation)
        , grid_x(0)
        , grid_y(0)
        , template_id(0)
    {}

    BuildingConstructedEvent(std::uint32_t entity, std::uint8_t owner,
                             ZoneBuildingType type, std::int32_t x, std::int32_t y,
                             std::uint32_t template_id_val)
        : entity_id(entity)
        , owner_id(owner)
        , zone_type(type)
        , grid_x(x)
        , grid_y(y)
        , template_id(template_id_val)
    {}
};

/**
 * @struct BuildingAbandonedEvent
 * @brief Event emitted when a structure is abandoned (decay starting).
 *
 * Emitted when BuildingState transitions from Active to Abandoned.
 * Abandon timer begins counting down.
 *
 * Consumed by:
 * - RenderingSystem: Apply abandoned visual effect (flickering lights)
 * - UISystem: Update building status overlay
 * - AudioSystem: Play abandonment sound
 * - StatisticsSystem: Track abandonment count
 */
struct BuildingAbandonedEvent {
    std::uint32_t entity_id;      ///< Building entity ID
    std::uint8_t owner_id;        ///< Owning overseer PlayerID
    std::int32_t grid_x;          ///< Grid X coordinate
    std::int32_t grid_y;          ///< Grid Y coordinate

    BuildingAbandonedEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
    {}

    BuildingAbandonedEvent(std::uint32_t entity, std::uint8_t owner,
                           std::int32_t x, std::int32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct BuildingRestoredEvent
 * @brief Event emitted when an abandoned structure is restored to Active state.
 *
 * Emitted when BuildingState transitions from Abandoned back to Active.
 * Abandon timer is reset.
 *
 * Consumed by:
 * - RenderingSystem: Remove abandoned visual effect
 * - UISystem: Update building status overlay
 * - AudioSystem: Play restoration sound
 */
struct BuildingRestoredEvent {
    std::uint32_t entity_id;      ///< Building entity ID
    std::uint8_t owner_id;        ///< Owning overseer PlayerID
    std::int32_t grid_x;          ///< Grid X coordinate
    std::int32_t grid_y;          ///< Grid Y coordinate

    BuildingRestoredEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
    {}

    BuildingRestoredEvent(std::uint32_t entity, std::uint8_t owner,
                          std::int32_t x, std::int32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct BuildingDerelictEvent
 * @brief Event emitted when a structure becomes fully decayed (derelict).
 *
 * Emitted when BuildingState transitions from Abandoned to Derelict.
 * Structure is non-functional but still occupies sectors.
 *
 * Consumed by:
 * - RenderingSystem: Apply derelict visual effect (no lights, damaged model)
 * - UISystem: Update building status overlay
 * - AudioSystem: Play derelict sound
 * - StatisticsSystem: Track derelict count
 */
struct BuildingDerelictEvent {
    std::uint32_t entity_id;      ///< Building entity ID
    std::uint8_t owner_id;        ///< Owning overseer PlayerID
    std::int32_t grid_x;          ///< Grid X coordinate
    std::int32_t grid_y;          ///< Grid Y coordinate

    BuildingDerelictEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
    {}

    BuildingDerelictEvent(std::uint32_t entity, std::uint8_t owner,
                          std::int32_t x, std::int32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct BuildingDeconstructedEvent
 * @brief Event emitted when a structure is demolished (debris created).
 *
 * Emitted when BuildingState transitions to Deconstructed.
 * BuildingComponent is removed and DebrisComponent is added.
 * BuildingGrid footprint is cleared.
 *
 * Consumed by:
 * - RenderingSystem: Show debris visual
 * - UISystem: Update building count statistics
 * - AudioSystem: Play demolition sound
 * - EconomySystem: Deduct demolition cost (if player-initiated)
 * - ZoneSystem: Update zone state if zone still exists
 */
struct BuildingDeconstructedEvent {
    std::uint32_t entity_id;      ///< Building entity ID (now debris)
    std::uint8_t owner_id;        ///< Owning overseer PlayerID
    std::int32_t grid_x;          ///< Grid X coordinate
    std::int32_t grid_y;          ///< Grid Y coordinate
    bool was_player_initiated;    ///< True if overseer demolished, false if automatic (decay)

    BuildingDeconstructedEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
        , was_player_initiated(false)
    {}

    BuildingDeconstructedEvent(std::uint32_t entity, std::uint8_t owner,
                               std::int32_t x, std::int32_t y, bool player_init)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
        , was_player_initiated(player_init)
    {}
};

/**
 * @struct DebrisClearedEvent
 * @brief Event emitted when debris is cleared (sector becomes available).
 *
 * Emitted when debris auto-clears (timer expires) or is manually cleared.
 * Debris entity is destroyed and sectors become available for new construction.
 *
 * Consumed by:
 * - RenderingSystem: Remove debris visual
 * - UISystem: Update sector availability overlay
 * - AudioSystem: Play debris clear sound
 */
struct DebrisClearedEvent {
    std::uint32_t entity_id;      ///< Debris entity ID (about to be destroyed)
    std::int32_t grid_x;          ///< Grid X coordinate
    std::int32_t grid_y;          ///< Grid Y coordinate

    DebrisClearedEvent()
        : entity_id(0)
        , grid_x(0)
        , grid_y(0)
    {}

    DebrisClearedEvent(std::uint32_t entity, std::int32_t x, std::int32_t y)
        : entity_id(entity)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct BuildingUpgradedEvent
 * @brief Event emitted when a structure upgrades to a higher level.
 *
 * Emitted when BuildingComponent.level increases.
 * Higher levels provide more capacity and functionality.
 *
 * Consumed by:
 * - RenderingSystem: Update building visual (may change model)
 * - UISystem: Show upgrade notification
 * - AudioSystem: Play upgrade sound
 * - StatisticsSystem: Track upgrade count
 */
struct BuildingUpgradedEvent {
    std::uint32_t entity_id;      ///< Building entity ID
    std::uint8_t old_level;       ///< Previous level
    std::uint8_t new_level;       ///< New level

    BuildingUpgradedEvent()
        : entity_id(0)
        , old_level(0)
        , new_level(0)
    {}

    BuildingUpgradedEvent(std::uint32_t entity, std::uint8_t old_lvl, std::uint8_t new_lvl)
        : entity_id(entity)
        , old_level(old_lvl)
        , new_level(new_lvl)
    {}
};

/**
 * @struct BuildingDowngradedEvent
 * @brief Event emitted when a structure downgrades to a lower level.
 *
 * Emitted when BuildingComponent.level decreases (due to lack of services, etc.).
 * Lower levels provide less capacity and functionality.
 *
 * Consumed by:
 * - RenderingSystem: Update building visual (may change model)
 * - UISystem: Show downgrade notification
 * - AudioSystem: Play downgrade sound
 * - StatisticsSystem: Track downgrade count
 */
struct BuildingDowngradedEvent {
    std::uint32_t entity_id;      ///< Building entity ID
    std::uint8_t old_level;       ///< Previous level
    std::uint8_t new_level;       ///< New level

    BuildingDowngradedEvent()
        : entity_id(0)
        , old_level(0)
        , new_level(0)
    {}

    BuildingDowngradedEvent(std::uint32_t entity, std::uint8_t old_lvl, std::uint8_t new_lvl)
        : entity_id(entity)
        , old_level(old_lvl)
        , new_level(new_lvl)
    {}
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGEVENTS_H
