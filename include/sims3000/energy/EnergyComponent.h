/**
 * @file EnergyComponent.h
 * @brief Energy consumer component for Epic 5 (Ticket 5-002)
 *
 * Defines:
 * - EnergyComponent: Attached to entities that consume energy
 *
 * Each tick the distribution system sets energy_received and is_powered
 * based on supply availability and rationing priority.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/energy/EnergyPriorities.h>
#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace energy {

/**
 * @struct EnergyComponent
 * @brief Energy consumer data attached to buildings that require power (12 bytes).
 *
 * Tracks per-entity energy demand, supply received, and rationing priority.
 * The energy distribution system writes energy_received and is_powered each tick.
 *
 * Layout (12 bytes):
 * - energy_required:  4 bytes (uint32_t) - energy units needed per tick
 * - energy_received:  4 bytes (uint32_t) - energy units actually received this tick
 * - is_powered:       1 byte  (bool)     - true if energy_received >= energy_required
 * - priority:         1 byte  (uint8_t)  - rationing priority 1-4
 * - grid_id:          1 byte  (uint8_t)  - future isolated grid support
 * - _padding:         1 byte  (uint8_t)  - alignment padding
 *
 * Total: 12 bytes
 */
struct EnergyComponent {
    uint32_t energy_required = 0;   ///< Energy units needed per tick, from template
    uint32_t energy_received = 0;   ///< Energy units actually received this tick
    bool is_powered = false;        ///< True if energy_received >= energy_required
    uint8_t priority = ENERGY_PRIORITY_DEFAULT; ///< Rationing priority 1-4
    uint8_t grid_id = 0;            ///< Future isolated grid support
    uint8_t _padding = 0;           ///< Alignment padding
};

// Verify EnergyComponent size (12 bytes)
static_assert(sizeof(EnergyComponent) == 12,
    "EnergyComponent must be 12 bytes");

// Verify EnergyComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<EnergyComponent>::value,
    "EnergyComponent must be trivially copyable");

} // namespace energy
} // namespace sims3000
