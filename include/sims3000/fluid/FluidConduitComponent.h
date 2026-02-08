/**
 * @file FluidConduitComponent.h
 * @brief Fluid conduit component structure for Epic 6 (Ticket 6-005)
 *
 * Defines:
 * - FluidConduitComponent: Per-conduit data for fluid distribution network
 *
 * Conduits form the fluid distribution network, connecting extractor/reservoir
 * facilities to buildings. Coverage radius determines how far fluid reaches
 * from conduits.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace fluid {

/**
 * @struct FluidConduitComponent
 * @brief Per-conduit data for fluid distribution (4 bytes).
 *
 * Conduits carry fluid from extractor/reservoir facilities to buildings.
 * They form a network that is traversed via BFS to determine connectivity.
 *
 * Identical structure to EnergyConduitComponent.
 *
 * Layout (4 bytes):
 * - coverage_radius: 1 byte (uint8_t) - tiles of coverage this conduit adds
 * - is_connected: 1 byte (bool) - true if connected to fluid network via BFS
 * - is_active: 1 byte (bool) - true if carrying fluid (for rendering)
 * - conduit_level: 1 byte (uint8_t) - 1=basic, 2=upgraded (future)
 *
 * Total: 4 bytes
 */
struct FluidConduitComponent {
    uint8_t coverage_radius = 3;   ///< Tiles of coverage this conduit adds
    bool is_connected = false;     ///< True if connected to fluid network via BFS
    bool is_active = false;        ///< True if carrying fluid (for rendering)
    uint8_t conduit_level = 1;     ///< 1=basic, 2=upgraded (future)
};

// Verify FluidConduitComponent size
static_assert(sizeof(FluidConduitComponent) == 4,
    "FluidConduitComponent must be 4 bytes");

// Verify FluidConduitComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<FluidConduitComponent>::value,
    "FluidConduitComponent must be trivially copyable");

} // namespace fluid
} // namespace sims3000
