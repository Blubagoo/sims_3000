/**
 * @file SubterraLayerManager.h
 * @brief Subterra layer grid manager for Epic 7 (Ticket E7-042)
 *
 * Manages a separate grid layer for underground (subterra) infrastructure.
 * Each cell stores an entity_id (0 = empty). Provides placement validation
 * (bounds check + not occupied) for MVP single-depth underground layer.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace transport {

/**
 * @class SubterraLayerManager
 * @brief Grid manager for underground transport layer.
 *
 * Maintains a flat 2D grid of entity IDs representing underground
 * infrastructure. Entity ID 0 means the cell is empty.
 * Single depth level (depth_level=1) for MVP.
 */
class SubterraLayerManager {
public:
    SubterraLayerManager() = default;

    /**
     * @brief Construct a subterra layer grid with the given dimensions.
     * @param width Grid width in cells.
     * @param height Grid height in cells.
     */
    SubterraLayerManager(uint32_t width, uint32_t height);

    /**
     * @brief Get the entity ID at the given cell.
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @return Entity ID at (x, y), or 0 if out of bounds.
     */
    uint32_t get_subterra_at(int32_t x, int32_t y) const;

    /**
     * @brief Check if a cell has subterra infrastructure.
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @return true if the cell has a non-zero entity ID.
     */
    bool has_subterra(int32_t x, int32_t y) const;

    /**
     * @brief Place a subterra entity at the given cell.
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @param entity_id The entity ID to place (must be non-zero).
     */
    void set_subterra(int32_t x, int32_t y, uint32_t entity_id);

    /**
     * @brief Remove subterra infrastructure from the given cell.
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     */
    void clear_subterra(int32_t x, int32_t y);

    /**
     * @brief Check if subterra infrastructure can be built at the given cell.
     *
     * Validates bounds and checks that the cell is not already occupied.
     * Full terrain validation is deferred for MVP.
     *
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @return true if the cell is in bounds and not occupied.
     */
    bool can_build_subterra_at(int32_t x, int32_t y) const;

    /**
     * @brief Enhanced placement validation with adjacency requirement (E7-044).
     *
     * Validates:
     * 1. Position is in bounds (negative coordinates rejected)
     * 2. Position is not already occupied
     * 3. If require_adjacent is true, at least one N/S/E/W neighbor
     *    must have a subterra entity (OR the grid is completely empty,
     *    allowing the first placement)
     *
     * Terrain validation (water/elevation) is deferred.
     *
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @param require_adjacent If true, requires adjacent subterra tiles
     *                         (unless grid is empty for first placement).
     * @return true if placement is valid.
     */
    bool can_build_subterra_at(int32_t x, int32_t y, bool require_adjacent) const;

    /**
     * @brief Get grid width.
     * @return Width in cells.
     */
    uint32_t width() const;

    /**
     * @brief Get grid height.
     * @return Height in cells.
     */
    uint32_t height() const;

    /**
     * @brief Check if coordinates are within grid bounds.
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @return true if (x, y) is within [0, width) x [0, height).
     */
    bool in_bounds(int32_t x, int32_t y) const;

private:
    /// Check if any subterra entity exists adjacent (N/S/E/W) to position
    bool has_adjacent_subterra(int32_t x, int32_t y) const;

    /// Check if the entire grid is empty (no subterra entities at all)
    bool is_grid_empty() const;

    std::vector<uint32_t> subterra_grid_;  ///< Entity ID per cell, 0 = empty
    uint32_t width_ = 0;                   ///< Grid width
    uint32_t height_ = 0;                  ///< Grid height
};

} // namespace transport
} // namespace sims3000
