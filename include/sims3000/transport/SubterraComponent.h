/**
 * @file SubterraComponent.h
 * @brief Subterra (underground) component structure for Epic 7 (Ticket E7-043)
 *
 * Defines:
 * - SubterraComponent: Per-tile underground infrastructure data
 *
 * SubterraComponent marks underground tiles that have been excavated
 * for subterra rail placement. Tracks depth level, ventilation, and
 * surface access for underground transit segments.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace transport {

/**
 * @struct SubterraComponent
 * @brief Per-tile underground infrastructure data (8 bytes).
 *
 * Attached to tiles that contain underground transit infrastructure.
 * Depth level is fixed at 1 for MVP (single underground layer).
 *
 * Layout (8 bytes):
 * - depth_level:        1 byte  (uint8_t) - underground layer (1 for MVP)
 * - is_excavated:       1 byte  (bool)    - true if tile is dug out
 * - ventilation_radius: 1 byte  (uint8_t) - ventilation reach in tiles
 * - has_surface_access: 1 byte  (bool)    - true if connected to surface
 * - padding:            4 bytes (uint8_t[4]) - alignment padding
 *
 * Total: 8 bytes
 */
struct SubterraComponent {
    uint8_t depth_level        = 1;     ///< Underground layer (1 for MVP)
    bool is_excavated          = true;  ///< True if tile is excavated
    uint8_t ventilation_radius = 2;     ///< Ventilation reach in tiles
    bool has_surface_access    = false;  ///< True if connected to surface
    uint8_t padding[4]         = {};    ///< Alignment padding
};

// Verify SubterraComponent size (8 bytes)
static_assert(sizeof(SubterraComponent) == 8,
    "SubterraComponent must be 8 bytes");

// Verify SubterraComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<SubterraComponent>::value,
    "SubterraComponent must be trivially copyable");

} // namespace transport
} // namespace sims3000
