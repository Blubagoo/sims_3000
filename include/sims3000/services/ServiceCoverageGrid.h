/**
 * @file ServiceCoverageGrid.h
 * @brief Dense 2D array for tracking radius-based service coverage per tile.
 *
 * ServiceCoverageGrid provides O(1) coordinate-to-coverage access for service
 * systems. Uses row-major storage (x varies fastest within a row) with
 * 1 byte per cell. Each cell stores a coverage value (0-255) representing
 * the cumulative service coverage at that tile.
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
 * @see FluidCoverageGrid.h for the reference pattern
 */

#pragma once

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace services {

/**
 * @class ServiceCoverageGrid
 * @brief Dense 2D array storing service coverage values for all tiles.
 *
 * Row-major layout: index = y * width + x
 *
 * Cell values:
 * - 0: no coverage
 * - 1-255: coverage intensity (higher = better coverage)
 *
 * Memory budget at 1 byte per cell:
 * - 128x128: 16,384 bytes (16KB)
 * - 256x256: 65,536 bytes (64KB)
 * - 512x512: 262,144 bytes (256KB)
 *
 * All public methods perform bounds checking. Out-of-bounds calls
 * return safe defaults (0/0.0f) without crashing.
 */
class ServiceCoverageGrid {
public:
    /**
     * @brief Construct a service coverage grid with the specified dimensions.
     *
     * All cells are initialized to 0 (no coverage).
     *
     * @param width Grid width in tiles.
     * @param height Grid height in tiles.
     */
    ServiceCoverageGrid(uint32_t width, uint32_t height);

    /**
     * @brief Get the coverage value at a cell.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Coverage value (0-255).
     *         Returns 0 for out-of-bounds coordinates.
     */
    uint8_t get_coverage_at(uint32_t x, uint32_t y) const;

    /**
     * @brief Get the normalized coverage value at a cell.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Normalized coverage value (0.0-1.0).
     *         Returns 0.0f for out-of-bounds coordinates.
     */
    float get_coverage_at_normalized(uint32_t x, uint32_t y) const;

    /**
     * @brief Set the coverage value at a cell.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param value Coverage value (0-255).
     *
     * @note No-op for out-of-bounds coordinates.
     */
    void set_coverage_at(uint32_t x, uint32_t y, uint8_t value);

    /**
     * @brief Reset the entire grid to 0 (no coverage).
     */
    void clear();

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

} // namespace services
} // namespace sims3000
