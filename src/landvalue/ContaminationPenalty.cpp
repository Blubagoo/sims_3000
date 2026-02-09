/**
 * @file ContaminationPenalty.cpp
 * @brief Implementation of contamination penalty calculation.
 *
 * @see ContaminationPenalty.h for algorithm documentation.
 * @see E10-104
 */

#include <sims3000/landvalue/ContaminationPenalty.h>

namespace sims3000 {
namespace landvalue {

uint8_t calculate_contamination_penalty(uint8_t contamination_level) {
    // Scale contamination (0-255) to penalty (0-50)
    // penalty = (contamination * 50) / 255
    const uint16_t scaled = static_cast<uint16_t>(contamination_level) * MAX_CONTAMINATION_PENALTY;
    return static_cast<uint8_t>(scaled / 255);
}

void apply_contamination_penalties(LandValueGrid& value_grid, const contamination::ContaminationGrid& contam_grid) {
    const uint16_t width = value_grid.get_width();
    const uint16_t height = value_grid.get_height();

    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            // Read contamination from previous tick buffer
            const uint8_t contam_level = contam_grid.get_level_previous_tick(x, y);

            // Calculate penalty
            const uint8_t penalty = calculate_contamination_penalty(contam_level);

            // Apply penalty (saturating subtraction)
            if (penalty > 0) {
                value_grid.subtract_value(x, y, penalty);
            }
        }
    }
}

} // namespace landvalue
} // namespace sims3000
