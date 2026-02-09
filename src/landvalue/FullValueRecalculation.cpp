/**
 * @file FullValueRecalculation.cpp
 * @brief Implementation of full land value recalculation.
 *
 * @see FullValueRecalculation.h for documentation.
 * @see E10-105
 */

#include <sims3000/landvalue/FullValueRecalculation.h>
#include <sims3000/landvalue/TerrainValueFactors.h>
#include <sims3000/landvalue/RoadAccessBonus.h>
#include <sims3000/landvalue/DisorderPenalty.h>
#include <sims3000/landvalue/ContaminationPenalty.h>
#include <algorithm>

namespace sims3000 {
namespace landvalue {

uint8_t calculate_tile_value(const LandValueTileInput& input) {
    // Start with base value
    int32_t value = static_cast<int32_t>(BASE_LAND_VALUE);

    // Add terrain bonus (can be negative for toxic terrain)
    int8_t terrain_bonus = calculate_terrain_bonus(input.terrain_type, input.water_distance);
    value += static_cast<int32_t>(terrain_bonus);

    // Add road bonus
    uint8_t road_bonus = calculate_road_bonus(input.road_distance);
    value += static_cast<int32_t>(road_bonus);

    // Subtract disorder penalty
    uint8_t disorder_penalty = calculate_disorder_penalty(input.disorder_level);
    value -= static_cast<int32_t>(disorder_penalty);

    // Subtract contamination penalty
    uint8_t contam_penalty = calculate_contamination_penalty(input.contam_level);
    value -= static_cast<int32_t>(contam_penalty);

    // Clamp to [0, 255]
    if (value < 0) {
        return 0;
    } else if (value > 255) {
        return 255;
    } else {
        return static_cast<uint8_t>(value);
    }
}

void recalculate_all_values(LandValueGrid& grid,
                             const LandValueTileInput* tile_inputs,
                             uint32_t tile_count) {
    const uint16_t width = grid.get_width();
    const uint16_t height = grid.get_height();

    // Verify tile count matches grid size
    const uint32_t expected_count = static_cast<uint32_t>(width) * static_cast<uint32_t>(height);
    if (tile_count != expected_count) {
        return;  // Mismatch, do nothing
    }

    // Recalculate each tile
    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            const size_t index = static_cast<size_t>(y) * width + static_cast<size_t>(x);
            const LandValueTileInput& input = tile_inputs[index];

            uint8_t final_value = calculate_tile_value(input);
            grid.set_value(x, y, final_value);
        }
    }
}

} // namespace landvalue
} // namespace sims3000
