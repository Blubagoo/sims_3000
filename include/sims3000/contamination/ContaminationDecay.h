/**
 * @file ContaminationDecay.h
 * @brief Natural contamination decay over time with environmental modifiers.
 *
 * Implements contamination decay that reduces contamination levels each tick.
 * Decay rate is affected by:
 * - Water proximity (natural filtration)
 * - Forest/spore plains terrain (bioremediation)
 *
 * @see E10-088
 */

#ifndef SIMS3000_CONTAMINATION_CONTAMINATIONDECAY_H
#define SIMS3000_CONTAMINATION_CONTAMINATIONDECAY_H

#include <sims3000/contamination/ContaminationGrid.h>
#include <cstdint>

namespace sims3000 {
namespace contamination {

/// Base decay rate applied to all tiles per tick
constexpr uint8_t BASE_DECAY_RATE = 2;

/// Additional decay for tiles near water (natural filtration)
constexpr uint8_t WATER_DECAY_BONUS = 3;

/// Additional decay for forest/spore plains (bioremediation)
constexpr uint8_t BIO_DECAY_BONUS = 3;

/**
 * @struct DecayTileInfo
 * @brief Information about a tile's decay modifiers.
 */
struct DecayTileInfo {
    uint8_t water_distance;  ///< Distance to nearest water (0 = on water, 255 = far)
    bool is_forest;          ///< BiolumeGrove terrain
    bool is_spore_plains;    ///< SporeFlats terrain
};

/**
 * @brief Calculate total decay rate for a tile.
 *
 * Combines base decay with environmental modifiers:
 * - Base: BASE_DECAY_RATE (2)
 * - Water proximity (dist <= 2): +WATER_DECAY_BONUS (3)
 * - Forest or spore plains: +BIO_DECAY_BONUS (3)
 *
 * @param info Tile environmental information.
 * @return Total decay rate (2-8).
 */
uint8_t calculate_decay_rate(const DecayTileInfo& info);

/**
 * @brief Apply decay to entire contamination grid.
 *
 * For each tile:
 * - If tile_info is nullptr: apply BASE_DECAY_RATE to all tiles
 * - Otherwise: calculate decay rate based on environmental factors
 * - Apply decay using grid.apply_decay()
 *
 * Only processes tiles with contamination > 0 for efficiency.
 *
 * @param grid Contamination grid to update.
 * @param tile_info Array of per-tile decay modifiers (can be nullptr for uniform decay).
 */
void apply_contamination_decay(ContaminationGrid& grid,
                                const DecayTileInfo* tile_info = nullptr);

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_CONTAMINATIONDECAY_H
