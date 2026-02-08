/**
 * @file PortComponent.h
 * @brief Port facility component structure for Epic 8 (Ticket E8-002)
 *
 * Defines:
 * - PortComponent: Per-port-facility data for external trade connections
 *
 * Each port facility provides external trade capacity and boosts demand
 * for specific zone types within its radius.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 * (aero_port, aqua_port - not airport/seaport)
 */

#pragma once

#include <cstdint>
#include <type_traits>
#include <sims3000/port/PortTypes.h>

namespace sims3000 {
namespace port {

/**
 * @struct PortComponent
 * @brief Per-port-facility data for external trade connections (16 bytes).
 *
 * Tracks port type, capacity, utilization, infrastructure level,
 * operational status, and connectivity for each port facility.
 *
 * Layout (16 bytes):
 * - port_type:              1 byte  (PortType/uint8_t)   - facility classification
 * - capacity:               2 bytes (uint16_t)           - current capacity (max 5000)
 * - max_capacity:           2 bytes (uint16_t)           - maximum capacity
 * - utilization:            1 byte  (uint8_t)            - utilization percentage (0-100)
 * - infrastructure_level:   1 byte  (uint8_t)            - infrastructure tier (0-3)
 * - is_operational:         1 byte  (bool)               - whether port is operational
 * - is_connected_to_edge:   1 byte  (bool)               - whether connected to map edge
 * - demand_bonus_radius:    1 byte  (uint8_t)            - demand boost radius in tiles
 * - connection_flags:       1 byte  (uint8_t)            - pathway/rail/etc bitmask
 * - padding:                4 bytes (uint8_t[4])         - reserved for future use
 *
 * Total: 16 bytes (no padding needed with this layout)
 */
struct PortComponent {
    PortType port_type            = PortType::Aero;  ///< Port facility classification
    uint16_t capacity             = 0;               ///< Current capacity (max 5000)
    uint16_t max_capacity         = 0;               ///< Maximum capacity
    uint8_t utilization           = 0;               ///< Utilization percentage (0-100%)
    uint8_t infrastructure_level  = 0;               ///< Infrastructure tier (0-3)
    bool is_operational           = false;           ///< Whether port is currently operational
    bool is_connected_to_edge     = false;           ///< Whether connected to a map edge
    uint8_t demand_bonus_radius   = 0;               ///< Demand boost radius in tiles
    uint8_t connection_flags      = 0;               ///< Bitmask: Pathway(1), Rail(2), Energy(4), Fluid(8)
    uint8_t padding[4]            = {0, 0, 0, 0};   ///< Reserved for future use
};

// Verify PortComponent size (16 bytes)
static_assert(sizeof(PortComponent) == 16,
    "PortComponent must be 16 bytes");

// Verify PortComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<PortComponent>::value,
    "PortComponent must be trivially copyable");

} // namespace port
} // namespace sims3000
