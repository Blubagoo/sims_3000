/**
 * @file RoadAccessBonus.h
 * @brief Road proximity-based land value bonus calculations.
 *
 * Calculates land value bonuses based on distance to the nearest road.
 * Tiles on or near roads receive a bonus that decreases with distance.
 *
 * @see E10-102
 */

#ifndef SIMS3000_LANDVALUE_ROADACCESSBONUS_H
#define SIMS3000_LANDVALUE_ROADACCESSBONUS_H

#include <cstdint>
#include <vector>

#include "sims3000/landvalue/LandValueGrid.h"

namespace sims3000 {
namespace landvalue {

/// Road bonus constants by distance.
namespace road_bonus {
    constexpr uint8_t ON_ROAD    = 20;   ///< Tile is on a road (dist == 0)
    constexpr uint8_t DISTANCE_1 = 15;   ///< One tile from road (dist == 1)
    constexpr uint8_t DISTANCE_2 = 10;   ///< Two tiles from road (dist == 2)
    constexpr uint8_t DISTANCE_3 = 5;    ///< Three tiles from road (dist == 3)
} // namespace road_bonus

/**
 * @struct RoadDistanceInfo
 * @brief Road distance information for a single tile.
 */
struct RoadDistanceInfo {
    int32_t x;                  ///< Tile X coordinate
    int32_t y;                  ///< Tile Y coordinate
    uint8_t road_distance;      ///< 0=on road, 1=adjacent, 2=two away, 255=no road
};

/**
 * @brief Calculate road bonus for a single tile.
 *
 * - dist 0: +20
 * - dist 1: +15
 * - dist 2: +10
 * - dist 3: +5
 * - dist > 3: +0
 *
 * @param road_distance Distance to nearest road (0=on road, 255=no road).
 * @return Road bonus (0-20).
 */
uint8_t calculate_road_bonus(uint8_t road_distance);

/**
 * @brief Apply road bonuses to entire grid.
 *
 * For each tile in road_info:
 * - Computes bonus via calculate_road_bonus()
 * - Adds bonus to current value: new_value = clamp(get_value() + bonus, 0, 255)
 *
 * @param grid The land value grid to update.
 * @param road_info Road distance for each tile.
 */
void apply_road_bonuses(LandValueGrid& grid,
                        const std::vector<RoadDistanceInfo>& road_info);

} // namespace landvalue
} // namespace sims3000

#endif // SIMS3000_LANDVALUE_ROADACCESSBONUS_H
