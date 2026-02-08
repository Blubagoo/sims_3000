/**
 * @file PortZoneComponent.h
 * @brief Port zone component structure for Epic 8 (Ticket E8-003)
 *
 * Defines:
 * - PortZoneComponent: Per-port-zone data tracking zone development,
 *   runway/dock requirements, and zone area
 *
 * Port zones have type-specific requirements:
 * - Aero ports require runways (has_runway, runway_length, runway_area)
 * - Aqua ports require docks (has_dock, dock_count)
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 * (aero_port, aqua_port - not airport/seaport)
 */

#pragma once

#include <cstdint>
#include <type_traits>
#include <sims3000/port/PortTypes.h>
#include <sims3000/terrain/TerrainEvents.h>

namespace sims3000 {
namespace port {

/**
 * @struct PortZoneComponent
 * @brief Per-port-zone data for zone development and requirements (16 bytes).
 *
 * Tracks port zone type, development level, type-specific requirements
 * (runway for aero, dock for aqua), and zone extent.
 *
 * Layout (16 bytes):
 * - port_type:      1 byte  (PortType/uint8_t)            - port zone classification
 * - zone_level:     1 byte  (uint8_t)                     - development level (0-4)
 * - has_runway:     1 byte  (bool)                        - aero requirement met
 * - has_dock:       1 byte  (bool)                        - aqua requirement met
 * - runway_length:  1 byte  (uint8_t)                     - runway length in tiles (aero)
 * - dock_count:     1 byte  (uint8_t)                     - water-adjacent docks (aqua)
 * - zone_tiles:     2 bytes (uint16_t)                    - total tiles in zone
 * - runway_area:    8 bytes (terrain::GridRect)            - runway bounding rect
 *
 * Total: 16 bytes (no padding needed with this layout)
 */
struct PortZoneComponent {
    PortType port_type            = PortType::Aero;  ///< Port zone classification
    uint8_t zone_level            = 0;               ///< Development level (0-4)
    bool has_runway               = false;           ///< Whether aero runway requirement is met
    bool has_dock                 = false;           ///< Whether aqua dock requirement is met
    uint8_t runway_length         = 0;               ///< Runway length in tiles (aero ports)
    uint8_t dock_count            = 0;               ///< Number of water-adjacent docks (aqua ports)
    uint16_t zone_tiles           = 0;               ///< Total tiles in zone
    terrain::GridRect runway_area;                   ///< Runway bounding rectangle (x,y,w,h as int16/uint16)
};

// Verify PortZoneComponent size (16 bytes)
static_assert(sizeof(PortZoneComponent) == 16,
    "PortZoneComponent must be 16 bytes");

// Verify PortZoneComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<PortZoneComponent>::value,
    "PortZoneComponent must be trivially copyable");

} // namespace port
} // namespace sims3000
