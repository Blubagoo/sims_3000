/**
 * @file RoadComponent.h
 * @brief Road/pathway component structure for Epic 7 (Ticket E7-002)
 *
 * Defines:
 * - RoadComponent: Per-pathway-segment data for the surface transport network
 *
 * Each pathway segment carries traffic between junctions and connects to
 * adjacent segments via the connection_mask bitmask.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 * (pathway - not road)
 */

#pragma once

#include <cstdint>
#include <type_traits>
#include <sims3000/transport/TransportEnums.h>

namespace sims3000 {
namespace transport {

/**
 * @struct RoadComponent
 * @brief Per-pathway-segment data for the surface transport network (16 bytes).
 *
 * Tracks pathway type, direction, capacity, health, connectivity, and
 * maintenance state for each surface pathway segment on the grid.
 *
 * Layout (16 bytes):
 * - type:                1 byte  (PathwayType/uint8_t)      - pathway classification
 * - direction:           1 byte  (PathwayDirection/uint8_t)  - flow direction mode
 * - base_capacity:       2 bytes (uint16_t)                  - max vehicles per tick
 * - current_capacity:    2 bytes (uint16_t)                  - effective capacity
 * - health:              1 byte  (uint8_t)                   - condition (0-255)
 * - decay_rate:          1 byte  (uint8_t)                   - health loss per tick
 * - connection_mask:     1 byte  (uint8_t)                   - N(1),S(2),E(4),W(8)
 * - is_junction:         1 byte  (bool)                      - true if intersection
 * - network_id:          2 bytes (uint16_t)                  - network membership
 * - last_maintained_tick:4 bytes (uint32_t)                  - tick of last maintenance
 *
 * Total: 16 bytes (no padding needed with this layout)
 */
struct RoadComponent {
    PathwayType type               = PathwayType::BasicPathway;    ///< Pathway classification
    PathwayDirection direction     = PathwayDirection::Bidirectional; ///< Flow direction mode
    uint16_t base_capacity         = 100;    ///< Maximum vehicles per tick
    uint16_t current_capacity      = 100;    ///< Effective capacity (may be reduced)
    uint8_t health                 = 255;    ///< Condition (0=destroyed, 255=pristine)
    uint8_t decay_rate             = 1;      ///< Health loss per maintenance tick
    uint8_t connection_mask        = 0;      ///< Bitmask: N(1), S(2), E(4), W(8)
    bool is_junction               = false;  ///< True if this segment is an intersection
    uint16_t network_id            = 0;      ///< Network this segment belongs to
    uint32_t last_maintained_tick  = 0;      ///< Simulation tick of last maintenance
};

// Verify RoadComponent size (16 bytes)
static_assert(sizeof(RoadComponent) == 16,
    "RoadComponent must be 16 bytes");

// Verify RoadComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<RoadComponent>::value,
    "RoadComponent must be trivially copyable");

} // namespace transport
} // namespace sims3000
