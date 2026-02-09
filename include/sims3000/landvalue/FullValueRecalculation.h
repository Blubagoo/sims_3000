/**
 * @file FullValueRecalculation.h
 * @brief Full land value recalculation combining all factors.
 *
 * Performs complete land value recalculation each tick by combining:
 * - Base value (128 neutral)
 * - Terrain bonuses (from TerrainValueFactors)
 * - Road accessibility bonus (from RoadAccessBonus)
 * - Disorder penalty (from DisorderPenalty)
 * - Contamination penalty (from ContaminationPenalty)
 *
 * Final value is clamped to [0, 255].
 *
 * @see E10-105
 */

#ifndef SIMS3000_LANDVALUE_FULLVALUERECALCULATION_H
#define SIMS3000_LANDVALUE_FULLVALUERECALCULATION_H

#include <sims3000/landvalue/LandValueGrid.h>
#include <cstdint>

namespace sims3000 {
namespace landvalue {

/// Base land value (neutral starting point)
constexpr uint8_t BASE_LAND_VALUE = 128;

/**
 * @struct LandValueTileInput
 * @brief Per-tile input data for full land value recalculation.
 */
struct LandValueTileInput {
    uint8_t terrain_type;      ///< From terrain grid
    uint8_t water_distance;    ///< Distance to water
    uint8_t road_distance;     ///< Distance to nearest road (0=on road, 255=no road)
    uint8_t disorder_level;    ///< From DisorderGrid previous tick
    uint8_t contam_level;      ///< From ContaminationGrid previous tick
};

/**
 * @brief Calculate land value for a single tile.
 *
 * Algorithm:
 * 1. Start with BASE_LAND_VALUE (128)
 * 2. Add terrain_bonus (can be negative for toxic terrain)
 * 3. Add road_bonus
 * 4. Subtract disorder_penalty
 * 5. Subtract contamination_penalty
 * 6. Clamp to [0, 255]
 *
 * Uses the following calculation functions from other modules:
 * - calculate_terrain_bonus(terrain_type, water_distance)
 * - calculate_road_bonus(road_distance)
 * - calculate_disorder_penalty(disorder_level)
 * - calculate_contamination_penalty(contam_level)
 *
 * @param input Tile input data.
 * @return Final land value (0-255).
 */
uint8_t calculate_tile_value(const LandValueTileInput& input);

/**
 * @brief Recalculate entire land value grid.
 *
 * For each tile in tile_inputs:
 * - Calculate final value via calculate_tile_value()
 * - Set value in the grid
 *
 * @param grid Land value grid to update.
 * @param tile_inputs Array of input data for each tile (row-major order).
 * @param tile_count Number of tiles (must match grid width * height).
 */
void recalculate_all_values(LandValueGrid& grid,
                             const LandValueTileInput* tile_inputs,
                             uint32_t tile_count);

} // namespace landvalue
} // namespace sims3000

#endif // SIMS3000_LANDVALUE_FULLVALUERECALCULATION_H
