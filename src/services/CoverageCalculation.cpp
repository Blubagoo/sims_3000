/**
 * @file CoverageCalculation.cpp
 * @brief Implementation of radius-based coverage calculation (Epic 9, Ticket E9-020)
 *
 * @see CoverageCalculation.h for function documentation.
 */

#include <sims3000/services/CoverageCalculation.h>
#include <sims3000/services/ServiceCoverageGrid.h>
#include <sims3000/services/ServiceTypes.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace sims3000 {
namespace services {

float calculate_falloff(float effectiveness, int distance, int radius) {
    if (radius <= 0 || distance >= radius) {
        return 0.0f;
    }
    if (distance < 0) {
        distance = -distance;
    }
    if (distance >= radius) {
        return 0.0f;
    }

    // Linear falloff: strength = effectiveness * (1.0 - distance / radius)
    float falloff = 1.0f - static_cast<float>(distance) / static_cast<float>(radius);
    return effectiveness * falloff;
}

void calculate_radius_coverage(ServiceCoverageGrid& grid,
                                const std::vector<ServiceBuildingData>& buildings) {
    // Step 1: Clear the grid
    grid.clear();

    const int32_t map_w = static_cast<int32_t>(grid.get_width());
    const int32_t map_h = static_cast<int32_t>(grid.get_height());

    // Step 2: For each active building, calculate coverage
    for (const auto& building : buildings) {
        // Skip inactive buildings
        if (!building.is_active) {
            continue;
        }

        // Look up config for this building's type and tier
        if (!isValidServiceTier(building.tier)) {
            continue;
        }
        ServiceTier tier = static_cast<ServiceTier>(building.tier);
        ServiceConfig config = get_service_config(building.type, tier);

        int radius = static_cast<int>(config.base_radius);
        if (radius <= 0) {
            continue;
        }

        // Normalize effectiveness from 0-255 to 0.0-1.0
        float effectiveness = static_cast<float>(building.effectiveness) / 255.0f;

        // TODO: Skip tiles not owned by building owner (use owner_id check).
        // For now, treat all tiles as owned since no zone ownership grid exists yet.

        // Iterate tiles within bounding box [-radius, +radius] around building
        int32_t min_x = building.x - radius;
        int32_t max_x = building.x + radius;
        int32_t min_y = building.y - radius;
        int32_t max_y = building.y + radius;

        // Clamp to map bounds (no wraparound)
        if (min_x < 0) min_x = 0;
        if (min_y < 0) min_y = 0;
        if (max_x >= map_w) max_x = map_w - 1;
        if (max_y >= map_h) max_y = map_h - 1;

        for (int32_t ty = min_y; ty <= max_y; ++ty) {
            for (int32_t tx = min_x; tx <= max_x; ++tx) {
                // Step 3: Calculate manhattan distance
                int distance = std::abs(tx - building.x) + std::abs(ty - building.y);

                // Skip tiles beyond radius
                if (distance > radius) {
                    continue;
                }

                // Step 4: Apply linear falloff
                float strength = calculate_falloff(effectiveness, distance, radius);

                // Step 5: Convert to uint8_t (0-255)
                uint8_t coverage_value = static_cast<uint8_t>(
                    std::min(255.0f, strength * 255.0f + 0.5f)  // Round to nearest
                );

                // Step 6: Max-value overlap
                uint32_t ux = static_cast<uint32_t>(tx);
                uint32_t uy = static_cast<uint32_t>(ty);
                uint8_t existing = grid.get_coverage_at(ux, uy);
                if (coverage_value > existing) {
                    grid.set_coverage_at(ux, uy, coverage_value);
                }
            }
        }
    }
}

} // namespace services
} // namespace sims3000
