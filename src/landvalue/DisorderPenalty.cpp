/**
 * @file DisorderPenalty.cpp
 * @brief Implementation of disorder penalty calculation.
 *
 * @see DisorderPenalty.h for algorithm documentation.
 * @see E10-103
 */

#include <sims3000/landvalue/DisorderPenalty.h>

namespace sims3000 {
namespace landvalue {

uint8_t calculate_disorder_penalty(uint8_t disorder_level) {
    // Scale disorder (0-255) to penalty (0-40)
    // penalty = (disorder * 40) / 255
    const uint16_t scaled = static_cast<uint16_t>(disorder_level) * MAX_DISORDER_PENALTY;
    return static_cast<uint8_t>(scaled / 255);
}

void apply_disorder_penalties(LandValueGrid& value_grid, const disorder::DisorderGrid& disorder_grid) {
    const uint16_t width = value_grid.get_width();
    const uint16_t height = value_grid.get_height();

    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            // Read disorder from previous tick buffer
            const uint8_t disorder_level = disorder_grid.get_level_previous_tick(x, y);

            // Calculate penalty
            const uint8_t penalty = calculate_disorder_penalty(disorder_level);

            // Apply penalty (saturating subtraction)
            if (penalty > 0) {
                value_grid.subtract_value(x, y, penalty);
            }
        }
    }
}

} // namespace landvalue
} // namespace sims3000
