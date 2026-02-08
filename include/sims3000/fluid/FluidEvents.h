/**
 * @file FluidEvents.h
 * @brief Fluid system event definitions for Epic 6 (Ticket 6-007)
 *
 * Defines all events emitted by the fluid system:
 * - FluidStateChangedEvent: Entity fluid state changed
 * - FluidDeficitBeganEvent: Fluid pool entered deficit
 * - FluidDeficitEndedEvent: Fluid pool exited deficit
 * - FluidCollapseBeganEvent: Fluid grid entered collapse state
 * - FluidCollapseEndedEvent: Fluid grid exited collapse state
 * - FluidConduitPlacedEvent: Fluid conduit placed on grid
 * - FluidConduitRemovedEvent: Fluid conduit removed from grid
 * - ExtractorPlacedEvent: Fluid extractor placed on grid
 * - ExtractorRemovedEvent: Fluid extractor removed from grid
 * - ReservoirPlacedEvent: Fluid reservoir placed on grid
 * - ReservoirRemovedEvent: Fluid reservoir removed from grid
 * - ReservoirLevelChangedEvent: Reservoir fill level changed
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace fluid {

/**
 * @struct FluidStateChangedEvent
 * @brief Event emitted when an entity's fluid supply state changes.
 *
 * Emitted when a consumer entity transitions between having and not having fluid.
 *
 * Consumed by:
 * - RenderingSystem: Update fluid/no-fluid visual state
 * - UISystem: Update fluid overlay
 * - BuildingSystem: Trigger abandon timer if no fluid
 */
struct FluidStateChangedEvent {
    uint32_t entity_id;     ///< Entity whose fluid state changed
    uint8_t owner_id;       ///< Owning overseer PlayerID
    bool had_fluid;         ///< Previous fluid state
    bool has_fluid;         ///< New fluid state

    FluidStateChangedEvent()
        : entity_id(0)
        , owner_id(0)
        , had_fluid(false)
        , has_fluid(false)
    {}

    FluidStateChangedEvent(uint32_t entity, uint8_t owner,
                           bool had, bool has)
        : entity_id(entity)
        , owner_id(owner)
        , had_fluid(had)
        , has_fluid(has)
    {}
};

/**
 * @struct FluidDeficitBeganEvent
 * @brief Event emitted when a fluid pool enters deficit state.
 *
 * Emitted when total demand exceeds total supply for a player's fluid pool.
 *
 * Consumed by:
 * - UISystem: Show fluid deficit warning
 * - AudioSystem: Play deficit warning sound
 * - StatisticsSystem: Track deficit events
 */
struct FluidDeficitBeganEvent {
    uint8_t owner_id;           ///< Owning overseer PlayerID
    int32_t deficit_amount;     ///< Amount of fluid deficit (demand - supply)
    uint32_t affected_consumers; ///< Number of consumers affected by deficit

    FluidDeficitBeganEvent()
        : owner_id(0)
        , deficit_amount(0)
        , affected_consumers(0)
    {}

    FluidDeficitBeganEvent(uint8_t owner, int32_t deficit, uint32_t affected)
        : owner_id(owner)
        , deficit_amount(deficit)
        , affected_consumers(affected)
    {}
};

/**
 * @struct FluidDeficitEndedEvent
 * @brief Event emitted when a fluid pool exits deficit state.
 *
 * Emitted when total supply meets or exceeds total demand again.
 *
 * Consumed by:
 * - UISystem: Clear fluid deficit warning
 * - AudioSystem: Play deficit resolved sound
 */
struct FluidDeficitEndedEvent {
    uint8_t owner_id;       ///< Owning overseer PlayerID
    int32_t surplus_amount; ///< Amount of fluid surplus (supply - demand)

    FluidDeficitEndedEvent()
        : owner_id(0)
        , surplus_amount(0)
    {}

    FluidDeficitEndedEvent(uint8_t owner, int32_t surplus)
        : owner_id(owner)
        , surplus_amount(surplus)
    {}
};

/**
 * @struct FluidCollapseBeganEvent
 * @brief Event emitted when the fluid grid enters collapse state.
 *
 * Emitted when deficit reaches critical threshold, causing widespread outages.
 *
 * Consumed by:
 * - UISystem: Show collapse warning overlay
 * - AudioSystem: Play collapse alarm
 * - BuildingSystem: Trigger mass abandon timers
 */
struct FluidCollapseBeganEvent {
    uint8_t owner_id;       ///< Owning overseer PlayerID
    int32_t deficit_amount; ///< Amount of fluid deficit at collapse

    FluidCollapseBeganEvent()
        : owner_id(0)
        , deficit_amount(0)
    {}

    FluidCollapseBeganEvent(uint8_t owner, int32_t deficit)
        : owner_id(owner)
        , deficit_amount(deficit)
    {}
};

/**
 * @struct FluidCollapseEndedEvent
 * @brief Event emitted when the fluid grid exits collapse state.
 *
 * Emitted when supply recovers enough to end the collapse condition.
 *
 * Consumed by:
 * - UISystem: Clear collapse warning overlay
 * - AudioSystem: Play collapse resolved sound
 */
struct FluidCollapseEndedEvent {
    uint8_t owner_id;   ///< Owning overseer PlayerID

    FluidCollapseEndedEvent()
        : owner_id(0)
    {}

    explicit FluidCollapseEndedEvent(uint8_t owner)
        : owner_id(owner)
    {}
};

/**
 * @struct FluidConduitPlacedEvent
 * @brief Event emitted when a fluid conduit is placed on the grid.
 *
 * Consumed by:
 * - RenderingSystem: Show conduit visual
 * - FluidSystem: Recalculate connectivity
 * - UISystem: Update fluid overlay
 */
struct FluidConduitPlacedEvent {
    uint32_t entity_id; ///< Conduit entity ID
    uint8_t owner_id;   ///< Owning overseer PlayerID
    uint32_t grid_x;    ///< Grid X coordinate
    uint32_t grid_y;    ///< Grid Y coordinate

    FluidConduitPlacedEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
    {}

    FluidConduitPlacedEvent(uint32_t entity, uint8_t owner,
                            uint32_t x, uint32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct FluidConduitRemovedEvent
 * @brief Event emitted when a fluid conduit is removed from the grid.
 *
 * Consumed by:
 * - RenderingSystem: Remove conduit visual
 * - FluidSystem: Recalculate connectivity
 * - UISystem: Update fluid overlay
 */
struct FluidConduitRemovedEvent {
    uint32_t entity_id; ///< Conduit entity ID
    uint8_t owner_id;   ///< Owning overseer PlayerID
    uint32_t grid_x;    ///< Grid X coordinate
    uint32_t grid_y;    ///< Grid Y coordinate

    FluidConduitRemovedEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
    {}

    FluidConduitRemovedEvent(uint32_t entity, uint8_t owner,
                             uint32_t x, uint32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct ExtractorPlacedEvent
 * @brief Event emitted when a fluid extractor is placed on the grid.
 *
 * Consumed by:
 * - RenderingSystem: Show extractor visual
 * - FluidSystem: Add fluid source, recalculate supply
 * - UISystem: Update fluid overlay
 * - EconomySystem: Deduct build cost
 */
struct ExtractorPlacedEvent {
    uint32_t entity_id;      ///< Extractor entity ID
    uint8_t owner_id;        ///< Owning overseer PlayerID
    uint32_t grid_x;         ///< Grid X coordinate
    uint32_t grid_y;         ///< Grid Y coordinate
    uint8_t water_distance;  ///< Distance to nearest water source

    ExtractorPlacedEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
        , water_distance(0)
    {}

    ExtractorPlacedEvent(uint32_t entity, uint8_t owner,
                         uint32_t x, uint32_t y, uint8_t dist)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
        , water_distance(dist)
    {}
};

/**
 * @struct ExtractorRemovedEvent
 * @brief Event emitted when a fluid extractor is removed from the grid.
 *
 * Consumed by:
 * - RenderingSystem: Remove extractor visual
 * - FluidSystem: Remove fluid source, recalculate supply
 * - UISystem: Update fluid overlay
 */
struct ExtractorRemovedEvent {
    uint32_t entity_id; ///< Extractor entity ID
    uint8_t owner_id;   ///< Owning overseer PlayerID
    uint32_t grid_x;    ///< Grid X coordinate
    uint32_t grid_y;    ///< Grid Y coordinate

    ExtractorRemovedEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
    {}

    ExtractorRemovedEvent(uint32_t entity, uint8_t owner,
                          uint32_t x, uint32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct ReservoirPlacedEvent
 * @brief Event emitted when a fluid reservoir is placed on the grid.
 *
 * Consumed by:
 * - RenderingSystem: Show reservoir visual
 * - FluidSystem: Add reservoir to network
 * - UISystem: Update fluid overlay
 * - EconomySystem: Deduct build cost
 */
struct ReservoirPlacedEvent {
    uint32_t entity_id; ///< Reservoir entity ID
    uint8_t owner_id;   ///< Owning overseer PlayerID
    uint32_t grid_x;    ///< Grid X coordinate
    uint32_t grid_y;    ///< Grid Y coordinate

    ReservoirPlacedEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
    {}

    ReservoirPlacedEvent(uint32_t entity, uint8_t owner,
                         uint32_t x, uint32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct ReservoirRemovedEvent
 * @brief Event emitted when a fluid reservoir is removed from the grid.
 *
 * Consumed by:
 * - RenderingSystem: Remove reservoir visual
 * - FluidSystem: Remove reservoir from network
 * - UISystem: Update fluid overlay
 */
struct ReservoirRemovedEvent {
    uint32_t entity_id; ///< Reservoir entity ID
    uint8_t owner_id;   ///< Owning overseer PlayerID
    uint32_t grid_x;    ///< Grid X coordinate
    uint32_t grid_y;    ///< Grid Y coordinate

    ReservoirRemovedEvent()
        : entity_id(0)
        , owner_id(0)
        , grid_x(0)
        , grid_y(0)
    {}

    ReservoirRemovedEvent(uint32_t entity, uint8_t owner,
                          uint32_t x, uint32_t y)
        : entity_id(entity)
        , owner_id(owner)
        , grid_x(x)
        , grid_y(y)
    {}
};

/**
 * @struct ReservoirLevelChangedEvent
 * @brief Event emitted when a reservoir's fill level changes.
 *
 * Emitted when fluid flows in or out of a reservoir, changing its stored amount.
 *
 * Consumed by:
 * - UISystem: Update reservoir info panel level display
 * - FluidSystem: Recalculate available supply
 */
struct ReservoirLevelChangedEvent {
    uint32_t entity_id;  ///< Reservoir entity ID
    uint8_t owner_id;    ///< Owning overseer PlayerID
    uint32_t old_level;  ///< Previous fill level
    uint32_t new_level;  ///< New fill level

    ReservoirLevelChangedEvent()
        : entity_id(0)
        , owner_id(0)
        , old_level(0)
        , new_level(0)
    {}

    ReservoirLevelChangedEvent(uint32_t entity, uint8_t owner,
                               uint32_t old_lvl, uint32_t new_lvl)
        : entity_id(entity)
        , owner_id(owner)
        , old_level(old_lvl)
        , new_level(new_lvl)
    {}
};

} // namespace fluid
} // namespace sims3000
