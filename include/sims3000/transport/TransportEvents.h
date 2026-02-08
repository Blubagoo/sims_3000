/**
 * @file TransportEvents.h
 * @brief Transport system event definitions for Epic 7 (Ticket E7-004)
 *
 * Defines all events emitted by the transport system:
 * - PathwayPlacedEvent: Pathway placed on grid
 * - PathwayRemovedEvent: Pathway removed from grid
 * - PathwayDeterioratedEvent: Pathway health decreased
 * - PathwayRepairedEvent: Pathway health restored
 * - NetworkConnectedEvent: Transport network connected players
 * - NetworkDisconnectedEvent: Transport network split
 * - FlowBlockageBeganEvent: Traffic flow blockage started
 * - FlowBlockageEndedEvent: Traffic flow blockage ended
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/TransportEnums.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace transport {

/**
 * @struct PathwayPlacedEvent
 * @brief Event emitted when a pathway is placed on the grid.
 *
 * Consumed by:
 * - RenderingSystem: Show pathway visual
 * - TransportSystem: Recalculate network connectivity
 * - UISystem: Update transport overlay
 * - EconomySystem: Deduct build cost
 */
struct PathwayPlacedEvent {
    uint32_t entity_id;     ///< Pathway entity ID
    uint32_t x;             ///< Grid X coordinate
    uint32_t y;             ///< Grid Y coordinate
    PathwayType type;       ///< Type of pathway placed
    uint8_t owner;          ///< Owning overseer PlayerID

    PathwayPlacedEvent()
        : entity_id(0)
        , x(0)
        , y(0)
        , type(PathwayType::BasicPathway)
        , owner(0)
    {}

    PathwayPlacedEvent(uint32_t entity, uint32_t gx, uint32_t gy,
                       PathwayType t, uint8_t own)
        : entity_id(entity)
        , x(gx)
        , y(gy)
        , type(t)
        , owner(own)
    {}
};

/**
 * @struct PathwayRemovedEvent
 * @brief Event emitted when a pathway is removed from the grid.
 *
 * Consumed by:
 * - RenderingSystem: Remove pathway visual
 * - TransportSystem: Recalculate network connectivity
 * - UISystem: Update transport overlay
 */
struct PathwayRemovedEvent {
    uint32_t entity_id;     ///< Pathway entity ID
    uint32_t x;             ///< Grid X coordinate
    uint32_t y;             ///< Grid Y coordinate
    uint8_t owner;          ///< Owning overseer PlayerID

    PathwayRemovedEvent()
        : entity_id(0)
        , x(0)
        , y(0)
        , owner(0)
    {}

    PathwayRemovedEvent(uint32_t entity, uint32_t gx, uint32_t gy,
                        uint8_t own)
        : entity_id(entity)
        , x(gx)
        , y(gy)
        , owner(own)
    {}
};

/**
 * @struct PathwayDeterioratedEvent
 * @brief Event emitted when a pathway's health decreases.
 *
 * Emitted when wear, age, or damage reduces pathway health.
 *
 * Consumed by:
 * - RenderingSystem: Update pathway visual (cracks, damage)
 * - UISystem: Show deterioration warning
 * - StatisticsSystem: Track infrastructure health
 */
struct PathwayDeterioratedEvent {
    uint32_t entity_id;     ///< Pathway entity ID
    uint32_t x;             ///< Grid X coordinate
    uint32_t y;             ///< Grid Y coordinate
    uint8_t new_health;     ///< New health value (0-255)

    PathwayDeterioratedEvent()
        : entity_id(0)
        , x(0)
        , y(0)
        , new_health(0)
    {}

    PathwayDeterioratedEvent(uint32_t entity, uint32_t gx, uint32_t gy,
                             uint8_t health)
        : entity_id(entity)
        , x(gx)
        , y(gy)
        , new_health(health)
    {}
};

/**
 * @struct PathwayRepairedEvent
 * @brief Event emitted when a pathway's health is restored.
 *
 * Emitted when maintenance or repair actions restore pathway health.
 *
 * Consumed by:
 * - RenderingSystem: Update pathway visual (restored appearance)
 * - UISystem: Clear deterioration warning
 * - EconomySystem: Deduct repair cost
 */
struct PathwayRepairedEvent {
    uint32_t entity_id;     ///< Pathway entity ID
    uint32_t x;             ///< Grid X coordinate
    uint32_t y;             ///< Grid Y coordinate
    uint8_t new_health;     ///< New health value (0-255)

    PathwayRepairedEvent()
        : entity_id(0)
        , x(0)
        , y(0)
        , new_health(0)
    {}

    PathwayRepairedEvent(uint32_t entity, uint32_t gx, uint32_t gy,
                         uint8_t health)
        : entity_id(entity)
        , x(gx)
        , y(gy)
        , new_health(health)
    {}
};

/**
 * @struct NetworkConnectedEvent
 * @brief Event emitted when a transport network connects players.
 *
 * Emitted when pathway placement creates connectivity between
 * previously disconnected player territories.
 *
 * Consumed by:
 * - UISystem: Show network connection notification
 * - TransportSystem: Update pathfinding caches
 * - StatisticsSystem: Track connectivity events
 */
struct NetworkConnectedEvent {
    uint32_t network_id;                    ///< Network ID that connected
    std::vector<uint8_t> connected_players; ///< Player IDs now connected

    NetworkConnectedEvent()
        : network_id(0)
        , connected_players()
    {}

    NetworkConnectedEvent(uint32_t net_id,
                          const std::vector<uint8_t>& players)
        : network_id(net_id)
        , connected_players(players)
    {}
};

/**
 * @struct NetworkDisconnectedEvent
 * @brief Event emitted when a transport network splits.
 *
 * Emitted when pathway removal causes a network to split into two
 * separate networks.
 *
 * Consumed by:
 * - UISystem: Show network disconnection warning
 * - TransportSystem: Invalidate pathfinding caches
 * - StatisticsSystem: Track disconnection events
 */
struct NetworkDisconnectedEvent {
    uint32_t old_id;        ///< Original network ID before split
    uint32_t new_id_a;      ///< First new network ID after split
    uint32_t new_id_b;      ///< Second new network ID after split

    NetworkDisconnectedEvent()
        : old_id(0)
        , new_id_a(0)
        , new_id_b(0)
    {}

    NetworkDisconnectedEvent(uint32_t old, uint32_t a, uint32_t b)
        : old_id(old)
        , new_id_a(a)
        , new_id_b(b)
    {}
};

/**
 * @struct FlowBlockageBeganEvent
 * @brief Event emitted when traffic flow blockage begins on a pathway.
 *
 * Emitted when congestion on a pathway reaches the blockage threshold,
 * preventing normal traffic flow.
 *
 * Consumed by:
 * - RenderingSystem: Show blockage visual indicator
 * - UISystem: Show traffic blockage warning
 * - TransportSystem: Reroute traffic around blockage
 */
struct FlowBlockageBeganEvent {
    uint32_t pathway_entity;    ///< Blocked pathway entity ID
    uint32_t x;                 ///< Grid X coordinate
    uint32_t y;                 ///< Grid Y coordinate
    uint8_t congestion_level;   ///< Congestion level at blockage start

    FlowBlockageBeganEvent()
        : pathway_entity(0)
        , x(0)
        , y(0)
        , congestion_level(0)
    {}

    FlowBlockageBeganEvent(uint32_t entity, uint32_t gx, uint32_t gy,
                           uint8_t congestion)
        : pathway_entity(entity)
        , x(gx)
        , y(gy)
        , congestion_level(congestion)
    {}
};

/**
 * @struct FlowBlockageEndedEvent
 * @brief Event emitted when traffic flow blockage ends on a pathway.
 *
 * Emitted when congestion on a pathway drops below the blockage threshold,
 * restoring normal traffic flow.
 *
 * Consumed by:
 * - RenderingSystem: Remove blockage visual indicator
 * - UISystem: Clear traffic blockage warning
 * - TransportSystem: Restore normal routing
 */
struct FlowBlockageEndedEvent {
    uint32_t pathway_entity;    ///< Unblocked pathway entity ID
    uint32_t x;                 ///< Grid X coordinate
    uint32_t y;                 ///< Grid Y coordinate

    FlowBlockageEndedEvent()
        : pathway_entity(0)
        , x(0)
        , y(0)
    {}

    FlowBlockageEndedEvent(uint32_t entity, uint32_t gx, uint32_t gy)
        : pathway_entity(entity)
        , x(gx)
        , y(gy)
    {}
};

} // namespace transport
} // namespace sims3000
