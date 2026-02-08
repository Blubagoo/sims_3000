/**
 * @file ZoneGrid.h
 * @brief Sparse spatial index for zone entities.
 *
 * ZoneGrid provides O(1) coordinate-to-zone-entity lookups for spatial queries.
 * Uses row-major storage (x varies fastest within a row) for optimal cache performance.
 *
 * This is a sparse grid (most cells are INVALID_ENTITY) per CCR-004.
 * Sparse storage is justified because:
 * - Only cells with designated zones have entries
 * - Most of the map is empty (no zones)
 * - Prevents zone overlaps via simple non-zero check
 *
 * Supported map sizes:
 * - 128x128: 64KB memory budget (16,384 tiles)
 * - 256x256: 256KB memory budget (65,536 tiles)
 * - 512x512: 1MB memory budget (262,144 tiles)
 *
 * @see /docs/canon/patterns.yaml (dense_grid_exception)
 * @see /docs/epics/epic-4/tickets.md (4-006)
 */

#ifndef SIMS3000_ZONE_ZONEGRID_H
#define SIMS3000_ZONE_ZONEGRID_H

#include <cstdint>
#include <vector>
#include <cassert>

namespace sims3000 {
namespace zone {

/// Invalid entity ID (no zone present)
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
 * @class ZoneGrid
 * @brief Sparse 2D array storing EntityID per tile for zone spatial queries.
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
class ZoneGrid {
public:
    /**
     * @brief Default constructor creates an empty grid.
     *
     * Call initialize() to allocate storage for a specific map size.
     */
    ZoneGrid()
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
    ZoneGrid(std::uint16_t width, std::uint16_t height)
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
     * All cells are initialized to INVALID_ENTITY (no zone).
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
     * @brief Place a zone at (x, y).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param entity_id EntityID to place.
     * @return true if placement succeeded, false if out of bounds or cell already occupied.
     */
    bool place_zone(std::int32_t x, std::int32_t y, std::uint32_t entity_id) {
        if (!in_bounds(x, y)) return false;
        std::size_t index = static_cast<std::size_t>(y) * m_width + static_cast<std::size_t>(x);
        if (m_cells[index] != INVALID_ENTITY) return false; // Cell already occupied
        m_cells[index] = entity_id;
        return true;
    }

    /**
     * @brief Remove a zone at (x, y).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if removal succeeded, false if out of bounds or no zone present.
     */
    bool remove_zone(std::int32_t x, std::int32_t y) {
        if (!in_bounds(x, y)) return false;
        std::size_t index = static_cast<std::size_t>(y) * m_width + static_cast<std::size_t>(x);
        if (m_cells[index] == INVALID_ENTITY) return false; // No zone to remove
        m_cells[index] = INVALID_ENTITY;
        return true;
    }

    /**
     * @brief Get zone entity ID at (x, y).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return EntityID of zone at position, or INVALID_ENTITY if none.
     *
     * @note Returns INVALID_ENTITY for out-of-bounds coordinates.
     */
    std::uint32_t get_zone_at(std::int32_t x, std::int32_t y) const {
        if (!in_bounds(x, y)) return INVALID_ENTITY;
        return m_cells[static_cast<std::size_t>(y) * m_width + static_cast<std::size_t>(x)];
    }

    /**
     * @brief Check if there is a zone at (x, y).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if cell contains a valid EntityID (not INVALID_ENTITY).
     */
    bool has_zone_at(std::int32_t x, std::int32_t y) const {
        return get_zone_at(x, y) != INVALID_ENTITY;
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
    std::vector<std::uint32_t> m_cells; ///< Sparse cell storage (row-major)
};

} // namespace zone
} // namespace sims3000

#endif // SIMS3000_ZONE_ZONEGRID_H
