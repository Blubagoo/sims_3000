/**
 * @file EnergyProducerComponent.h
 * @brief Energy producer/nexus component for Epic 5 (Ticket 5-003)
 *
 * Defines:
 * - EnergyProducerComponent: Attached to energy nexus entities
 *
 * Each tick the energy system recalculates current_output from
 * base_output, efficiency, and age_factor. Offline nexuses produce 0.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/energy/EnergyEnums.h>
#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace energy {

/**
 * @struct EnergyProducerComponent
 * @brief Energy nexus (producer) data (24 bytes).
 *
 * Tracks per-nexus output capacity, efficiency degradation, and
 * contamination generation. The energy system updates current_output
 * each tick based on base_output * efficiency * age_factor.
 *
 * Layout (24 bytes):
 * - base_output:           4 bytes (uint32_t) - maximum output at 100% efficiency
 * - current_output:        4 bytes (uint32_t) - actual output this tick
 * - efficiency:            4 bytes (float)    - current efficiency multiplier 0.0-1.0
 * - age_factor:            4 bytes (float)    - aging degradation, starts at 1.0
 * - ticks_since_built:     2 bytes (uint16_t) - age in ticks, capped at 65535
 * - nexus_type:            1 byte  (uint8_t)  - NexusType enum value
 * - is_online:             1 byte  (bool)     - true if operational
 * - contamination_output:  4 bytes (uint32_t) - contamination units per tick
 *
 * Total: 24 bytes
 */
struct EnergyProducerComponent {
    uint32_t base_output = 0;           ///< Maximum output at 100% efficiency
    uint32_t current_output = 0;        ///< Actual output this tick
    float efficiency = 1.0f;            ///< Current efficiency multiplier 0.0-1.0
    float age_factor = 1.0f;            ///< Aging degradation, starts at 1.0
    uint16_t ticks_since_built = 0;     ///< Age in ticks, capped at 65535
    uint8_t nexus_type = 0;             ///< NexusType enum value
    bool is_online = true;              ///< True if operational
    uint32_t contamination_output = 0;  ///< Contamination units per tick
};

// Verify EnergyProducerComponent size (24 bytes)
static_assert(sizeof(EnergyProducerComponent) == 24,
    "EnergyProducerComponent must be 24 bytes");

// Verify EnergyProducerComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<EnergyProducerComponent>::value,
    "EnergyProducerComponent must be trivially copyable");

} // namespace energy
} // namespace sims3000
