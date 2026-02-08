/**
 * @file FluidReservoirComponent.h
 * @brief Fluid reservoir ECS component for Epic 6 (Ticket 6-004)
 *
 * Defines:
 * - FluidReservoirComponent: Per-entity fluid storage and flow rates
 *
 * Each reservoir entity (e.g. water tower, pumping station) carries one
 * FluidReservoirComponent. The fluid distribution system reads capacity,
 * fill_rate, and drain_rate each tick to move fluid through the network.
 *
 * Asymmetric rates: drain_rate > fill_rate to model real-world behavior
 * where consumption outpaces refill (pressure-driven distribution).
 *
 * Default values per CCR-005:
 * - capacity:   1000 units (MVP reservoir size)
 * - fill_rate:  50 units/tick
 * - drain_rate: 100 units/tick
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace fluid {

/**
 * @struct FluidReservoirComponent
 * @brief Per-entity fluid storage and flow tracking (16 bytes).
 *
 * Compact ECS component for fluid reservoir entities. Stores
 * maximum capacity, current fill level, and asymmetric flow rates.
 *
 * Layout (16 bytes, natural alignment):
 * - capacity:       4 bytes (uint32_t) - maximum storage in fluid units
 * - current_level:  4 bytes (uint32_t) - current stored amount
 * - fill_rate:      2 bytes (uint16_t) - units/tick that can fill
 * - drain_rate:     2 bytes (uint16_t) - units/tick that can drain
 * - is_active:      1 byte  (bool)     - connected to network
 * - reservoir_type: 1 byte  (uint8_t)  - reserved for future use
 * - _padding:       2 bytes (uint8_t[2]) - alignment padding
 */
struct FluidReservoirComponent {
    uint32_t capacity = 1000;       ///< Maximum storage (1000 units MVP per CCR-005)
    uint32_t current_level = 0;     ///< Current stored amount
    uint16_t fill_rate = 50;        ///< Units/tick that can fill (50 per CCR-005)
    uint16_t drain_rate = 100;      ///< Units/tick that can drain (100 per CCR-005)
    bool is_active = false;         ///< Connected to fluid network
    uint8_t reservoir_type = 0;     ///< Reserved for future reservoir types
    uint8_t _padding[2] = {0, 0};   ///< Alignment padding
};

// Verify FluidReservoirComponent is exactly 16 bytes
static_assert(sizeof(FluidReservoirComponent) == 16,
    "FluidReservoirComponent must be exactly 16 bytes");

// Verify FluidReservoirComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<FluidReservoirComponent>::value,
    "FluidReservoirComponent must be trivially copyable");

} // namespace fluid
} // namespace sims3000
