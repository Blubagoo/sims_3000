/**
 * @file LandValueSystem.h
 * @brief Land value simulation system skeleton (Ticket E10-100)
 *
 * Manages the land value overlay grid. Runs at tick_priority 85
 * with the following tick phases:
 * - reset_values: reset grid to neutral (128)
 * - apply_terrain_bonus: add terrain-based value (E10-101)
 * - apply_road_bonus: add road proximity value (E10-102)
 * - apply_disorder_penalty: subtract disorder penalty (E10-103)
 * - apply_contamination_penalty: subtract contamination penalty (E10-104)
 *
 * Phase implementations are stubs in this skeleton; they will be
 * filled in by later tickets.
 */

#ifndef SIMS3000_LANDVALUE_LANDVALUESYSTEM_H
#define SIMS3000_LANDVALUE_LANDVALUESYSTEM_H

#include <cstdint>

#include "sims3000/core/ISimulatable.h"
#include "sims3000/landvalue/LandValueGrid.h"

namespace sims3000 {
namespace landvalue {

/**
 * @class LandValueSystem
 * @brief Manages land value recalculation each simulation tick.
 *
 * Owns a LandValueGrid and recalculates land values from terrain,
 * road proximity, disorder, and contamination factors. Implements
 * ISimulatable at priority 85 (runs after contamination system).
 *
 * @see LandValueGrid
 * @see E10-100
 */
class LandValueSystem : public ISimulatable {
public:
    /**
     * @brief Construct a land value system with the specified grid dimensions.
     * @param grid_width Grid width in tiles.
     * @param grid_height Grid height in tiles.
     */
    LandValueSystem(uint16_t grid_width, uint16_t grid_height);

    // ISimulatable interface

    /**
     * @brief Called once per simulation tick (20 Hz).
     *
     * Executes the following phases in order:
     * 1. reset_values() - reset all values to neutral (128)
     * 2. apply_terrain_bonus() - add terrain-based bonuses
     * 3. apply_road_bonus() - add road proximity bonuses
     * 4. apply_disorder_penalty() - subtract disorder penalties
     * 5. apply_contamination_penalty() - subtract contamination penalties
     *
     * @param time Read-only access to simulation timing.
     */
    void tick(const ISimulationTime& time) override;

    /**
     * @brief Get the priority of this system.
     * @return 85 (runs after contamination system).
     */
    int getPriority() const override { return 85; }

    /**
     * @brief Get the name of this system for debugging.
     * @return "LandValueSystem"
     */
    const char* getName() const override { return "LandValueSystem"; }

    // Grid access

    /**
     * @brief Get read-only access to the land value grid.
     * @return Const reference to the land value grid.
     */
    const LandValueGrid& get_grid() const;

    /**
     * @brief Get mutable access to the land value grid.
     * @return Mutable reference to the land value grid.
     */
    LandValueGrid& get_grid_mut();

    // Value query (implements ILandValueProvider concept)

    /**
     * @brief Get the land value at the specified coordinates.
     *
     * Returns the grid value cast to float (0-255 -> 0.0-255.0).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Land value as float (0.0-255.0).
     */
    float get_land_value(uint32_t x, uint32_t y) const;

private:
    /**
     * @brief Recalculate all land values from scratch.
     * Stub for now; implemented by E10-105.
     */
    void recalculate();

    /**
     * @brief Apply terrain-based value bonuses.
     * Stub for now; implemented by E10-101.
     */
    void apply_terrain_bonus();

    /**
     * @brief Apply road proximity value bonuses.
     * Stub for now; implemented by E10-102.
     */
    void apply_road_bonus();

    /**
     * @brief Apply disorder-based value penalties.
     * Stub for now; implemented by E10-103.
     */
    void apply_disorder_penalty();

    /**
     * @brief Apply contamination-based value penalties.
     * Stub for now; implemented by E10-104.
     */
    void apply_contamination_penalty();

    LandValueGrid m_grid;  ///< Land value grid
};

} // namespace landvalue
} // namespace sims3000

#endif // SIMS3000_LANDVALUE_LANDVALUESYSTEM_H
