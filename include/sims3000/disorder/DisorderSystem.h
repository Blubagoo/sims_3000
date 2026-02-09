/**
 * @file DisorderSystem.h
 * @brief Disorder simulation system skeleton (Ticket E10-072)
 *
 * Manages the disorder overlay grid and runs simulation phases each tick.
 * Runs at tick_priority 70 and owns a DisorderGrid for double-buffered
 * disorder level tracking.
 *
 * Tick phases (all stubs in this skeleton):
 * 1. swap_buffers() — double-buffer rotation
 * 2. generate() — disorder source generation (E10-073)
 * 3. apply_land_value() — land value modifier (E10-074)
 * 4. apply_spread() — disorder diffusion (E10-075)
 * 5. apply_suppression() — enforcer suppression (E10-076)
 * 6. update_stats() — aggregate statistic recalculation
 *
 * @see E10-072
 */

#ifndef SIMS3000_DISORDER_DISORDERSYSTEM_H
#define SIMS3000_DISORDER_DISORDERSYSTEM_H

#include <cstdint>

#include "sims3000/core/ISimulatable.h"
#include "sims3000/disorder/DisorderGrid.h"

namespace sims3000 {
namespace disorder {

/**
 * @class DisorderSystem
 * @brief Manages disorder simulation across the city grid.
 *
 * Owns a DisorderGrid and runs phased simulation each tick:
 * swap buffers, generate disorder, apply land value modifiers,
 * spread disorder to neighbors, apply enforcer suppression,
 * and update aggregate statistics.
 */
class DisorderSystem : public ISimulatable {
public:
    /**
     * @brief Construct a DisorderSystem with the specified grid dimensions.
     * @param grid_width Grid width in tiles.
     * @param grid_height Grid height in tiles.
     */
    DisorderSystem(uint16_t grid_width, uint16_t grid_height);

    // ISimulatable interface
    void tick(const ISimulationTime& time) override;
    int getPriority() const override { return 70; }
    const char* getName() const override { return "DisorderSystem"; }

    /**
     * @brief Get const reference to the disorder grid.
     * @return Const reference to the internal DisorderGrid.
     */
    const DisorderGrid& get_grid() const;

    /**
     * @brief Get mutable reference to the disorder grid.
     * @return Mutable reference to the internal DisorderGrid.
     */
    DisorderGrid& get_grid_mut();

    /**
     * @brief Get the total disorder across all tiles.
     * @return Sum of all disorder levels in the grid.
     */
    uint32_t get_total_disorder() const;

    /**
     * @brief Get the count of tiles with disorder above a threshold.
     * @param threshold Minimum disorder level to count (default 128).
     * @return Number of tiles at or above the threshold.
     */
    uint32_t get_high_disorder_tiles(uint8_t threshold = 128) const;

private:
    /**
     * @brief Generate disorder from sources.
     * Stub implementation; filled in by E10-073.
     */
    void generate();

    /**
     * @brief Apply land value modifiers to disorder levels.
     * Stub implementation; filled in by E10-074.
     */
    void apply_land_value();

    /**
     * @brief Spread disorder to neighboring tiles via diffusion.
     * Stub implementation; filled in by E10-075.
     */
    void apply_spread();

    /**
     * @brief Apply enforcer suppression to reduce disorder.
     * Stub implementation; filled in by E10-076.
     */
    void apply_suppression();

    /**
     * @brief Recalculate aggregate disorder statistics.
     */
    void update_stats();

    /// The disorder overlay grid (double-buffered)
    DisorderGrid m_grid;
};

} // namespace disorder
} // namespace sims3000

#endif // SIMS3000_DISORDER_DISORDERSYSTEM_H
