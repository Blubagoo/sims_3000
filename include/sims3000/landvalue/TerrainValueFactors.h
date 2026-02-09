/**
 * @file TerrainValueFactors.h
 * @brief Terrain-based land value bonus calculations.
 *
 * Calculates land value bonuses based on terrain type (crystal fields,
 * spore plains, forest, toxic marshes) and water proximity. These bonuses
 * are applied to the LandValueGrid during the terrain phase of land value
 * recalculation.
 *
 * @see E10-101
 */

#ifndef SIMS3000_LANDVALUE_TERRAINVALUEFACTORS_H
#define SIMS3000_LANDVALUE_TERRAINVALUEFACTORS_H

#include <cstdint>
#include <vector>

#include "sims3000/landvalue/LandValueGrid.h"

namespace sims3000 {
namespace landvalue {

/// Terrain bonus constants for each terrain type.
namespace terrain_bonus {
    constexpr int8_t WATER_ADJACENT = 30;    ///< Adjacent to water (dist <= 1)
    constexpr int8_t WATER_1_TILE   = 20;    ///< One tile from water (dist == 2)
    constexpr int8_t WATER_2_TILES  = 10;    ///< Two tiles from water (dist == 3)
    constexpr int8_t CRYSTAL_FIELDS = 25;    ///< Prisma fields terrain bonus
    constexpr int8_t SPORE_PLAINS   = 15;    ///< Spore plains terrain bonus
    constexpr int8_t FOREST         = 10;    ///< Biolume grove terrain bonus
    constexpr int8_t TOXIC_MARSHES  = -30;   ///< Blight mires terrain penalty
} // namespace terrain_bonus

/**
 * @struct TerrainTileInfo
 * @brief Terrain information for a single tile, used as input for bonus calculation.
 */
struct TerrainTileInfo {
    int32_t x;                  ///< Tile X coordinate
    int32_t y;                  ///< Tile Y coordinate
    uint8_t terrain_type;       ///< Terrain enum value
    uint8_t water_distance;     ///< 0=water tile, 1=adjacent, 2=one tile away, etc.
};

/**
 * @brief Calculate terrain bonus for a single tile.
 *
 * Computes a bonus from two independent factors:
 * - Base bonus from terrain type: crystal=+25, spore=+15, forest=+10, toxic=-30, others=0
 * - Water proximity: dist<=1 = +30, dist==2 = +20, dist==3 = +10, dist>3 = 0
 *
 * @param terrain_type Terrain enum value.
 * @param water_distance Distance to nearest water tile (0=water, 1=adjacent, etc.)
 * @return Combined bonus (can be negative for toxic terrain with no water).
 */
int8_t calculate_terrain_bonus(uint8_t terrain_type, uint8_t water_distance);

/**
 * @brief Apply terrain bonuses to the entire grid.
 *
 * For each tile in terrain_info:
 * - Computes bonus via calculate_terrain_bonus()
 * - Stores positive bonuses via set_terrain_bonus() (negative bonuses stored as 0)
 * - Adds bonus to current value: new_value = clamp(get_value() + bonus, 0, 255)
 *
 * @param grid The land value grid to update.
 * @param terrain_info Terrain type and water distance for each tile.
 */
void apply_terrain_bonuses(LandValueGrid& grid,
                           const std::vector<TerrainTileInfo>& terrain_info);

} // namespace landvalue
} // namespace sims3000

#endif // SIMS3000_LANDVALUE_TERRAINVALUEFACTORS_H
