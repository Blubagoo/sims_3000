/**
 * @file BuildingOccupancyComponent.h
 * @brief Per-building occupancy tracking (Ticket E10-012)
 *
 * ECS component for tracking building occupancy state.
 * Attached to individual building entities to track how
 * many beings currently occupy the building vs. its capacity.
 */

#ifndef SIMS3000_POPULATION_BUILDINGOCCUPANCYCOMPONENT_H
#define SIMS3000_POPULATION_BUILDINGOCCUPANCYCOMPONENT_H

#include <cstdint>

namespace sims3000 {
namespace population {

/**
 * @enum OccupancyState
 * @brief Discrete occupancy levels for buildings.
 *
 * Used for visual feedback and simulation logic:
 * - Empty: no occupants
 * - UnderOccupied: below 25% capacity
 * - NormalOccupied: 25-75% capacity
 * - FullyOccupied: 75-100% capacity
 * - Overcrowded: above 100% capacity
 */
enum class OccupancyState : uint8_t {
    Empty = 0,            ///< No occupants
    UnderOccupied = 1,    ///< Below 25% capacity
    NormalOccupied = 2,   ///< 25-75% capacity (healthy)
    FullyOccupied = 3,    ///< 75-100% capacity
    Overcrowded = 4       ///< Above 100% capacity (negative effects)
};

/**
 * @struct BuildingOccupancyComponent
 * @brief Tracks occupancy for a single building entity.
 *
 * Lightweight ECS component with capacity, current count,
 * occupancy state classification, and the tick at which
 * the occupancy last changed.
 *
 * Target size: 9 bytes.
 */
struct BuildingOccupancyComponent {
    uint16_t capacity = 0;              ///< Maximum occupant capacity
    uint16_t current_occupancy = 0;     ///< Current number of occupants
    OccupancyState state = OccupancyState::Empty;  ///< Current occupancy classification
    uint32_t occupancy_changed_tick = 0; ///< Tick when occupancy last changed
};

static_assert(sizeof(BuildingOccupancyComponent) <= 12, "BuildingOccupancyComponent should be approximately 9 bytes");

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_BUILDINGOCCUPANCYCOMPONENT_H
