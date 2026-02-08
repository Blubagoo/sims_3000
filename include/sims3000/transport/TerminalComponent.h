/**
 * @file TerminalComponent.h
 * @brief Terminal component structure for Epic 7 (Ticket E7-031)
 *
 * Defines:
 * - TerminalType: Enum for terminal types (Surface, Subterra, Intermodal)
 * - TerminalComponent: Per-terminal data for the transit network
 *
 * Terminals serve as boarding/alighting points for beings using the
 * rail transit system. Each terminal has a type, capacity, and coverage
 * radius that determines how far beings will walk to reach it.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace transport {

/**
 * @enum TerminalType
 * @brief Types of terminals in the transit network.
 */
enum class TerminalType : uint8_t {
    SurfaceStation  = 0,   ///< Ground-level station
    SubterraStation = 1,   ///< Underground station
    IntermodalHub   = 2    ///< Multi-mode transfer hub
};

/**
 * @struct TerminalComponent
 * @brief Per-terminal data for the transit network (10 bytes).
 *
 * Terminals allow beings to board and exit the rail network.
 * Coverage radius determines walk distance for nearby beings.
 *
 * Layout (10 bytes):
 * - type:            1 byte  (TerminalType/uint8_t) - terminal type
 * - coverage_radius: 1 byte  (uint8_t)             - walk distance in tiles
 * - capacity:        2 bytes (uint16_t)             - max beings at station
 * - current_usage:   2 bytes (uint16_t)             - current beings at station
 * - is_powered:      1 byte  (bool)                 - has power
 * - is_active:       1 byte  (bool)                 - currently active
 * - padding:         2 bytes (uint8_t[2])           - alignment padding
 *
 * Total: 10 bytes
 */
struct TerminalComponent {
    TerminalType type       = TerminalType::SurfaceStation; ///< Terminal type
    uint8_t coverage_radius = 8;       ///< Walk distance in tiles
    uint16_t capacity       = 200;     ///< Max beings at this terminal
    uint16_t current_usage  = 0;       ///< Current beings at this terminal
    bool is_powered         = false;   ///< True if terminal has power
    bool is_active          = false;   ///< True if terminal is operational
    uint8_t padding[2]      = {};      ///< Alignment padding
};

// Verify TerminalComponent size (10 bytes)
static_assert(sizeof(TerminalComponent) == 10,
    "TerminalComponent must be 10 bytes");

// Verify TerminalComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<TerminalComponent>::value,
    "TerminalComponent must be trivially copyable");

} // namespace transport
} // namespace sims3000
