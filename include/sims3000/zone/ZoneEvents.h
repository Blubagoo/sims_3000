/**
 * @file ZoneEvents.h
 * @brief Zone-related event definitions for Epic 4
 *
 * Defines all events emitted by ZoneSystem:
 * - ZoneDesignatedEvent: Zone placed by overseer
 * - ZoneUndesignatedEvent: Zone removed by overseer
 * - ZoneStateChangedEvent: Zone state transition (Designated/Occupied/Stalled)
 * - ZoneDemandChangedEvent: Demand values updated (per overseer)
 * - DemolitionRequestEvent: De-zone occupied sector (decoupled flow per CCR-012)
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#ifndef SIMS3000_ZONE_ZONEEVENTS_H
#define SIMS3000_ZONE_ZONEEVENTS_H

#include <sims3000/zone/ZoneTypes.h>
#include <cstdint>

namespace sims3000 {
namespace zone {

/**
 * @struct ZoneDesignatedEvent
 * @brief Event emitted when a zone is designated by an overseer.
 *
 * Emitted after successful zone placement. Consumed by:
 * - UISystem: Update zone overlay visualization
 * - AudioSystem: Play designation sound
 * - BuildingSystem: Trigger structure development checks
 */
struct ZoneDesignatedEvent {
    std::uint32_t entity_id;      ///< Zone entity ID
    std::int32_t grid_x;          ///< Grid X coordinate
    std::int32_t grid_y;          ///< Grid Y coordinate
    ZoneType zone_type;           ///< Zone type (Habitation/Exchange/Fabrication)
    ZoneDensity density;          ///< Density level (Low/High)
    std::uint8_t owner_id;        ///< Owning overseer PlayerID

    ZoneDesignatedEvent()
        : entity_id(0)
        , grid_x(0)
        , grid_y(0)
        , zone_type(ZoneType::Habitation)
        , density(ZoneDensity::LowDensity)
        , owner_id(0)
    {}

    ZoneDesignatedEvent(std::uint32_t entity, std::int32_t x, std::int32_t y,
                        ZoneType type, ZoneDensity dens, std::uint8_t owner)
        : entity_id(entity)
        , grid_x(x)
        , grid_y(y)
        , zone_type(type)
        , density(dens)
        , owner_id(owner)
    {}
};

/**
 * @struct ZoneUndesignatedEvent
 * @brief Event emitted when a zone is undesignated (removed) by an overseer.
 *
 * Emitted after successful zone removal. Consumed by:
 * - UISystem: Remove zone overlay visualization
 * - AudioSystem: Play undesignation sound
 * - BuildingSystem: Handle de-zone of occupied sectors
 */
struct ZoneUndesignatedEvent {
    std::uint32_t entity_id;      ///< Zone entity ID
    std::int32_t grid_x;          ///< Grid X coordinate
    std::int32_t grid_y;          ///< Grid Y coordinate
    ZoneType zone_type;           ///< Zone type (Habitation/Exchange/Fabrication)
    std::uint8_t owner_id;        ///< Owning overseer PlayerID

    ZoneUndesignatedEvent()
        : entity_id(0)
        , grid_x(0)
        , grid_y(0)
        , zone_type(ZoneType::Habitation)
        , owner_id(0)
    {}

    ZoneUndesignatedEvent(std::uint32_t entity, std::int32_t x, std::int32_t y,
                          ZoneType type, std::uint8_t owner)
        : entity_id(entity)
        , grid_x(x)
        , grid_y(y)
        , zone_type(type)
        , owner_id(owner)
    {}
};

/**
 * @struct ZoneStateChangedEvent
 * @brief Event emitted when a zone's state changes.
 *
 * Emitted when zone transitions between states:
 * - Designated -> Occupied (structure built)
 * - Occupied -> Designated (structure demolished, zone remains)
 * - Designated -> Stalled (development blocked)
 * - Stalled -> Designated (blockage removed)
 *
 * Consumed by:
 * - UISystem: Update zone overlay visualization
 * - StatisticsSystem: Track zone state counts
 */
struct ZoneStateChangedEvent {
    std::uint32_t entity_id;      ///< Zone entity ID
    std::int32_t grid_x;          ///< Grid X coordinate
    std::int32_t grid_y;          ///< Grid Y coordinate
    ZoneState old_state;          ///< Previous state
    ZoneState new_state;          ///< New state

    ZoneStateChangedEvent()
        : entity_id(0)
        , grid_x(0)
        , grid_y(0)
        , old_state(ZoneState::Designated)
        , new_state(ZoneState::Designated)
    {}

    ZoneStateChangedEvent(std::uint32_t entity, std::int32_t x, std::int32_t y,
                          ZoneState old_s, ZoneState new_s)
        : entity_id(entity)
        , grid_x(x)
        , grid_y(y)
        , old_state(old_s)
        , new_state(new_s)
    {}
};

/**
 * @struct ZoneDemandChangedEvent
 * @brief Event emitted when zone demand values change for an overseer.
 *
 * Emitted when ZoneSystem recalculates demand (per overseer).
 * Demand ranges from -100 (negative demand) to +100 (positive demand).
 *
 * Consumed by:
 * - UISystem: Update demand meter visualization
 * - BuildingSystem: Influence structure development priority
 */
struct ZoneDemandChangedEvent {
    std::uint8_t player_id;       ///< Overseer PlayerID
    ZoneDemandData demand;        ///< Updated demand values

    ZoneDemandChangedEvent()
        : player_id(0)
        , demand()
    {}

    ZoneDemandChangedEvent(std::uint8_t player, const ZoneDemandData& d)
        : player_id(player)
        , demand(d)
    {}
};

/**
 * @struct DemolitionRequestEvent
 * @brief Event emitted when de-zoning an occupied sector (CCR-012 decoupled flow).
 *
 * Per CCR-012, de-zoning an occupied sector emits this event rather than
 * directly calling BuildingSystem. This decouples ZoneSystem from BuildingSystem.
 *
 * Flow:
 * 1. Overseer de-zones occupied sector
 * 2. ZoneSystem emits DemolitionRequestEvent
 * 3. BuildingSystem handles demolition
 * 4. BuildingSystem calls ZoneSystem.set_zone_state(Designated)
 * 5. ZoneSystem destroys zone entity
 *
 * Consumed by:
 * - BuildingSystem: Initiate building demolition
 */
struct DemolitionRequestEvent {
    std::int32_t grid_x;          ///< Grid X coordinate of zone to demolish
    std::int32_t grid_y;          ///< Grid Y coordinate of zone to demolish
    std::uint32_t requesting_entity_id;  ///< Zone entity requesting demolition

    DemolitionRequestEvent()
        : grid_x(0)
        , grid_y(0)
        , requesting_entity_id(0)
    {}

    DemolitionRequestEvent(std::int32_t x, std::int32_t y, std::uint32_t entity)
        : grid_x(x)
        , grid_y(y)
        , requesting_entity_id(entity)
    {}
};

} // namespace zone
} // namespace sims3000

#endif // SIMS3000_ZONE_ZONEEVENTS_H
