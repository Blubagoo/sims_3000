/**
 * @file GridSwapCoordinator.h
 * @brief Coordinates double-buffer swaps for all grids at tick boundaries.
 *
 * GridSwapCoordinator manages the sequence of double-buffer swaps so that
 * all grids transition atomically at the start of each simulation tick.
 * Individual grids already implement swap_buffers(); this class ensures
 * they are all called together in the correct order.
 *
 * Swap sequence per tick:
 * 1. GridSwapCoordinator::swap_all() -- current becomes previous
 * 2. DisorderSystem writes to current grid, reads LandValueGrid (no double-buffer needed)
 * 3. ContaminationSystem writes to current grid
 * 4. LandValueSystem reads DisorderGrid.get_level_previous_tick() and
 *    ContaminationGrid.get_level_previous_tick()
 *
 * The key semantic: swap_all() should be called BEFORE any system writes
 * to current grids. After swap:
 * - Systems WRITE to current grid (via set_level, add_disorder, etc.)
 * - Systems READ from previous grid (via get_level_previous_tick) for
 *   cross-system dependencies
 *
 * @see E10-063
 */

#ifndef SIMS3000_SIM_GRIDSWAPCOORDINATOR_H
#define SIMS3000_SIM_GRIDSWAPCOORDINATOR_H

#include <sims3000/disorder/DisorderGrid.h>
#include <sims3000/contamination/ContaminationGrid.h>

namespace sims3000 {
namespace sim {

/**
 * @class GridSwapCoordinator
 * @brief Coordinates double-buffer swaps for all registered grids.
 *
 * Usage:
 * @code
 * GridSwapCoordinator coordinator;
 * coordinator.register_disorder_grid(&disorder_grid);
 * coordinator.register_contamination_grid(&contamination_grid);
 *
 * // Each tick:
 * coordinator.swap_all();  // Must be called before any system writes
 * // ... systems read from previous, write to current ...
 * @endcode
 */
class GridSwapCoordinator {
public:
    /**
     * @brief Register a disorder grid for coordinated swapping.
     * @param grid Pointer to the DisorderGrid. May be nullptr to unregister.
     */
    void register_disorder_grid(disorder::DisorderGrid* grid) {
        m_disorder_grid = grid;
    }

    /**
     * @brief Register a contamination grid for coordinated swapping.
     * @param grid Pointer to the ContaminationGrid. May be nullptr to unregister.
     */
    void register_contamination_grid(contamination::ContaminationGrid* grid) {
        m_contamination_grid = grid;
    }

    /**
     * @brief Swap all registered grids' double buffers.
     *
     * Call this at the start of each simulation tick, BEFORE any system
     * writes to the current buffers. After this call, what was the current
     * buffer becomes the previous buffer (readable via get_level_previous_tick),
     * and the old previous buffer becomes the new current buffer for writing.
     *
     * Safe to call with no grids registered (no-op) or with only some
     * grids registered (only registered grids are swapped).
     */
    void swap_all() {
        if (m_disorder_grid) {
            m_disorder_grid->swap_buffers();
        }
        if (m_contamination_grid) {
            m_contamination_grid->swap_buffers();
        }
    }

    /**
     * @brief Check if a disorder grid is registered.
     * @return true if a non-null disorder grid pointer has been registered.
     */
    bool has_disorder_grid() const {
        return m_disorder_grid != nullptr;
    }

    /**
     * @brief Check if a contamination grid is registered.
     * @return true if a non-null contamination grid pointer has been registered.
     */
    bool has_contamination_grid() const {
        return m_contamination_grid != nullptr;
    }

private:
    disorder::DisorderGrid* m_disorder_grid = nullptr;
    contamination::ContaminationGrid* m_contamination_grid = nullptr;
};

} // namespace sim
} // namespace sims3000

#endif // SIMS3000_SIM_GRIDSWAPCOORDINATOR_H
