/**
 * @file PortRenderData.h
 * @brief Port zone rendering support data structures (Epic 8, Ticket E8-030)
 *
 * Provides data to RenderingSystem for port visualization:
 * - Port position and dimensions
 * - Port type and development level (0-4)
 * - Operational status
 * - Boundary flags for edge rendering
 * - Type-specific infrastructure (runway for aero, docks for aqua)
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 * (aero_port, aqua_port - not airport/seaport)
 */

#pragma once

#include <sims3000/port/PortTypes.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace port {

/// Boundary flag bitmask constants
constexpr uint8_t BOUNDARY_NORTH = 1;   ///< Northern boundary present
constexpr uint8_t BOUNDARY_SOUTH = 2;   ///< Southern boundary present
constexpr uint8_t BOUNDARY_EAST  = 4;   ///< Eastern boundary present
constexpr uint8_t BOUNDARY_WEST  = 8;   ///< Western boundary present

/**
 * @struct PortRenderData
 * @brief Visual state data for rendering a single port zone.
 *
 * Contains all information needed by RenderingSystem to draw
 * a port zone, including position, type, development level,
 * operational status, and type-specific infrastructure details.
 */
struct PortRenderData {
    int32_t x = 0;                 ///< Port zone X position (tile coordinates)
    int32_t y = 0;                 ///< Port zone Y position (tile coordinates)
    uint16_t width = 0;            ///< Port zone width in tiles
    uint16_t height = 0;           ///< Port zone height in tiles
    PortType port_type = PortType::Aero; ///< Port facility type
    uint8_t zone_level = 0;        ///< Development level (0-4)
    bool is_operational = false;   ///< Whether port is currently operational
    uint8_t boundary_flags = 0;    ///< Boundary edges: N(1)/S(2)/E(4)/W(8)

    // Type-specific infrastructure rendering data
    int16_t runway_x = 0;          ///< Runway X offset within zone (aero ports)
    int16_t runway_y = 0;          ///< Runway Y offset within zone (aero ports)
    uint16_t runway_w = 0;         ///< Runway width in tiles (aero ports)
    uint16_t runway_h = 0;         ///< Runway height in tiles (aero ports)
    uint8_t dock_count = 0;        ///< Number of dock structures (aqua ports)
};

} // namespace port
} // namespace sims3000
