/**
 * @file PerPlayerFluidPool.h
 * @brief Per-player fluid pool aggregate for Epic 6 (Ticket 6-006)
 *
 * Defines:
 * - PerPlayerFluidPool: Tracks aggregate fluid supply/demand per player
 *
 * One PerPlayerFluidPool exists per player (overseer). The fluid
 * distribution system updates it each tick by summing all extractor
 * outputs, reservoir levels, and consumer demands within that
 * player's territory.
 *
 * Unlike energy, fluid tracks reservoir storage separately from
 * generation, and available = total_generated + total_reservoir_stored.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/fluid/FluidEnums.h>
#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace fluid {

/**
 * @struct PerPlayerFluidPool
 * @brief Aggregate fluid supply/demand tracking per player (40 bytes).
 *
 * Summarizes the fluid situation for one player's city:
 * - Total generation from all operational extractors
 * - Total reservoir stored and capacity
 * - Available supply (generated + reservoir stored)
 * - Total consumption from all consumers in coverage
 * - Surplus/deficit calculation
 * - Pool health state (Healthy/Marginal/Deficit/Collapse)
 *
 * Layout (40 bytes, natural alignment):
 * - total_generated:          4 bytes (uint32_t) - sum of operational extractor outputs
 * - total_reservoir_stored:   4 bytes (uint32_t) - sum of all reservoir current levels
 * - total_reservoir_capacity: 4 bytes (uint32_t) - sum of all reservoir max capacities
 * - available:                4 bytes (uint32_t) - total_generated + total_reservoir_stored
 * - total_consumed:           4 bytes (uint32_t) - sum of consumer fluid_required in coverage
 * - surplus:                  4 bytes (int32_t)  - available - total_consumed (can be negative)
 * - extractor_count:          4 bytes (uint32_t) - operational extractors
 * - reservoir_count:          4 bytes (uint32_t) - reservoirs
 * - consumer_count:           4 bytes (uint32_t) - consumers in coverage
 * - state:                    1 byte  (FluidPoolState) - current pool health state
 * - previous_state:           1 byte  (FluidPoolState) - previous tick state
 * - _padding:                 2 bytes (uint8_t[2]) - alignment padding
 *
 * Total: 40 bytes
 */
struct PerPlayerFluidPool {
    uint32_t total_generated = 0;           ///< Sum of operational extractor outputs
    uint32_t total_reservoir_stored = 0;    ///< Sum of all reservoir current levels
    uint32_t total_reservoir_capacity = 0;  ///< Sum of all reservoir max capacities
    uint32_t available = 0;                 ///< total_generated + total_reservoir_stored
    uint32_t total_consumed = 0;            ///< Sum of consumer fluid_required in coverage
    int32_t surplus = 0;                    ///< available - total_consumed (can be negative)
    uint32_t extractor_count = 0;           ///< Operational extractors
    uint32_t reservoir_count = 0;           ///< Reservoirs
    uint32_t consumer_count = 0;            ///< Consumers in coverage
    FluidPoolState state = FluidPoolState::Healthy;           ///< Current pool health state
    FluidPoolState previous_state = FluidPoolState::Healthy;  ///< Previous tick pool health state
    uint8_t _padding[2] = {0, 0};           ///< Alignment padding

    /**
     * @brief Reset all fields to default/zero values.
     *
     * Called at the start of each tick before recalculating aggregates.
     */
    void clear() {
        total_generated = 0;
        total_reservoir_stored = 0;
        total_reservoir_capacity = 0;
        available = 0;
        total_consumed = 0;
        surplus = 0;
        extractor_count = 0;
        reservoir_count = 0;
        consumer_count = 0;
        state = FluidPoolState::Healthy;
        previous_state = FluidPoolState::Healthy;
        _padding[0] = 0;
        _padding[1] = 0;
    }
};

// Verify PerPlayerFluidPool size (40 bytes)
static_assert(sizeof(PerPlayerFluidPool) == 40,
    "PerPlayerFluidPool must be 40 bytes");

// Verify PerPlayerFluidPool is trivially copyable for serialization
static_assert(std::is_trivially_copyable<PerPlayerFluidPool>::value,
    "PerPlayerFluidPool must be trivially copyable");

} // namespace fluid
} // namespace sims3000
