/**
 * @file ContaminationSystem.h
 * @brief Contamination simulation system skeleton (Ticket E10-081)
 *
 * Manages the contamination overlay grid. Runs at tick_priority 80
 * with the following tick phases:
 * - swap_buffers: double-buffer swap
 * - generate: emit contamination from sources (E10-082..086)
 * - apply_spread: diffuse contamination to neighbors (E10-087)
 * - apply_decay: reduce contamination over time (E10-088)
 * - update_stats: recalculate aggregate statistics
 *
 * Phase implementations are stubs in this skeleton; they will be
 * filled in by later tickets.
 */

#ifndef SIMS3000_CONTAMINATION_CONTAMINATIONSYSTEM_H
#define SIMS3000_CONTAMINATION_CONTAMINATIONSYSTEM_H

#include <cstdint>

#include "sims3000/core/ISimulatable.h"
#include "sims3000/contamination/ContaminationGrid.h"

namespace sims3000 {
namespace contamination {

/**
 * @class ContaminationSystem
 * @brief Manages environmental contamination simulation.
 *
 * Owns a ContaminationGrid and processes contamination generation,
 * spread, and decay each simulation tick. Implements ISimulatable
 * at priority 80.
 *
 * @see ContaminationGrid
 * @see E10-081
 */
class ContaminationSystem : public ISimulatable {
public:
    /**
     * @brief Construct a contamination system with the specified grid dimensions.
     * @param grid_width Grid width in tiles.
     * @param grid_height Grid height in tiles.
     */
    ContaminationSystem(uint16_t grid_width, uint16_t grid_height);

    // ISimulatable interface

    /**
     * @brief Called once per simulation tick (20 Hz).
     *
     * Executes the following phases in order:
     * 1. swap_buffers() - swap double buffers
     * 2. generate() - emit contamination from sources
     * 3. apply_spread() - diffuse contamination
     * 4. apply_decay() - reduce contamination over time
     * 5. update_stats() - recalculate aggregate statistics
     *
     * @param time Read-only access to simulation timing.
     */
    void tick(const ISimulationTime& time) override;

    /**
     * @brief Get the priority of this system.
     * @return 80 (runs after population, before land value).
     */
    int getPriority() const override { return 80; }

    /**
     * @brief Get the name of this system for debugging.
     * @return "ContaminationSystem"
     */
    const char* getName() const override { return "ContaminationSystem"; }

    // Grid access

    /**
     * @brief Get read-only access to the contamination grid.
     * @return Const reference to the contamination grid.
     */
    const ContaminationGrid& get_grid() const;

    /**
     * @brief Get mutable access to the contamination grid.
     * @return Mutable reference to the contamination grid.
     */
    ContaminationGrid& get_grid_mut();

    // Stats

    /**
     * @brief Get the total contamination across all cells.
     * @return Sum of all contamination levels.
     */
    uint32_t get_total_contamination() const;

    /**
     * @brief Get the count of tiles with contamination above a threshold.
     * @param threshold Minimum contamination level to count (default 128).
     * @return Number of tiles at or above the threshold.
     */
    uint32_t get_toxic_tiles(uint8_t threshold = 128) const;

private:
    /**
     * @brief Generate contamination from active sources.
     * Stub for now; implemented by E10-082, 083, 084, 085, 086.
     */
    void generate();

    /**
     * @brief Spread contamination to neighboring cells.
     * Stub for now; implemented by E10-087.
     */
    void apply_spread();

    /**
     * @brief Apply natural decay to contamination levels.
     * Stub for now; implemented by E10-088.
     */
    void apply_decay();

    /**
     * @brief Recalculate aggregate statistics from grid data.
     */
    void update_stats();

    ContaminationGrid m_grid;  ///< Double-buffered contamination grid
};

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_CONTAMINATIONSYSTEM_H
