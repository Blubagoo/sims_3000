/**
 * @file FluidProducerComponent.h
 * @brief Fluid producer component for Epic 6 (Ticket 6-003)
 *
 * Defines:
 * - FluidProducerComponent: Attached to fluid extractor/reservoir entities
 *
 * Each tick the fluid system recalculates current_output from
 * base_output, water proximity factor, and powered state.
 * Non-operational producers (unpowered or too far from water) produce 0.
 *
 * Simpler than EnergyProducerComponent: no aging, no contamination.
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
 * @struct FluidProducerComponent
 * @brief Fluid producer (extractor/reservoir) data (12 bytes).
 *
 * Tracks per-producer output capacity and water proximity.
 * The fluid system updates current_output each tick:
 *   current_output = base_output * water_factor * (powered ? 1 : 0)
 *
 * No aging or contamination fields (simpler than energy nexuses).
 *
 * Layout (12 bytes, natural alignment):
 * - base_output:             4 bytes (uint32_t) - maximum output at optimal conditions
 * - current_output:          4 bytes (uint32_t) - actual output this tick
 * - max_water_distance:      1 byte  (uint8_t)  - max tiles from water (typically 5)
 * - current_water_distance:  1 byte  (uint8_t)  - actual distance to nearest water
 * - is_operational:          1 byte  (bool)     - true if powered AND within water proximity
 * - producer_type:           1 byte  (uint8_t)  - FluidProducerType enum value
 *
 * Total: 12 bytes
 */
struct FluidProducerComponent {
    uint32_t base_output = 0;              ///< Maximum output at optimal conditions
    uint32_t current_output = 0;           ///< Actual output this tick
    uint8_t max_water_distance = 5;        ///< Maximum tiles from water for operation
    uint8_t current_water_distance = 0;    ///< Actual distance to nearest water source
    bool is_operational = false;           ///< True if powered AND within water proximity
    uint8_t producer_type = 0;             ///< FluidProducerType enum value
};

// Verify FluidProducerComponent size (12 bytes)
static_assert(sizeof(FluidProducerComponent) == 12,
    "FluidProducerComponent must be 12 bytes");

// Verify FluidProducerComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<FluidProducerComponent>::value,
    "FluidProducerComponent must be trivially copyable");

} // namespace fluid
} // namespace sims3000
