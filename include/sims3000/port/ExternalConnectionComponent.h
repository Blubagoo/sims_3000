/**
 * @file ExternalConnectionComponent.h
 * @brief External connection component structure for Epic 8 (Ticket E8-004)
 *
 * Defines:
 * - ExternalConnectionComponent: Per-connection data for map-edge external links
 *
 * Each external connection represents a pathway, rail, energy, or fluid
 * link at the edge of the map that enables trade and migration flows
 * with neighboring cities or NPC regions.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include <type_traits>
#include <sims3000/port/PortTypes.h>
#include <sims3000/core/types.h>

namespace sims3000 {
namespace port {

/**
 * @struct ExternalConnectionComponent
 * @brief Per-connection data for map-edge external links (16 bytes).
 *
 * Tracks the connection type, map edge location, activation state,
 * and trade/migration capacity for each external connection point.
 *
 * Layout (16 bytes):
 * - connection_type:    1 byte  (ConnectionType/uint8_t)  - infrastructure type
 * - edge_side:          1 byte  (MapEdge/uint8_t)         - which map edge
 * - edge_position:      2 bytes (uint16_t)                - position along edge
 * - is_active:          1 byte  (bool)                    - whether connection is active
 * - padding1:           1 byte  (uint8_t)                 - alignment padding
 * - trade_capacity:     2 bytes (uint16_t)                - max trade flow per tick
 * - migration_capacity: 2 bytes (uint16_t)                - max migration flow per tick
 * - padding2:           2 bytes (uint16_t)                - alignment padding
 * - position:           4 bytes (GridPosition)            - grid position (int16_t x, y)
 *
 * Total: 16 bytes (no padding needed with this layout)
 */
struct ExternalConnectionComponent {
    ConnectionType connection_type = ConnectionType::Pathway;  ///< Infrastructure type
    MapEdge edge_side              = MapEdge::North;           ///< Which map edge
    uint16_t edge_position         = 0;     ///< Position along the edge (tile index)
    bool is_active                 = false; ///< Whether the connection is currently active
    uint8_t padding1               = 0;     ///< Alignment padding
    uint16_t trade_capacity        = 0;     ///< Maximum trade flow per simulation tick
    uint16_t migration_capacity    = 0;     ///< Maximum migration flow per simulation tick
    uint16_t padding2              = 0;     ///< Alignment padding
    GridPosition position;                  ///< Grid position of this connection (4 bytes)
};

// Verify ExternalConnectionComponent size (16 bytes)
static_assert(sizeof(ExternalConnectionComponent) == 16,
    "ExternalConnectionComponent must be 16 bytes");

// Verify ExternalConnectionComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<ExternalConnectionComponent>::value,
    "ExternalConnectionComponent must be trivially copyable");

} // namespace port
} // namespace sims3000
