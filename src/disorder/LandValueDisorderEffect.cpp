/**
 * @file LandValueDisorderEffect.cpp
 * @brief Implementation of land value effect on disorder levels.
 *
 * @see LandValueDisorderEffect.h for function documentation.
 * @see E10-074
 */

#include <sims3000/disorder/LandValueDisorderEffect.h>
#include <algorithm>

namespace sims3000 {
namespace disorder {

void apply_land_value_effect(DisorderGrid& grid, const landvalue::LandValueGrid& land_value_grid) {
    const uint16_t width = grid.get_width();
    const uint16_t height = grid.get_height();

    for (int32_t y = 0; y < static_cast<int32_t>(height); ++y) {
        for (int32_t x = 0; x < static_cast<int32_t>(width); ++x) {
            uint8_t disorder = grid.get_level(x, y);
            if (disorder == 0) {
                continue;
            }

            uint8_t land_value = land_value_grid.get_value(x, y);

            // extra = disorder * (1.0 - land_value / 255.0)
            // Low land value (0) -> extra = disorder (doubles it)
            // High land value (255) -> extra = 0
            float factor = 1.0f - static_cast<float>(land_value) / 255.0f;
            float extra_f = static_cast<float>(disorder) * factor;
            uint8_t extra = static_cast<uint8_t>(std::min(extra_f, 255.0f));

            // Add extra with saturation
            grid.add_disorder(x, y, extra);
        }
    }
}

} // namespace disorder
} // namespace sims3000
