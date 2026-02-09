/**
 * @file EnforcerSuppression.cpp
 * @brief Implementation of enforcer-based disorder suppression (Ticket E10-076).
 */

#include <sims3000/disorder/EnforcerSuppression.h>
#include <sims3000/services/ServiceTypes.h>

namespace sims3000 {
namespace disorder {

void apply_enforcer_suppression(
    DisorderGrid& grid,
    const building::IServiceQueryable& service_query,
    uint8_t player_id
) {
    const uint16_t width = grid.get_width();
    const uint16_t height = grid.get_height();

    // ServiceType::Enforcer = 0 (from ServiceTypes.h)
    constexpr uint8_t ENFORCER_SERVICE_TYPE = static_cast<uint8_t>(services::ServiceType::Enforcer);

    // Get global effectiveness for enforcers (applies to all tiles)
    const float effectiveness = service_query.get_effectiveness(ENFORCER_SERVICE_TYPE, player_id);

    // Iterate over all tiles
    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            // Get enforcer coverage at this tile
            const float coverage = service_query.get_coverage_at(ENFORCER_SERVICE_TYPE, x, y, player_id);

            // Skip tiles with no enforcer coverage
            if (coverage <= 0.0f) {
                continue;
            }

            // Get current disorder level
            const uint8_t current_level = grid.get_level(x, y);

            // Skip tiles with no disorder
            if (current_level == 0) {
                continue;
            }

            // Calculate suppression amount
            // suppression = current_level * coverage * effectiveness * ENFORCER_SUPPRESSION_MULTIPLIER
            const float suppression_float = current_level * coverage * effectiveness * services::ENFORCER_SUPPRESSION_MULTIPLIER;
            const uint8_t suppression = static_cast<uint8_t>(suppression_float);

            // Apply suppression (saturating subtraction)
            grid.apply_suppression(x, y, suppression);
        }
    }
}

} // namespace disorder
} // namespace sims3000
