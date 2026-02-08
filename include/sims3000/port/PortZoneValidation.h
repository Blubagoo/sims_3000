/**
 * @file PortZoneValidation.h
 * @brief Port zone validation functions for Epic 8 (Tickets E8-008, E8-009)
 *
 * Provides validation for aero port and aqua port zone placement:
 * - validate_aero_port_zone: Checks minimum size, runway detection, terrain
 *   flatness, and pathway accessibility
 * - validate_aqua_port_zone: Checks minimum size, water adjacency, dock
 *   tile count, and pathway accessibility
 *
 * Both functions use ITerrainQueryable for terrain queries and
 * ITransportProvider for pathway accessibility checks.
 */

#pragma once

#include <sims3000/terrain/TerrainEvents.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>

namespace sims3000 {
namespace port {

/// Minimum zone area for aero port (6x6 = 36 tiles)
constexpr uint32_t AERO_PORT_MIN_TILES = 36;

/// Minimum zone area for aqua port (4x8 = 32 tiles)
constexpr uint32_t AQUA_PORT_MIN_TILES = 32;

/// Minimum runway length in tiles for aero port
constexpr uint16_t AERO_RUNWAY_MIN_LENGTH = 6;

/// Minimum runway width in tiles for aero port
constexpr uint16_t AERO_RUNWAY_MIN_WIDTH = 2;

/// Minimum water-adjacent perimeter tiles for aqua port dock placement
constexpr uint32_t AQUA_PORT_MIN_DOCK_TILES = 4;

/// Maximum distance from zone perimeter to pathway for accessibility
constexpr uint32_t PORT_PATHWAY_MAX_DISTANCE = 3;

/**
 * @brief Validate an aero port zone placement.
 *
 * Checks:
 * 1. Minimum zone size (36 tiles, 6x6)
 * 2. Valid runway area within zone (6 tiles long, 2 tiles wide minimum)
 * 3. Terrain flatness for runway (single elevation level)
 * 4. Pathway accessibility (within 3 tiles of zone perimeter)
 *
 * @param zone The rectangular zone area to validate.
 * @param terrain Terrain query interface for elevation/type checks.
 * @param transport Transport query interface for pathway accessibility.
 * @return true if the zone is valid for aero port placement.
 */
bool validate_aero_port_zone(const terrain::GridRect& zone,
                              const terrain::ITerrainQueryable& terrain,
                              const building::ITransportProvider& transport);

/**
 * @brief Validate an aqua port zone placement.
 *
 * Checks:
 * 1. Minimum zone size (32 tiles, 4x8)
 * 2. Water adjacency on zone perimeter (DeepVoid, FlowChannel, or StillBasin)
 * 3. Minimum 4 water-adjacent perimeter tiles for dock placement
 * 4. Pathway accessibility (within 3 tiles of zone perimeter)
 *
 * @param zone The rectangular zone area to validate.
 * @param terrain Terrain query interface for terrain type/water checks.
 * @param transport Transport query interface for pathway accessibility.
 * @return true if the zone is valid for aqua port placement.
 */
bool validate_aqua_port_zone(const terrain::GridRect& zone,
                              const terrain::ITerrainQueryable& terrain,
                              const building::ITransportProvider& transport);

} // namespace port
} // namespace sims3000
