/**
 * @file LandValueGrid.h
 * @brief Dense 2D grid for tracking land value per tile.
 *
 * LandValueGrid does NOT need double-buffering because it reads from
 * other grids' previous buffers (disorder, contamination) which are
 * already double-buffered.
 *
 * Each cell stores a total value (0-255, 128 = neutral) and a cached
 * terrain bonus.
 *
 * Memory budget: 2 bytes/cell (no double buffer).
 * - 128x128: ~32KB
 * - 256x256: ~128KB
 * - 512x512: ~512KB
 *
 * All public methods perform bounds checking. Out-of-bounds reads return 0,
 * out-of-bounds writes are no-ops.
 *
 * @see E10-062
 */

#ifndef SIMS3000_LANDVALUE_LANDVALUEGRID_H
#define SIMS3000_LANDVALUE_LANDVALUEGRID_H

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace landvalue {

/**
 * @struct LandValueCell
 * @brief Single cell in the land value grid.
 */
struct LandValueCell {
    uint8_t total_value = 128;  ///< 0-255, 128 = neutral
    uint8_t terrain_bonus = 0;  ///< Terrain contribution cached
};

static_assert(sizeof(LandValueCell) == 2, "LandValueCell must be 2 bytes");

/**
 * @class LandValueGrid
 * @brief Dense 2D grid storing land value data for all tiles.
 *
 * Row-major layout: index = y * width + x
 *
 * Default value for all cells is 128 (neutral). The LandValueSystem
 * is responsible for recalculating values each tick by reading from
 * other grids' previous tick buffers.
 */
class LandValueGrid {
public:
    /**
     * @brief Construct a land value grid with the specified dimensions.
     *
     * All cells are initialized to total_value=128 (neutral), terrain_bonus=0.
     *
     * @param width Grid width in tiles.
     * @param height Grid height in tiles.
     */
    LandValueGrid(uint16_t width, uint16_t height);

    /** @brief Get grid width in tiles. */
    uint16_t get_width() const;

    /** @brief Get grid height in tiles. */
    uint16_t get_height() const;

    /**
     * @brief Get the total land value for a cell.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Land value 0-255. Returns 0 for out-of-bounds.
     */
    uint8_t get_value(int32_t x, int32_t y) const;

    /**
     * @brief Get the cached terrain bonus for a cell.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Terrain bonus 0-255. Returns 0 for out-of-bounds.
     */
    uint8_t get_terrain_bonus(int32_t x, int32_t y) const;

    /**
     * @brief Set the total land value for a cell.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param value Land value 0-255.
     * @note No-op for out-of-bounds coordinates.
     */
    void set_value(int32_t x, int32_t y, uint8_t value);

    /**
     * @brief Subtract from land value with saturating arithmetic.
     *
     * The result is clamped to 0 (no wrap-around).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param amount Amount to subtract.
     * @note No-op for out-of-bounds coordinates.
     */
    void subtract_value(int32_t x, int32_t y, uint8_t amount);

    /**
     * @brief Set the cached terrain bonus for a cell.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param bonus Terrain bonus 0-255.
     * @note No-op for out-of-bounds coordinates.
     */
    void set_terrain_bonus(int32_t x, int32_t y, uint8_t bonus);

    /**
     * @brief Reset all total_value fields to 128 (neutral) for recalculation.
     *
     * Called by LandValueSystem at the start of each recalculation pass.
     * Does NOT reset terrain_bonus values (those are cached separately).
     */
    void reset_values();

    /**
     * @brief Get raw pointer to value data (for overlay rendering).
     *
     * Note: Since cells are stored as LandValueCell structs, this extracts
     * total_value fields into a separate contiguous buffer for overlay rendering.
     *
     * @return Pointer to contiguous uint8_t value data.
     */
    const uint8_t* get_value_data() const;

    /**
     * @brief Reset all cells to defaults (total_value=128, terrain_bonus=0).
     */
    void clear();

    /**
     * @brief Check if coordinates are within grid bounds.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if (x, y) is within [0, width) x [0, height).
     */
    bool is_valid(int32_t x, int32_t y) const;

private:
    uint16_t m_width;                          ///< Grid width in tiles
    uint16_t m_height;                         ///< Grid height in tiles
    std::vector<LandValueCell> m_grid;         ///< Dense cell storage

    mutable std::vector<uint8_t> m_value_cache;///< Cache for get_value_data()
    mutable bool m_value_cache_dirty = true;   ///< Whether cache needs rebuild

    /**
     * @brief Calculate the linear index for a coordinate pair.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Linear index into the grid vector.
     * @note Does NOT perform bounds checking.
     */
    size_t index(int32_t x, int32_t y) const;
};

} // namespace landvalue
} // namespace sims3000

#endif // SIMS3000_LANDVALUE_LANDVALUEGRID_H
