/**
 * @file ContaminationDecay.cpp
 * @brief Implementation of contamination decay algorithm.
 *
 * @see ContaminationDecay.h for documentation.
 * @see E10-088
 */

#include <sims3000/contamination/ContaminationDecay.h>

namespace sims3000 {
namespace contamination {

uint8_t calculate_decay_rate(const DecayTileInfo& info) {
    uint8_t total_decay = BASE_DECAY_RATE;

    // Water proximity bonus (natural filtration)
    if (info.water_distance <= 2) {
        total_decay += WATER_DECAY_BONUS;
    }

    // Bioremediation bonus (forest or spore plains)
    if (info.is_forest || info.is_spore_plains) {
        total_decay += BIO_DECAY_BONUS;
    }

    return total_decay;
}

void apply_contamination_decay(ContaminationGrid& grid,
                                const DecayTileInfo* tile_info) {
    const uint16_t width = grid.get_width();
    const uint16_t height = grid.get_height();

    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            // Skip tiles with no contamination
            uint8_t level = grid.get_level(x, y);
            if (level == 0) {
                continue;
            }

            uint8_t decay_rate;
            if (tile_info == nullptr) {
                // Uniform base decay only
                decay_rate = BASE_DECAY_RATE;
            } else {
                // Calculate decay based on environmental factors
                const size_t index = static_cast<size_t>(y) * width + static_cast<size_t>(x);
                decay_rate = calculate_decay_rate(tile_info[index]);
            }

            grid.apply_decay(x, y, decay_rate);
        }
    }
}

} // namespace contamination
} // namespace sims3000
