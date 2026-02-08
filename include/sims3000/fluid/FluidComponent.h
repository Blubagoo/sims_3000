/**
 * @file FluidComponent.h
 * @brief Fluid consumer component for Epic 6 (Ticket 6-002)
 *
 * Defines:
 * - FluidComponent: Attached to entities that consume fluid
 *
 * Each tick the distribution system sets fluid_received and has_fluid
 * based on supply availability. Unlike energy, fluid uses all-or-nothing
 * distribution (CCR-002) - no priority/rationing field.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace fluid {

/**
 * @struct FluidComponent
 * @brief Fluid consumer data attached to buildings that require fluid (12 bytes).
 *
 * Tracks per-entity fluid demand and supply received.
 * The fluid distribution system writes fluid_received and has_fluid each tick.
 *
 * Unlike EnergyComponent, there is NO priority field because fluid uses
 * all-or-nothing distribution (CCR-002).
 *
 * Layout (12 bytes):
 * - fluid_required:  4 bytes (uint32_t) - fluid units needed per tick
 * - fluid_received:  4 bytes (uint32_t) - fluid units actually received this tick
 * - has_fluid:       1 byte  (bool)     - true if fluid_received >= fluid_required
 * - _padding:        3 bytes (uint8_t[3]) - alignment padding to 12 bytes
 *
 * Total: 12 bytes
 */
struct FluidComponent {
    uint32_t fluid_required = 0;   ///< Fluid units needed per tick, from template
    uint32_t fluid_received = 0;   ///< Fluid units actually received this tick
    bool has_fluid = false;        ///< True if fluid_received >= fluid_required
    uint8_t _padding[3] = {0, 0, 0};  ///< Alignment padding to 12 bytes
};

// Verify FluidComponent size (12 bytes)
static_assert(sizeof(FluidComponent) == 12,
    "FluidComponent must be 12 bytes");

// Verify FluidComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<FluidComponent>::value,
    "FluidComponent must be trivially copyable");

} // namespace fluid
} // namespace sims3000
