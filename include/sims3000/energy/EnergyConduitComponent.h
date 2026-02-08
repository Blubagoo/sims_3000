/**
 * @file EnergyConduitComponent.h
 * @brief Energy conduit component structure for Epic 5
 *
 * Defines:
 * - EnergyConduitComponent: Per-conduit data for energy distribution network
 *
 * Conduits form the energy distribution network, connecting nexus facilities
 * to buildings. Coverage radius determines how far energy reaches from conduits.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace energy {

/**
 * @struct EnergyConduitComponent
 * @brief Per-conduit data for energy distribution (4 bytes).
 *
 * Conduits carry energy from nexus facilities to buildings.
 * They form a network that is traversed via BFS to determine connectivity.
 *
 * Layout (4 bytes):
 * - coverage_radius: 1 byte (uint8_t) - tiles of coverage this conduit adds
 * - is_connected: 1 byte (bool) - true if connected to energy matrix via BFS
 * - is_active: 1 byte (bool) - true if carrying energy (for rendering glow)
 * - conduit_level: 1 byte (uint8_t) - 1=basic, 2=upgraded (future)
 *
 * Total: 4 bytes
 */
struct EnergyConduitComponent {
    uint8_t coverage_radius = 3;   ///< Tiles of coverage this conduit adds
    bool is_connected = false;     ///< True if connected to energy matrix via BFS
    bool is_active = false;        ///< True if carrying energy (for rendering glow)
    uint8_t conduit_level = 1;     ///< 1=basic, 2=upgraded (future)
};

// Verify EnergyConduitComponent size
static_assert(sizeof(EnergyConduitComponent) == 4,
    "EnergyConduitComponent must be 4 bytes");

// Verify EnergyConduitComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<EnergyConduitComponent>::value,
    "EnergyConduitComponent must be trivially copyable");

} // namespace energy
} // namespace sims3000
