/**
 * @file OccupancyDistribution.h
 * @brief Building occupancy distribution (Ticket E10-022)
 *
 * Distributes population across habitation buildings based on
 * capacity and zone type. Calculates per-building occupancy
 * and occupancy state classification.
 */

#ifndef SIMS3000_POPULATION_OCCUPANCYDISTRIBUTION_H
#define SIMS3000_POPULATION_OCCUPANCYDISTRIBUTION_H

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace population {

/**
 * @struct BuildingOccupancyInput
 * @brief Input data for a single building.
 *
 * Contains building identifier, capacity, and zone type
 * for occupancy distribution calculation.
 */
struct BuildingOccupancyInput {
    uint32_t building_id;   ///< Unique building identifier
    uint32_t capacity;      ///< Maximum occupant capacity
    uint8_t zone_type;      ///< Zone type: 0=habitation, 1=exchange, 2=fabrication
};

/**
 * @struct OccupancyResult
 * @brief Result of occupancy distribution for a single building.
 *
 * Contains the assigned occupancy count and occupancy state
 * classification for a building.
 */
struct OccupancyResult {
    uint32_t building_id;   ///< Building identifier (matches input)
    uint32_t occupancy;     ///< Assigned occupancy count
    uint8_t state;          ///< OccupancyState enum value (0-4)
};

/**
 * @brief Distribute population across habitation buildings.
 *
 * Algorithm:
 * - Filter to habitation buildings (zone_type == 0) only
 * - Calculate total capacity across all habitation buildings
 * - If total_beings >= total_capacity: fill all buildings to capacity (FullyOccupied)
 * - If total_beings < total_capacity: distribute proportionally by capacity share
 * - Set OccupancyState based on occupancy ratio:
 *   - Empty: 0%
 *   - UnderOccupied: <50%
 *   - NormalOccupied: 50-90%
 *   - FullyOccupied: >90% and <=100%
 *   - Overcrowded: >100%
 *
 * @param total_beings Total population to distribute
 * @param buildings Vector of building input data
 * @return Vector of occupancy results for habitation buildings only
 */
std::vector<OccupancyResult> distribute_occupancy(
    uint32_t total_beings,
    const std::vector<BuildingOccupancyInput>& buildings
);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_OCCUPANCYDISTRIBUTION_H
