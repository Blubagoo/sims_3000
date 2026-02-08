/**
 * @file PathwayGrid.h
 * @brief Dense 2D array for tracking pathway placement per tile (Ticket E7-005)
 *
 * PathwayGrid provides O(1) spatial queries for transport pathway placement.
 * Uses row-major storage (x varies fastest within a row) with 4 bytes per cell.
 * Each cell stores the entity ID of the pathway occupying that cell, or 0 if empty.
 *
 * Includes a network dirty flag for triggering network graph rebuilds
 * when pathways are added or removed.
 *
 * Memory budget at 4 bytes/cell (uint32_t entity_id):
 * - 128x128:   64KB  (16,384 cells)
 * - 256x256:  256KB  (65,536 cells)
 * - 512x512: 1024KB  (262,144 cells)
 *
 * This is a canonical exception to the ECS-everywhere principle.
 * Dense grids preserve ECS separation of concerns:
 * - Data: Pure entity ID values (uint32_t per cell)
 * - Logic: Stateless system operations
 * - Identity: Grid coordinates serve as implicit entity IDs
 *
 * @see /docs/canon/patterns.yaml (dense_grid_exception)
 */

#pragma once

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace transport {

/**
 * @struct PathwayGridCell
 * @brief Single cell in the pathway grid (4 bytes).
 *
 * Stores the entity ID of the pathway occupying this cell.
 * A value of 0 indicates the cell is empty (no pathway).
 */
struct PathwayGridCell {
    uint32_t entity_id = 0;  ///< 0 = empty, non-zero = pathway entity ID
};

/**
 * @class PathwayGrid
 * @brief Dense 2D array storing pathway entity IDs for all tiles.
 *
 * Row-major layout: index = y * width + x
 *
 * Cell values:
 * - 0: empty (no pathway)
 * - non-zero: entity ID of pathway occupying this cell
 *
 * All public methods perform bounds checking. Out-of-bounds calls
 * return safe defaults (0/false) without crashing.
 */
class PathwayGrid {
public:
    /**
     * @brief Default constructor. Creates an empty 0x0 grid.
     */
    PathwayGrid() = default;

    /**
     * @brief Construct a pathway grid with the specified dimensions.
     *
     * All cells are initialized to 0 (empty).
     *
     * @param width Grid width in tiles.
     * @param height Grid height in tiles.
     */
    PathwayGrid(uint32_t width, uint32_t height);

    // ========================================================================
    // Core operations
    // ========================================================================

    /**
     * @brief Get the pathway entity ID at a cell.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return The pathway entity ID, or 0 if empty/out-of-bounds.
     */
    uint32_t get_pathway_at(int32_t x, int32_t y) const;

    /**
     * @brief Check if a cell contains a pathway.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if the cell contains a pathway (entity_id != 0).
     *         Returns false for out-of-bounds coordinates.
     */
    bool has_pathway(int32_t x, int32_t y) const;

    /**
     * @brief Place a pathway entity at a cell.
     *
     * Sets the cell's entity ID and marks the network as dirty.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param entity_id The pathway entity ID to place.
     *
     * @note No-op for out-of-bounds coordinates.
     */
    void set_pathway(int32_t x, int32_t y, uint32_t entity_id);

    /**
     * @brief Remove a pathway from a cell (set to 0).
     *
     * Clears the cell's entity ID and marks the network as dirty.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     *
     * @note No-op for out-of-bounds coordinates.
     */
    void clear_pathway(int32_t x, int32_t y);

    // ========================================================================
    // Dirty tracking for network rebuild
    // ========================================================================

    /**
     * @brief Check if the network graph needs rebuilding.
     *
     * The network is marked dirty when pathways are added or removed.
     * Systems should check this flag and rebuild the network graph
     * when it returns true, then call mark_network_clean().
     *
     * @return true if the network needs rebuilding.
     */
    bool is_network_dirty() const;

    /**
     * @brief Manually mark the network as dirty.
     *
     * Useful when external changes require a network rebuild
     * (e.g., pathway property changes that affect connectivity).
     */
    void mark_network_dirty();

    /**
     * @brief Mark the network as clean after a successful rebuild.
     *
     * Should be called by the network rebuild system after it has
     * processed all pending changes.
     */
    void mark_network_clean();

    // ========================================================================
    // Dimensions
    // ========================================================================

    /**
     * @brief Get grid width in tiles.
     * @return Width of the grid.
     */
    uint32_t width() const;

    /**
     * @brief Get grid height in tiles.
     * @return Height of the grid.
     */
    uint32_t height() const;

    // ========================================================================
    // Bounds check
    // ========================================================================

    /**
     * @brief Check if coordinates are within grid bounds.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if (x, y) is within [0, width) x [0, height).
     */
    bool in_bounds(int32_t x, int32_t y) const;

private:
    /**
     * @brief Calculate the linear index for a coordinate pair.
     *
     * Row-major: index = y * width + x
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Linear index into the grid_ vector.
     *
     * @note Does NOT perform bounds checking.
     */
    uint32_t index(int32_t x, int32_t y) const;

    std::vector<PathwayGridCell> grid_;      ///< Dense cell storage (row-major), 4 bytes per cell
    uint32_t width_ = 0;                     ///< Grid width in tiles
    uint32_t height_ = 0;                    ///< Grid height in tiles
    bool network_dirty_ = true;              ///< True if network graph needs rebuilding
};

} // namespace transport
} // namespace sims3000
