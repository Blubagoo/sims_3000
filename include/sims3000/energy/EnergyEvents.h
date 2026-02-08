/**
 * @file EnergyEvents.h
 * @brief Energy system event definitions for Epic 5
 *
 * Defines all events emitted by the energy system:
 * - EnergyStateChangedEvent: Entity powered state changed
 * - EnergyDeficitBeganEvent: Energy pool entered deficit
 * - EnergyDeficitEndedEvent: Energy pool exited deficit
 * - GridCollapseBeganEvent: Energy grid entered collapse state
 * - GridCollapseEndedEvent: Energy grid exited collapse state
 * - ConduitPlacedEvent: Energy conduit placed on grid
 * - ConduitRemovedEvent: Energy conduit removed from grid
 * - NexusPlacedEvent: Energy nexus placed on grid
 * - NexusRemovedEvent: Energy nexus removed from grid
 * - NexusAgedEvent: Nexus efficiency changed due to aging
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace energy {

/**
 * @struct EnergyStateChangedEvent
 * @brief Event emitted when an entity's powered state changes.
 *
 * Emitted when a consumer entity transitions between powered and unpowered.
 *
 * Consumed by:
 * - RenderingSystem: Update powered/unpowered visual state
 * - UISystem: Update energy overlay
 * - BuildingSystem: Trigger abandon timer if unpowered
 */
struct EnergyStateChangedEvent {
    uint32_t entity_id;     ///< Entity whose power state changed
    uint8_t owner_id;       ///< Owning overseer PlayerID
    bool was_powered;       ///< Previous powered state
    bool is_powered;        ///< New powered state

    EnergyStateChangedEvent()
        : entity_id(0)
        , owner_id(0)
        , was_powered(false)
        , is_powered(false)
    {}

    EnergyStateChangedEvent(uint32_t entity, uint8_t owner,
                            bool was_pow, bool is_pow)
        : entity_id(entity)
        , owner_id(owner)
        , was_powered(was_pow)
        , is_powered(is_pow)
    {}
};

/**
 * @struct EnergyDeficitBeganEvent
 * @brief Event emitted when an energy pool enters deficit state.
 *
 * Emitted when total demand exceeds total supply for a player's energy pool.
 *
 * Consumed by:
 * - UISystem: Show deficit warning
 * - AudioSystem: Play deficit warning sound
 * - StatisticsSystem: Track deficit events
 */
struct EnergyDeficitBeganEvent {
    uint8_t owner_id;           ///< Owning overseer PlayerID
    int32_t deficit_amount;     ///< Amount of energy deficit (demand - supply)
    uint32_t affected_consumers; ///< Number of consumers affected by deficit

    EnergyDeficitBeganEvent()
        : owner_id(0)
        , deficit_amount(0)
        , affected_consumers(0)
    {}

    EnergyDeficitBeganEvent(uint8_t owner, int32_t deficit, uint32_t affected)
        : owner_id(owner)
        , deficit_amount(deficit)
        , affected_consumers(affected)
    {}
};

/**
 * @struct EnergyDeficitEndedEvent
 * @brief Event emitted when an energy pool exits deficit state.
 *
 * Emitted when total supply meets or exceeds total demand again.
 *
 * Consumed by:
 * - UISystem: Clear deficit warning
 * - AudioSystem: Play deficit resolved sound
 */
struct EnergyDeficitEndedEvent {
    uint8_t owner_id;       ///< Owning overseer PlayerID
    int32_t surplus_amount; ///< Amount of energy surplus (supply - demand)

    EnergyDeficitEndedEvent()
        : owner_id(0)
        , surplus_amount(0)
    {}

    EnergyDeficitEndedEvent(uint8_t owner, int32_t surplus)
        : owner_id(owner)
        , surplus_amount(surplus)
    {}
};

/**
 * @struct GridCollapseBeganEvent
 * @brief Event emitted when the energy grid enters collapse state.
 *
 * Emitted when deficit reaches critical threshold, causing widespread outages.
 *
 * Consumed by:
 * - UISystem: Show collapse warning overlay
 * - AudioSystem: Play collapse alarm
 * - BuildingSystem: Trigger mass abandon timers
 */
struct GridCollapseBeganEvent {
    uint8_t owner_id;       ///< Owning overseer PlayerID
    int32_t deficit_amount; ///< Amount of energy deficit at collapse

    GridCollapseBeganEvent()
        : owner_id(0)
        , deficit_amount(0)
    {}

    GridCollapseBeganEvent(uint8_t owner, int32_t deficit)
        : owner_id(owner)
        , deficit_amount(deficit)
    {}
};

/**
 * @struct GridCollapseEndedEvent
 * @brief Event emitted when the energy grid exits collapse state.
 *
 * Emitted when supply recovers enough to end the collapse condition.
 *
 * Consumed by:
 * - UISystem: Clear collapse warning overlay
 * - AudioSystem: Play collapse resolved sound
 */
struct GridCollapseEndedEvent {
    uint8_t owner_id;   ///< Owning overseer PlayerID

    GridCollapseEndedEvent()
        : owner_id(0)
    {}

    explicit GridCollapseEndedEvent(uint8_t owner)
        : owner_id(owner)
    {}
};

/**
 * @struct ConduitPlacedEvent
 * @brief Event emitted when an energy conduit is placed on the grid.
 *
 * Consumed by:
 * - RenderingSystem: Show conduit visual
 * - EnergySystem: Recalculate connectivity
 * - UISystem: Update energy overlay
 */
struct ConduitPlacedEvent {
    uint32_t entity_id; ///< Conduit entity ID
    uint8_t owner_id;   ///< Owning overseer PlayerID
    int32_t grid_x;     ///< Grid X coordinate
    int32_t grid_y;     ///< Grid Y coordinate

    ConduitPlacedEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
    {}

    ConduitPlacedEvent(uint32_t entity, uint8_t owner,
                       int32_t x, int32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct ConduitRemovedEvent
 * @brief Event emitted when an energy conduit is removed from the grid.
 *
 * Consumed by:
 * - RenderingSystem: Remove conduit visual
 * - EnergySystem: Recalculate connectivity
 * - UISystem: Update energy overlay
 */
struct ConduitRemovedEvent {
    uint32_t entity_id; ///< Conduit entity ID
    uint8_t owner_id;   ///< Owning overseer PlayerID
    int32_t grid_x;     ///< Grid X coordinate
    int32_t grid_y;     ///< Grid Y coordinate

    ConduitRemovedEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
    {}

    ConduitRemovedEvent(uint32_t entity, uint8_t owner,
                        int32_t x, int32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct NexusPlacedEvent
 * @brief Event emitted when an energy nexus is placed on the grid.
 *
 * Consumed by:
 * - RenderingSystem: Show nexus visual
 * - EnergySystem: Add energy source, recalculate supply
 * - UISystem: Update energy overlay
 * - EconomySystem: Deduct build cost
 */
struct NexusPlacedEvent {
    uint32_t entity_id; ///< Nexus entity ID
    uint8_t owner_id;   ///< Owning overseer PlayerID
    uint8_t nexus_type; ///< NexusType enum value
    int32_t grid_x;     ///< Grid X coordinate
    int32_t grid_y;     ///< Grid Y coordinate

    NexusPlacedEvent()
        : entity_id(0)
        , owner_id(0)
        , nexus_type(0)
        , grid_x(0)
        , grid_y(0)
    {}

    NexusPlacedEvent(uint32_t entity, uint8_t owner, uint8_t type,
                     int32_t x, int32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , nexus_type(type)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct NexusRemovedEvent
 * @brief Event emitted when an energy nexus is removed from the grid.
 *
 * Consumed by:
 * - RenderingSystem: Remove nexus visual
 * - EnergySystem: Remove energy source, recalculate supply
 * - UISystem: Update energy overlay
 */
struct NexusRemovedEvent {
    uint32_t entity_id; ///< Nexus entity ID
    uint8_t owner_id;   ///< Owning overseer PlayerID
    int32_t grid_x;     ///< Grid X coordinate
    int32_t grid_y;     ///< Grid Y coordinate

    NexusRemovedEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
    {}

    NexusRemovedEvent(uint32_t entity, uint8_t owner,
                      int32_t x, int32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct NexusAgedEvent
 * @brief Event emitted when a nexus's efficiency changes due to aging.
 *
 * Nexus facilities degrade over time, reducing their effective output.
 * Efficiency approaches the aging_floor asymptotically.
 *
 * Consumed by:
 * - UISystem: Update nexus info panel efficiency display
 * - EnergySystem: Recalculate effective supply
 */
struct NexusAgedEvent {
    uint32_t entity_id;     ///< Nexus entity ID
    uint8_t owner_id;       ///< Owning overseer PlayerID
    float new_efficiency;   ///< New efficiency value (0.0 - 1.0)

    NexusAgedEvent()
        : entity_id(0)
        , owner_id(0)
        , new_efficiency(1.0f)
    {}

    NexusAgedEvent(uint32_t entity, uint8_t owner, float efficiency)
        : entity_id(entity)
        , owner_id(owner)
        , new_efficiency(efficiency)
    {}
};

} // namespace energy
} // namespace sims3000
