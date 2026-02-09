/**
 * @file RoadAccessBonus.cpp
 * @brief Implementation of road proximity-based land value bonus calculations.
 *
 * @see RoadAccessBonus.h for interface documentation.
 * @see E10-102
 */

#include <sims3000/landvalue/RoadAccessBonus.h>
#include <algorithm>

namespace sims3000 {
namespace landvalue {

uint8_t calculate_road_bonus(uint8_t road_distance) {
    switch (road_distance) {
        case 0: return road_bonus::ON_ROAD;       // +20
        case 1: return road_bonus::DISTANCE_1;    // +15
        case 2: return road_bonus::DISTANCE_2;    // +10
        case 3: return road_bonus::DISTANCE_3;    // +5
        default: return 0;
    }
}

void apply_road_bonuses(LandValueGrid& grid,
                        const std::vector<RoadDistanceInfo>& road_info) {
    for (const auto& tile : road_info) {
        uint8_t bonus = calculate_road_bonus(tile.road_distance);
        if (bonus == 0) {
            continue;
        }

        // Add bonus to current value with clamping to 0-255
        int current = static_cast<int>(grid.get_value(tile.x, tile.y));
        int new_value = current + static_cast<int>(bonus);
        new_value = std::min(255, new_value);
        grid.set_value(tile.x, tile.y, static_cast<uint8_t>(new_value));
    }
}

} // namespace landvalue
} // namespace sims3000
