/**
 * @file ServiceEvents.h
 * @brief Service-related event definitions for Epic 9 (Ticket E9-012)
 *
 * Defines all events emitted by the services system:
 * - ServiceBuildingPlacedEvent: Service building placed on map
 * - ServiceBuildingRemovedEvent: Service building removed from map
 * - ServiceEffectivenessChangedEvent: Service building effectiveness changed
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/services/ServiceTypes.h>
#include <cstdint>

namespace sims3000 {
namespace services {

/**
 * @struct ServiceBuildingPlacedEvent
 * @brief Event emitted when a service building is placed on the map.
 *
 * Emitted when a service facility (Enforcer Post, Medical Station, etc.)
 * is constructed and begins providing coverage.
 *
 * Consumed by:
 * - ServicesSystem: Add to tracked entities, recalculate coverage
 * - UISystem: Update service overlay
 * - StatisticsSystem: Track service building count
 */
struct ServiceBuildingPlacedEvent {
    uint32_t    entity_id;      ///< Service building entity ID
    uint8_t     owner_id;       ///< Owning overseer PlayerID
    ServiceType service_type;   ///< Service classification
    ServiceTier tier;           ///< Facility tier (Post/Station/Nexus)
    int32_t     grid_x;         ///< Grid X coordinate
    int32_t     grid_y;         ///< Grid Y coordinate

    ServiceBuildingPlacedEvent()
        : entity_id(0)
        , owner_id(0)
        , service_type(ServiceType::Enforcer)
        , tier(ServiceTier::Post)
        , grid_x(0)
        , grid_y(0)
    {}

    ServiceBuildingPlacedEvent(uint32_t entity, uint8_t owner,
                               ServiceType type, ServiceTier t,
                               int32_t x, int32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , service_type(type)
        , tier(t)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct ServiceBuildingRemovedEvent
 * @brief Event emitted when a service building is removed from the map.
 *
 * Emitted when a service facility is demolished or destroyed,
 * ceasing to provide coverage.
 *
 * Consumed by:
 * - ServicesSystem: Remove from tracked entities, recalculate coverage
 * - UISystem: Update service overlay
 * - StatisticsSystem: Track service building count
 */
struct ServiceBuildingRemovedEvent {
    uint32_t    entity_id;      ///< Service building entity ID
    uint8_t     owner_id;       ///< Owning overseer PlayerID
    ServiceType service_type;   ///< Service classification
    ServiceTier tier;           ///< Facility tier (Post/Station/Nexus)
    int32_t     grid_x;         ///< Grid X coordinate
    int32_t     grid_y;         ///< Grid Y coordinate

    ServiceBuildingRemovedEvent()
        : entity_id(0)
        , owner_id(0)
        , service_type(ServiceType::Enforcer)
        , tier(ServiceTier::Post)
        , grid_x(0)
        , grid_y(0)
    {}

    ServiceBuildingRemovedEvent(uint32_t entity, uint8_t owner,
                                ServiceType type, ServiceTier t,
                                int32_t x, int32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , service_type(type)
        , tier(t)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct ServiceEffectivenessChangedEvent
 * @brief Event emitted when a service building's effectiveness changes.
 *
 * Emitted when power changes, funding changes, or other factors
 * alter a service building's operational effectiveness.
 *
 * Consumed by:
 * - ServicesSystem: Mark coverage dirty, recalculate on next tick
 * - UISystem: Update service building info panel
 * - StatisticsSystem: Track effectiveness changes
 */
struct ServiceEffectivenessChangedEvent {
    uint32_t    entity_id;      ///< Service building entity ID
    uint8_t     owner_id;       ///< Owning overseer PlayerID
    ServiceType service_type;   ///< Service classification
    ServiceTier tier;           ///< Facility tier (Post/Station/Nexus)
    int32_t     grid_x;         ///< Grid X coordinate
    int32_t     grid_y;         ///< Grid Y coordinate

    ServiceEffectivenessChangedEvent()
        : entity_id(0)
        , owner_id(0)
        , service_type(ServiceType::Enforcer)
        , tier(ServiceTier::Post)
        , grid_x(0)
        , grid_y(0)
    {}

    ServiceEffectivenessChangedEvent(uint32_t entity, uint8_t owner,
                                      ServiceType type, ServiceTier t,
                                      int32_t x, int32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , service_type(type)
        , tier(t)
        , grid_x(x)
        , grid_y(y)
    {}
};

} // namespace services
} // namespace sims3000
