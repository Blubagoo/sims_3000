/**
 * @file BuildingGrid.h
 * @brief Dense 2D array storage for building occupancy data.
 *
 * BuildingGrid provides O(1) coordinate-to-building-entity lookups for spatial queries.
 * Uses row-major storage (x varies fastest within a row) for optimal cache performance.
 *
 * This is a dense grid exception (like TerrainGrid) per CCR-004 and canonical patterns.
 * Dense storage is justified because:
 * - Every tile potentially has a building
 * - Spatial lookups must be O(1)
 * - Per-entity overhead is prohibitive at scale (24+ bytes vs 4 bytes per tile)
 *
 * Supports multi-tile footprint registration (a 2x2 building marks all 4 cells).
 *
 * Supported map sizes:
 * - 128x128: 64KB memory budget (16,384 tiles)
 * - 256x256: 256KB memory budget (65,536 tiles)
 * - 512x512: 1MB memory budget (262,144 tiles)
 *
 * @see /docs/canon/patterns.yaml (dense_grid_exception)
 * @see /docs/epics/epic-4/tickets.md (4-007)
 */

#ifndef SIMS3000_BUILDING_BUILDINGGRID_H
#define SIMS3000_BUILDING_BUILDINGGRID_H

#include <cstdint>
#include <vector>
#include <cassert>

namespace sims3000 {
namespace building {

/// Invalid entity ID (no building present)
constexpr std::uint32_t INVALID_ENTITY = 0;

/**
 * @brief Check if a dimension value is a valid map size.
 * @param dimension The width or height to check.
 * @return true if dimension is 128, 256, or 512.
 */
constexpr bool isValidMapSize(std::uint16_t dimension) {
    return dimension == 128 || dimension == 256 || dimension == 512;
}

/**
 * @class BuildingGrid
 * @brief Dense 2D array storing EntityID per tile for building occupancy.
 *
 * Row-major layout: index = y * width + x
 * This layout is optimal for:
 * - Row-by-row spatial queries
 * - Horizontal scanline operations
 * - Cache-friendly iteration patterns
 *
 * Memory budget at 4 bytes per tile:
 * - 128x128: 16,384 tiles * 4 bytes = 65,536 bytes (64KB)
 * - 256x256: 65,536 tiles * 4 bytes = 262,144 bytes (256KB)
 * - 512x512: 262,144 tiles * 4 bytes = 1,048,576 bytes (1MB)
 */
class BuildingGrid {
public:
    /**
     * @brief Default constructor creates an empty grid.
     *
     * Call initialize() to allocate storage for a specific map size.
     */
    BuildingGrid()
        : m_width(0)
        , m_height(0)
        , m_cells()
    {}

    /**
     * @brief Construct a grid with explicit dimensions.
     *
     * @param width Grid width (must be 128, 256, or 512).
     * @param height Grid height (must be 128, 256, or 512).
     *
     * @note Width must equal height (square maps only).
     *       In debug builds, asserts if dimensions are invalid.
     */
    BuildingGrid(std::uint16_t width, std::uint16_t height)
        : m_width(width)
        , m_height(height)
        , m_cells()
    {
        assert(isValidMapSize(width) && "Width must be 128, 256, or 512");
        assert(isValidMapSize(height) && "Height must be 128, 256, or 512");
        assert(width == height && "Maps must be square");
        m_cells.resize(static_cast<std::size_t>(width) * height, INVALID_ENTITY);
    }

    /**
     * @brief Initialize or reinitialize the grid to a specific size.
     *
     * Clears any existing data and allocates fresh storage.
     * All cells are initialized to INVALID_ENTITY (no building).
     *
     * @param width Grid width (must be 128, 256, or 512).
     * @param height Grid height (must be 128, 256, or 512).
     */
    void initialize(std::uint16_t width, std::uint16_t height) {
        assert(isValidMapSize(width) && "Width must be 128, 256, or 512");
        assert(isValidMapSize(height) && "Height must be 128, 256, or 512");
        assert(width == height && "Maps must be square");
        m_width = width;
        m_height = height;
        m_cells.clear();
        m_cells.resize(static_cast<std::size_t>(width) * height, INVALID_ENTITY);
    }

    /**
     * @brief Get grid width in tiles.
     * @return Grid width.
     */
    std::uint16_t getWidth() const { return m_width; }

    /**
     * @brief Get grid height in tiles.
     * @return Grid height.
     */
    std::uint16_t getHeight() const { return m_height; }

    /**
     * @brief Check if coordinates are within grid bounds.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if (x, y) is within [0, width) x [0, height).
     */
    bool in_bounds(std::int32_t x, std::int32_t y) const {
        return x >= 0 && x < static_cast<std::int32_t>(m_width) &&
               y >= 0 && y < static_cast<std::int32_t>(m_height);
    }

    /**
     * @brief Get building entity ID at (x, y).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return EntityID of building at position, or INVALID_ENTITY if none.
     *
     * @note Returns INVALID_ENTITY for out-of-bounds coordinates.
     */
    std::uint32_t get_building_at(std::int32_t x, std::int32_t y) const {
        if (!in_bounds(x, y)) return INVALID_ENTITY;
        return m_cells[static_cast<std::size_t>(y) * m_width + static_cast<std::size_t>(x)];
    }

    /**
     * @brief Set building entity ID at (x, y).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param entity_id EntityID to set.
     *
     * @note Does nothing if coordinates are out of bounds.
     */
    void set_building_at(std::int32_t x, std::int32_t y, std::uint32_t entity_id) {
        if (!in_bounds(x, y)) return;
        m_cells[static_cast<std::size_t>(y) * m_width + static_cast<std::size_t>(x)] = entity_id;
    }

    /**
     * @brief Clear building entity ID at (x, y).
     *
     * Sets the cell to INVALID_ENTITY.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     *
     * @note Does nothing if coordinates are out of bounds.
     */
    void clear_building_at(std::int32_t x, std::int32_t y) {
        set_building_at(x, y, INVALID_ENTITY);
    }

    /**
     * @brief Check if tile at (x, y) is occupied by a building.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if cell contains a valid EntityID (not INVALID_ENTITY).
     */
    bool is_tile_occupied(std::int32_t x, std::int32_t y) const {
        return get_building_at(x, y) != INVALID_ENTITY;
    }

    /**
     * @brief Check if a rectangular footprint is available (all cells empty).
     *
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param w Width in tiles.
     * @param h Height in tiles.
     * @return true if all cells in footprint are INVALID_ENTITY and in bounds.
     */
    bool is_footprint_available(std::int32_t x, std::int32_t y, std::uint8_t w, std::uint8_t h) const {
        for (std::int32_t dy = 0; dy < h; ++dy) {
            for (std::int32_t dx = 0; dx < w; ++dx) {
                if (!in_bounds(x + dx, y + dy)) return false;
                if (is_tile_occupied(x + dx, y + dy)) return false;
            }
        }
        return true;
    }

    /**
     * @brief Register a building across a rectangular footprint.
     *
     * Sets all cells within the footprint to the given EntityID.
     * Used for multi-tile buildings (e.g., 2x2, 3x3).
     *
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param w Width in tiles.
     * @param h Height in tiles.
     * @param entity_id EntityID to register.
     *
     * @note Skips out-of-bounds cells. In debug builds, asserts if any cell
     *       in the footprint is already occupied (caller should check first).
     */
    void set_footprint(std::int32_t x, std::int32_t y, std::uint8_t w, std::uint8_t h, std::uint32_t entity_id) {
        for (std::int32_t dy = 0; dy < h; ++dy) {
            for (std::int32_t dx = 0; dx < w; ++dx) {
                std::int32_t cx = x + dx;
                std::int32_t cy = y + dy;
                if (!in_bounds(cx, cy)) continue;
                assert(!is_tile_occupied(cx, cy) && "Cell already occupied during set_footprint");
                set_building_at(cx, cy, entity_id);
            }
        }
    }

    /**
     * @brief Clear a rectangular footprint.
     *
     * Sets all cells within the footprint to INVALID_ENTITY.
     * Used when demolishing multi-tile buildings.
     *
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param w Width in tiles.
     * @param h Height in tiles.
     *
     * @note Skips out-of-bounds cells.
     */
    void clear_footprint(std::int32_t x, std::int32_t y, std::uint8_t w, std::uint8_t h) {
        for (std::int32_t dy = 0; dy < h; ++dy) {
            for (std::int32_t dx = 0; dx < w; ++dx) {
                std::int32_t cx = x + dx;
                std::int32_t cy = y + dy;
                if (!in_bounds(cx, cy)) continue;
                clear_building_at(cx, cy);
            }
        }
    }

    /**
     * @brief Get total number of cells in the grid.
     *
     * @return width * height
     */
    std::size_t cell_count() const {
        return static_cast<std::size_t>(m_width) * m_height;
    }

    /**
     * @brief Get memory size in bytes used by cell storage.
     *
     * @return Number of bytes used by the cells vector.
     */
    std::size_t memory_bytes() const {
        return m_cells.size() * sizeof(std::uint32_t);
    }

    /**
     * @brief Check if the grid is empty (uninitialized).
     *
     * @return true if width or height is 0.
     */
    bool empty() const {
        return m_width == 0 || m_height == 0 || m_cells.empty();
    }

    /**
     * @brief Clear all cells (set all to INVALID_ENTITY).
     */
    void clear_all() {
        std::fill(m_cells.begin(), m_cells.end(), INVALID_ENTITY);
    }

private:
    std::uint16_t m_width;              ///< Grid width in tiles
    std::uint16_t m_height;             ///< Grid height in tiles
    std::vector<std::uint32_t> m_cells; ///< Dense cell storage (row-major)
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGGRID_H
