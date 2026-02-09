/**
 * @file OccupancyDistribution.cpp
 * @brief Implementation of occupancy distribution (Ticket E10-022)
 */

#include "sims3000/population/OccupancyDistribution.h"
#include "sims3000/population/BuildingOccupancyComponent.h"
#include <algorithm>

namespace sims3000 {
namespace population {

// Helper function to determine OccupancyState from ratio
static uint8_t calculate_occupancy_state(uint32_t occupancy, uint32_t capacity) {
    if (capacity == 0) {
        return static_cast<uint8_t>(OccupancyState::Empty);
    }

    float ratio = static_cast<float>(occupancy) / static_cast<float>(capacity);

    if (ratio == 0.0f) {
        return static_cast<uint8_t>(OccupancyState::Empty);
    } else if (ratio < 0.5f) {
        return static_cast<uint8_t>(OccupancyState::UnderOccupied);
    } else if (ratio <= 0.9f) {
        return static_cast<uint8_t>(OccupancyState::NormalOccupied);
    } else if (ratio <= 1.0f) {
        return static_cast<uint8_t>(OccupancyState::FullyOccupied);
    } else {
        return static_cast<uint8_t>(OccupancyState::Overcrowded);
    }
}

std::vector<OccupancyResult> distribute_occupancy(
    uint32_t total_beings,
    const std::vector<BuildingOccupancyInput>& buildings
) {
    std::vector<OccupancyResult> results;

    // Filter to habitation buildings only (zone_type == 0)
    std::vector<BuildingOccupancyInput> hab_buildings;
    for (const auto& building : buildings) {
        if (building.zone_type == 0) {
            hab_buildings.push_back(building);
        }
    }

    // If no habitation buildings, return empty results
    if (hab_buildings.empty()) {
        return results;
    }

    // Calculate total capacity
    uint64_t total_capacity = 0;
    for (const auto& building : hab_buildings) {
        total_capacity += building.capacity;
    }

    // If total capacity is zero, all buildings have zero occupancy
    if (total_capacity == 0) {
        for (const auto& building : hab_buildings) {
            results.push_back(OccupancyResult{
                building.building_id,
                0,
                static_cast<uint8_t>(OccupancyState::Empty)
            });
        }
        return results;
    }

    // Case 1: Total beings >= total capacity - fill all buildings to capacity
    if (total_beings >= total_capacity) {
        for (const auto& building : hab_buildings) {
            results.push_back(OccupancyResult{
                building.building_id,
                building.capacity,
                static_cast<uint8_t>(OccupancyState::FullyOccupied)
            });
        }
        return results;
    }

    // Case 2: Total beings < total capacity - distribute proportionally
    uint32_t distributed = 0;
    for (size_t i = 0; i < hab_buildings.size(); ++i) {
        const auto& building = hab_buildings[i];
        uint32_t occupancy;

        // For the last building, assign remaining to avoid rounding errors
        if (i == hab_buildings.size() - 1) {
            occupancy = total_beings - distributed;
        } else {
            // Proportional share: (building_capacity / total_capacity) * total_beings
            uint64_t share = (static_cast<uint64_t>(building.capacity) * total_beings) / total_capacity;
            occupancy = static_cast<uint32_t>(share);
            distributed += occupancy;
        }

        uint8_t state = calculate_occupancy_state(occupancy, building.capacity);

        results.push_back(OccupancyResult{
            building.building_id,
            occupancy,
            state
        });
    }

    return results;
}

} // namespace population
} // namespace sims3000
