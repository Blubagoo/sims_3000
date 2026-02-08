/**
 * @file CoverageGrid.h
 * @brief Dense 2D array for tracking energy/utility coverage per tile.
 *
 * CoverageGrid provides O(1) coordinate-to-coverage access for energy
 * systems. Uses row-major storage (x varies fastest within a row) with
 * 1 byte per cell. Each cell stores the overseer_id (1-4) of the
 * entity providing coverage, or 0 if the cell is uncovered.
 *
 * Supported map sizes:
 * - 128x128: 16KB memory budget (16,384 cells)
 * - 256x256: 64KB memory budget (65,536 cells)
 * - 512x512: 256KB memory budget (262,144 cells)
 *
 * This is a canonical exception to the ECS-everywhere principle.
 * Dense grids preserve ECS separation of concerns:
 * - Data: Pure coverage values (uint8_t per cell)
 * - Logic: Stateless system operations
 * - Identity: Grid coordinates serve as implicit entity IDs
 *
 * @see /docs/canon/patterns.yaml (dense_grid_exception)
 */

#pragma once

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace energy {

/**
 * @class CoverageGrid
 * @brief Dense 2D array storing coverage ownership for all tiles.
 *
 * Row-major layout: index = y * width + x
 *
 * Cell values:
 * - 0: uncovered (no overseer)
 * - 1-4: overseer_id providing coverage
 *
 * Memory budget at 1 byte per cell:
 * - 128x128: 16,384 bytes (16KB)
 * - 256x256: 65,536 bytes (64KB)
 * - 512x512: 262,144 bytes (256KB)
 *
 * All public methods perform bounds checking. Out-of-bounds calls
 * return safe defaults (false/0) without crashing.
 */
class CoverageGrid {
public:
    /**
     * @brief Construct a coverage grid with the specified dimensions.
     *
     * All cells are initialized to 0 (uncovered).
     *
     * @param width Grid width in tiles.
     * @param height Grid height in tiles.
     */
    CoverageGrid(uint32_t width, uint32_t height);

    /**
     * @brief Check if a cell is covered by a specific owner.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner The overseer_id to check for (1-4).
     * @return true if the cell is covered by the specified owner.
     *         Returns false for out-of-bounds coordinates.
     */
    bool is_in_coverage(uint32_t x, uint32_t y, uint8_t owner) const;

    /**
     * @brief Get the overseer_id that covers a cell.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return The overseer_id (1-4) or 0 if uncovered.
     *         Returns 0 for out-of-bounds coordinates.
     */
    uint8_t get_coverage_owner(uint32_t x, uint32_t y) const;

    /**
     * @brief Mark a cell as covered by a specific owner.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner The overseer_id providing coverage (1-4).
     *
     * @note No-op for out-of-bounds coordinates.
     */
    void set(uint32_t x, uint32_t y, uint8_t owner);

    /**
     * @brief Mark a cell as uncovered (set to 0).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     *
     * @note No-op for out-of-bounds coordinates.
     */
    void clear(uint32_t x, uint32_t y);

    /**
     * @brief Clear all cells owned by a specific overseer.
     *
     * Iterates the entire grid and sets any cell matching the
     * given owner back to 0 (uncovered).
     *
     * @param owner The overseer_id to clear (1-4).
     */
    void clear_all_for_owner(uint8_t owner);

    /**
     * @brief Reset the entire grid to uncovered (all cells = 0).
     */
    void clear_all();

    /**
     * @brief Get grid width in tiles.
     * @return Width of the grid.
     */
    uint32_t get_width() const;

    /**
     * @brief Get grid height in tiles.
     * @return Height of the grid.
     */
    uint32_t get_height() const;

    /**
     * @brief Check if coordinates are within grid bounds.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if (x, y) is within [0, width) x [0, height).
     */
    bool is_valid(uint32_t x, uint32_t y) const;

    /**
     * @brief Count the number of cells covered by a specific owner.
     *
     * Iterates the entire grid to count cells matching the owner.
     *
     * @param owner The overseer_id to count (1-4).
     * @return Number of cells covered by the specified owner.
     */
    uint32_t get_coverage_count(uint8_t owner) const;

private:
    /**
     * @brief Calculate the linear index for a coordinate pair.
     *
     * Row-major: index = y * width + x
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Linear index into the m_data vector.
     *
     * @note Does NOT perform bounds checking.
     */
    uint32_t index(uint32_t x, uint32_t y) const;

    uint32_t m_width;                ///< Grid width in tiles
    uint32_t m_height;               ///< Grid height in tiles
    std::vector<uint8_t> m_data;     ///< Dense cell storage (row-major), 1 byte per cell
};

} // namespace energy
} // namespace sims3000
