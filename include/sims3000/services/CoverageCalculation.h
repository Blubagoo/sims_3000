/**
 * @file CoverageCalculation.h
 * @brief Radius-based coverage calculation for service buildings (Epic 9, Ticket E9-020)
 *
 * Provides functions to calculate service coverage from radius-based buildings
 * (Enforcer, HazardResponse) onto a ServiceCoverageGrid.
 *
 * Algorithm:
 * 1. Clear the grid
 * 2. For each active building, iterate tiles within bounding box [-radius, +radius]
 * 3. Calculate manhattan distance
 * 4. Apply linear falloff: strength = effectiveness * (1.0 - dist/radius)
 * 5. Convert to uint8_t (0-255)
 * 6. Use max-value overlap: grid[x,y] = max(grid[x,y], calculated_value)
 */

#pragma once

#include <sims3000/services/ServiceTypes.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace services {

// Forward declaration
class ServiceCoverageGrid;

/**
 * @struct ServiceBuildingData
 * @brief Lightweight data struct representing a service building for coverage calculation.
 *
 * Contains all data needed to calculate a building's coverage contribution
 * without requiring ECS component access.
 */
struct ServiceBuildingData {
    int32_t x;                  ///< Building X position (tile coordinate)
    int32_t y;                  ///< Building Y position (tile coordinate)
    ServiceType type;           ///< Service type (Enforcer, HazardResponse, etc.)
    uint8_t tier;               ///< Service tier (1=Post, 2=Station, 3=Nexus)
    uint8_t effectiveness;      ///< Current effectiveness (0-255)
    bool is_active;             ///< Whether the building is active (powered, staffed)
    uint8_t owner_id;           ///< Owning player ID (0 to MAX_PLAYERS-1)
    uint16_t capacity = 0;      ///< Population/being capacity (used by global services)
};

/**
 * @brief Calculate linear falloff for coverage strength.
 *
 * Applies linear falloff from full effectiveness at distance 0 to zero
 * at the edge of the radius.
 *
 * Formula: effectiveness * (1.0 - distance / radius)
 *
 * @param effectiveness Normalized effectiveness (0.0 - 1.0)
 * @param distance Manhattan distance from building to tile
 * @param radius Building coverage radius
 * @return Coverage strength (0.0 - 1.0). Returns 0.0 if distance >= radius or radius == 0.
 */
float calculate_falloff(float effectiveness, int distance, int radius);

/**
 * @brief Calculate radius-based coverage for all buildings and apply to grid.
 *
 * This function:
 * 1. Clears the grid
 * 2. For each active building, calculates coverage at all tiles within radius
 * 3. Applies linear falloff based on manhattan distance
 * 4. Uses max-value overlap (multiple buildings covering same tile keep highest value)
 *
 * Skips:
 * - Inactive buildings (is_active == false)
 * - Tiles outside map bounds (no wraparound)
 * - Tiles beyond the building's configured radius
 *
 * @param grid The coverage grid to populate (will be cleared first)
 * @param buildings Vector of service building data to calculate coverage from
 *
 * @note Currently treats all tiles as owned by all players.
 *       TODO: Add owner_id check when zone ownership grid is implemented.
 */
void calculate_radius_coverage(ServiceCoverageGrid& grid,
                                const std::vector<ServiceBuildingData>& buildings);

} // namespace services
} // namespace sims3000
