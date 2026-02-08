/**
 * @file FluidPlacementValidation.h
 * @brief Placement validation for fluid extractors and reservoirs (Tickets 6-027, 6-028)
 *
 * Provides validation functions and result types for placing fluid infrastructure:
 * - ExtractorPlacementResult: includes water distance, expected efficiency, operability
 * - ReservoirPlacementResult: simple can_place/reason
 * - validate_extractor_placement(): checks bounds, water proximity, terrain, structure
 * - validate_reservoir_placement(): checks bounds, terrain, structure (no water req)
 * - calculate_water_factor(): distance-to-efficiency curve for extractors
 *
 * Follows the same validation pattern as EnergySystem::validate_nexus_placement().
 *
 * @see FluidExtractorConfig.h for extractor constants
 * @see FluidReservoirConfig.h for reservoir constants
 * @see /docs/epics/epic-6/tickets.md (tickets 6-027, 6-028)
 */

#pragma once

#include <sims3000/fluid/FluidExtractorConfig.h>
#include <entt/entity/fwd.hpp>
#include <cstdint>

// Forward declarations
namespace sims3000 {
namespace terrain {
    class ITerrainQueryable;
} // namespace terrain
} // namespace sims3000

namespace sims3000 {
namespace fluid {

// =============================================================================
// Placement Result Types
// =============================================================================

/**
 * @struct ExtractorPlacementResult
 * @brief Result of extractor placement validation.
 *
 * Contains success flag, failure reason, and extractor-specific data:
 * - water_distance: actual Manhattan distance to nearest water tile
 * - expected_efficiency: water factor (0.0-1.0) at this distance
 * - will_be_operational: true if distance <= MAX_OPERATIONAL_DISTANCE
 *
 * @see Ticket 6-027: Extractor Placement Validation
 */
struct ExtractorPlacementResult {
    bool can_place = false;                ///< True if placement is valid
    const char* reason = "";               ///< Human-readable failure reason (empty on success)
    uint8_t water_distance = 255;          ///< Manhattan distance to nearest water tile
    float expected_efficiency = 0.0f;      ///< Water factor at this distance (0.0-1.0)
    bool will_be_operational = false;      ///< True if distance <= MAX_OPERATIONAL_DISTANCE
};

/**
 * @struct ReservoirPlacementResult
 * @brief Result of reservoir placement validation.
 *
 * Simpler than extractor: no water proximity requirement.
 *
 * @see Ticket 6-028: Reservoir Placement Validation
 */
struct ReservoirPlacementResult {
    bool can_place = false;                ///< True if placement is valid
    const char* reason = "";               ///< Human-readable failure reason (empty on success)
};

// =============================================================================
// Water Factor Calculation
// =============================================================================

/**
 * @brief Calculate water proximity efficiency factor for an extractor.
 *
 * Returns a multiplier (0.0-1.0) based on Manhattan distance to water:
 * - 0 tiles:   1.0 (100%)
 * - 1-2 tiles: 0.9 (90%)
 * - 3-4 tiles: 0.7 (70%)
 * - 5-6 tiles: 0.5 (50%)
 * - 7-8 tiles: 0.3 (30%)
 * - 9+ tiles:  0.0 (cannot operate)
 *
 * @param distance Manhattan distance to nearest water tile.
 * @return Efficiency multiplier (0.0-1.0).
 */
float calculate_water_factor(uint8_t distance);

// =============================================================================
// Extractor Placement Validation
// =============================================================================

/**
 * @brief Validate extractor placement at a grid position.
 *
 * Checks in order:
 * 1. Bounds check: x < map_width, y < map_height
 * 2. Water proximity: get_water_distance(x, y) <= MAX_PLACEMENT_DISTANCE (8)
 * 3. Calculate expected_efficiency from distance using calculate_water_factor()
 * 4. will_be_operational = (distance <= MAX_OPERATIONAL_DISTANCE)
 * 5. Terrain buildable check (if terrain != nullptr)
 * 6. No existing structure check (if registry != nullptr, stub: always passes)
 *
 * @param x X coordinate (column).
 * @param y Y coordinate (row).
 * @param owner Owning player ID (0-3).
 * @param map_width Map width in tiles.
 * @param map_height Map height in tiles.
 * @param terrain Pointer to terrain query interface (may be nullptr).
 * @param registry Pointer to ECS registry (may be nullptr).
 * @return ExtractorPlacementResult with placement data.
 */
ExtractorPlacementResult validate_extractor_placement(
    uint32_t x, uint32_t y, uint8_t owner,
    uint32_t map_width, uint32_t map_height,
    const terrain::ITerrainQueryable* terrain,
    entt::registry* registry);

// =============================================================================
// Reservoir Placement Validation
// =============================================================================

/**
 * @brief Validate reservoir placement at a grid position.
 *
 * Checks in order:
 * 1. Bounds check: x < map_width, y < map_height
 * 2. Terrain buildable check (if terrain != nullptr)
 * 3. No existing structure check (if registry != nullptr, stub: always passes)
 * 4. NO water proximity requirement (reservoirs can go anywhere buildable)
 *
 * @param x X coordinate (column).
 * @param y Y coordinate (row).
 * @param owner Owning player ID (0-3).
 * @param map_width Map width in tiles.
 * @param map_height Map height in tiles.
 * @param terrain Pointer to terrain query interface (may be nullptr).
 * @param registry Pointer to ECS registry (may be nullptr).
 * @return ReservoirPlacementResult with placement data.
 */
ReservoirPlacementResult validate_reservoir_placement(
    uint32_t x, uint32_t y, uint8_t owner,
    uint32_t map_width, uint32_t map_height,
    const terrain::ITerrainQueryable* terrain,
    entt::registry* registry);

} // namespace fluid
} // namespace sims3000
