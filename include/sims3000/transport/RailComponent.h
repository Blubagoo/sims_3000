/**
 * @file RailComponent.h
 * @brief Rail component structure for Epic 7 (Ticket E7-030)
 *
 * Defines:
 * - RailType: Enum for rail types (Surface, Elevated, Subterra)
 * - RailComponent: Per-rail-segment data for the transit network
 *
 * Rail segments form the transit network, connecting terminals and
 * carrying beings across the city. Each segment has a type, capacity,
 * and network membership.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace transport {

/**
 * @enum RailType
 * @brief Types of rail segments in the transit network.
 */
enum class RailType : uint8_t {
    SurfaceRail  = 0,   ///< Ground-level rail
    ElevatedRail = 1,   ///< Above-ground elevated rail
    SubterraRail = 2    ///< Underground rail
};

/**
 * @struct RailComponent
 * @brief Per-rail-segment data for the transit network (12 bytes).
 *
 * Each rail segment carries beings between terminals. Segments belong
 * to a rail network identified by rail_network_id.
 *
 * Layout (12 bytes):
 * - type:                1 byte  (RailType/uint8_t) - rail type
 * - connection_mask:     1 byte  (uint8_t)          - bitmask of connections
 * - capacity:            2 bytes (uint16_t)         - beings per cycle
 * - current_load:        2 bytes (uint16_t)         - current load
 * - rail_network_id:     2 bytes (uint16_t)         - network membership
 * - is_terminal_adjacent:1 byte  (bool)             - adjacent to terminal
 * - is_powered:          1 byte  (bool)             - has power
 * - is_active:           1 byte  (bool)             - currently active
 * - health:              1 byte  (uint8_t)          - condition (0-255)
 *
 * Total: 12 bytes (no padding needed with this layout)
 */
struct RailComponent {
    RailType type             = RailType::SurfaceRail;  ///< Rail type
    uint8_t connection_mask   = 0;      ///< Bitmask of directional connections
    uint16_t capacity         = 500;    ///< Beings per cycle
    uint16_t current_load     = 0;      ///< Current beings on this segment
    uint16_t rail_network_id  = 0;      ///< Network this segment belongs to
    bool is_terminal_adjacent = false;  ///< True if adjacent to a terminal
    bool is_powered           = false;  ///< True if segment has power
    bool is_active            = false;  ///< True if segment is operational
    uint8_t health            = 255;    ///< Segment condition (0=destroyed, 255=perfect)
};

// Verify RailComponent size (12 bytes)
static_assert(sizeof(RailComponent) == 12,
    "RailComponent must be 12 bytes");

// Verify RailComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<RailComponent>::value,
    "RailComponent must be trivially copyable");

} // namespace transport
} // namespace sims3000
