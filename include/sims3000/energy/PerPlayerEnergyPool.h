/**
 * @file PerPlayerEnergyPool.h
 * @brief Per-player energy pool aggregate for Epic 5 (Ticket 5-005)
 *
 * Defines:
 * - PerPlayerEnergyPool: Tracks aggregate energy supply/demand per player
 *
 * One PerPlayerEnergyPool exists per player (overseer). The energy
 * distribution system updates it each tick by summing all nexus outputs
 * and consumer demands within that player's territory.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/core/types.h>
#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace energy {

/**
 * @struct PerPlayerEnergyPool
 * @brief Aggregate energy supply/demand tracking per player (32 bytes).
 *
 * Summarizes the energy situation for one player's city:
 * - Total generation from all nexuses
 * - Total consumption from all powered structures
 * - Surplus/deficit calculation
 * - Pool health state (Healthy/Marginal/Deficit/Collapse)
 *
 * Layout (32 bytes, natural alignment):
 * - total_generated:   4 bytes (uint32_t) - sum of all nexus current_output
 * - total_consumed:    4 bytes (uint32_t) - sum of all consumer energy_required in coverage
 * - surplus:           4 bytes (int32_t)  - generated - consumed (can be negative)
 * - nexus_count:       4 bytes (uint32_t) - number of active nexuses
 * - consumer_count:    4 bytes (uint32_t) - number of consumers in coverage
 * - owner:             1 byte  (PlayerID) - overseer who owns this pool
 * - state:             1 byte  (EnergyPoolState) - current pool health state
 * - previous_state:    1 byte  (EnergyPoolState) - previous tick pool health state
 * - _padding:          1 byte  (uint8_t)  - alignment padding
 *
 * Note: Fields ordered with uint32_t members first for natural alignment.
 * Actual size: 24 bytes.
 */
struct PerPlayerEnergyPool {
    uint32_t total_generated = 0;       ///< Sum of all nexus current_output
    uint32_t total_consumed = 0;        ///< Sum of all consumer energy_required in coverage
    int32_t surplus = 0;                ///< generated - consumed (can be negative)
    uint32_t nexus_count = 0;           ///< Number of active nexuses
    uint32_t consumer_count = 0;        ///< Number of consumers in coverage
    PlayerID owner = 0;                 ///< Overseer who owns this pool
    EnergyPoolState state = EnergyPoolState::Healthy;           ///< Current pool health state
    EnergyPoolState previous_state = EnergyPoolState::Healthy;  ///< Previous tick pool health state
    uint8_t _padding = 0;               ///< Alignment padding
};

// Document actual size: 24 bytes (5x uint32_t/int32_t = 20 bytes + 4x uint8_t = 4 bytes)
static_assert(sizeof(PerPlayerEnergyPool) == 24,
    "PerPlayerEnergyPool should be 24 bytes");

// Verify PerPlayerEnergyPool is trivially copyable for serialization
static_assert(std::is_trivially_copyable<PerPlayerEnergyPool>::value,
    "PerPlayerEnergyPool must be trivially copyable");

} // namespace energy
} // namespace sims3000
