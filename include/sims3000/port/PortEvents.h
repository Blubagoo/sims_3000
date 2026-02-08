/**
 * @file PortEvents.h
 * @brief Port system event definitions for Epic 8 (Ticket E8-028)
 *
 * Defines all events emitted by the port system:
 * - PortOperationalEvent: Port became operational or non-operational
 * - PortUpgradedEvent: Port upgrade level changed
 * - PortCapacityChangedEvent: Port throughput capacity changed
 * - ExternalConnectionCreatedEvent: External connection established at map edge
 * - ExternalConnectionRemovedEvent: External connection removed from map edge
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <cstdint>

namespace sims3000 {
namespace port {

/**
 * @struct PortOperationalEvent
 * @brief Event emitted when a port becomes operational or non-operational.
 *
 * Emitted when a port facility completes construction and becomes active,
 * or when it is deactivated due to damage, lack of resources, or demolition.
 *
 * Consumed by:
 * - UISystem: Show port status notification
 * - EconomySystem: Enable/disable trade routes through this port
 * - StatisticsSystem: Track port operational status
 */
struct PortOperationalEvent {
    uint32_t port;              ///< Port entity ID
    bool is_operational;        ///< Whether the port is now operational
    uint8_t owner;              ///< Owning overseer PlayerID

    PortOperationalEvent()
        : port(0)
        , is_operational(false)
        , owner(0)
    {}

    PortOperationalEvent(uint32_t port_id, bool operational, uint8_t owner_id)
        : port(port_id)
        , is_operational(operational)
        , owner(owner_id)
    {}
};

/**
 * @struct PortUpgradedEvent
 * @brief Event emitted when a port's upgrade level changes.
 *
 * Emitted when a port facility is upgraded to a higher level, increasing
 * its capabilities, throughput, and visual appearance.
 *
 * Consumed by:
 * - UISystem: Show upgrade notification
 * - RenderingSystem: Update port visual to new level
 * - EconomySystem: Recalculate trade capacity
 */
struct PortUpgradedEvent {
    uint32_t port;          ///< Port entity ID
    uint8_t old_level;      ///< Previous upgrade level
    uint8_t new_level;      ///< New upgrade level

    PortUpgradedEvent()
        : port(0)
        , old_level(0)
        , new_level(0)
    {}

    PortUpgradedEvent(uint32_t port_id, uint8_t old_lvl, uint8_t new_lvl)
        : port(port_id)
        , old_level(old_lvl)
        , new_level(new_lvl)
    {}
};

/**
 * @struct PortCapacityChangedEvent
 * @brief Event emitted when a port's throughput capacity changes.
 *
 * Emitted when upgrades, damage, or configuration changes alter the
 * maximum throughput capacity of a port facility.
 *
 * Consumed by:
 * - UISystem: Update port info panel capacity display
 * - EconomySystem: Recalculate trade flow limits
 * - StatisticsSystem: Track capacity changes
 */
struct PortCapacityChangedEvent {
    uint32_t port;              ///< Port entity ID
    uint32_t old_capacity;      ///< Previous throughput capacity
    uint32_t new_capacity;      ///< New throughput capacity

    PortCapacityChangedEvent()
        : port(0)
        , old_capacity(0)
        , new_capacity(0)
    {}

    PortCapacityChangedEvent(uint32_t port_id, uint32_t old_cap, uint32_t new_cap)
        : port(port_id)
        , old_capacity(old_cap)
        , new_capacity(new_cap)
    {}
};

/**
 * @struct ExternalConnectionCreatedEvent
 * @brief Event emitted when an external connection is established at a map edge.
 *
 * Emitted when a new connection to the outside world is created,
 * enabling trade, migration, or resource flow across the map boundary.
 *
 * Consumed by:
 * - UISystem: Show connection notification on map edge
 * - RenderingSystem: Draw connection visual at map boundary
 * - TransportSystem: Update pathfinding to include external routes
 * - EconomySystem: Enable trade routes through this connection
 */
struct ExternalConnectionCreatedEvent {
    uint32_t connection;        ///< Connection entity ID
    MapEdge edge;               ///< Map edge where connection is placed
    ConnectionType type;        ///< Type of connection established

    ExternalConnectionCreatedEvent()
        : connection(0)
        , edge(MapEdge::North)
        , type(ConnectionType::Pathway)
    {}

    ExternalConnectionCreatedEvent(uint32_t conn_id, MapEdge map_edge,
                                   ConnectionType conn_type)
        : connection(conn_id)
        , edge(map_edge)
        , type(conn_type)
    {}
};

/**
 * @struct ExternalConnectionRemovedEvent
 * @brief Event emitted when an external connection is removed from a map edge.
 *
 * Emitted when a connection to the outside world is destroyed or
 * decommissioned, disabling the associated trade/flow route.
 *
 * Consumed by:
 * - UISystem: Remove connection indicator from map edge
 * - RenderingSystem: Remove connection visual
 * - TransportSystem: Remove external routes from pathfinding
 * - EconomySystem: Disable trade routes through this connection
 */
struct ExternalConnectionRemovedEvent {
    uint32_t connection;    ///< Connection entity ID
    MapEdge edge;           ///< Map edge where connection was located

    ExternalConnectionRemovedEvent()
        : connection(0)
        , edge(MapEdge::North)
    {}

    ExternalConnectionRemovedEvent(uint32_t conn_id, MapEdge map_edge)
        : connection(conn_id)
        , edge(map_edge)
    {}
};

} // namespace port
} // namespace sims3000
