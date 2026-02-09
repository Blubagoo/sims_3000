/**
 * @file ContaminationGrid.h
 * @brief Dense 2D grid for tracking contamination levels and types per tile
 *        with double-buffering.
 *
 * ContaminationGrid uses double-buffered storage for circular dependency
 * resolution with LandValue. Each cell stores a contamination level (0-255)
 * and a dominant contamination type (uint8_t).
 *
 * Memory budget: 2 bytes/cell * 2 buffers = 4 bytes/cell.
 * - 128x128: ~64KB
 * - 256x256: ~256KB
 * - 512x512: ~1MB
 *
 * All public methods perform bounds checking. Out-of-bounds reads return 0,
 * out-of-bounds writes are no-ops.
 *
 * @see E10-061
 */

#ifndef SIMS3000_CONTAMINATION_CONTAMINATIONGRID_H
#define SIMS3000_CONTAMINATION_CONTAMINATIONGRID_H

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace contamination {

/**
 * @struct ContaminationCell
 * @brief Single cell in the contamination grid.
 */
struct ContaminationCell {
    uint8_t level = 0;         ///< 0-255 contamination level
    uint8_t dominant_type = 0; ///< ContaminationType as uint8_t
};

static_assert(sizeof(ContaminationCell) == 2, "ContaminationCell must be 2 bytes");

/**
 * @class ContaminationGrid
 * @brief Dense 2D double-buffered grid storing contamination data for all tiles.
 *
 * Row-major layout: index = y * width + x
 *
 * Double-buffering protocol:
 * 1. At the start of each tick, call swap_buffers()
 * 2. Systems read from get_level_previous_tick() / get_dominant_type_previous_tick()
 * 3. Systems write to set_level() / add_contamination() / apply_decay()
 */
class ContaminationGrid {
public:
    /**
     * @brief Construct a contamination grid with the specified dimensions.
     *
     * Both buffers are initialized to 0 (no contamination).
     *
     * @param width Grid width in tiles.
     * @param height Grid height in tiles.
     */
    ContaminationGrid(uint16_t width, uint16_t height);

    /** @brief Get grid width in tiles. */
    uint16_t get_width() const;

    /** @brief Get grid height in tiles. */
    uint16_t get_height() const;

    /**
     * @brief Get the contamination level for a cell in the current tick buffer.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Contamination level 0-255. Returns 0 for out-of-bounds.
     */
    uint8_t get_level(int32_t x, int32_t y) const;

    /**
     * @brief Get the dominant contamination type for a cell in the current buffer.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Dominant type as uint8_t. Returns 0 for out-of-bounds.
     */
    uint8_t get_dominant_type(int32_t x, int32_t y) const;

    /**
     * @brief Set the contamination level for a cell in the current tick buffer.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param level Contamination level 0-255.
     * @note No-op for out-of-bounds coordinates.
     */
    void set_level(int32_t x, int32_t y, uint8_t level);

    /**
     * @brief Add contamination to a cell with saturating arithmetic.
     *
     * The level is clamped to 255 (no wrap-around). If the added amount
     * is greater than the existing level contribution of the current
     * dominant type, the dominant type is updated to the new type.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param amount Amount to add.
     * @param type Contamination type contributing this amount.
     * @note No-op for out-of-bounds coordinates.
     */
    void add_contamination(int32_t x, int32_t y, uint8_t amount, uint8_t type);

    /**
     * @brief Subtract contamination from a cell with saturating arithmetic.
     *
     * The level is clamped to 0 (no wrap-around). If the level reaches 0,
     * the dominant type is reset to 0.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param amount Amount to subtract.
     * @note No-op for out-of-bounds coordinates.
     */
    void apply_decay(int32_t x, int32_t y, uint8_t amount);

    /**
     * @brief Get the contamination level from the previous tick buffer.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Contamination level 0-255. Returns 0 for out-of-bounds.
     */
    uint8_t get_level_previous_tick(int32_t x, int32_t y) const;

    /**
     * @brief Get the dominant type from the previous tick buffer.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Dominant type as uint8_t. Returns 0 for out-of-bounds.
     */
    uint8_t get_dominant_type_previous_tick(int32_t x, int32_t y) const;

    /**
     * @brief Swap the current and previous buffers.
     *
     * Call this at the start of each simulation tick. Uses std::swap
     * on the underlying vectors for O(1) pointer swap.
     */
    void swap_buffers();

    /**
     * @brief Get the sum of all contamination levels across the grid.
     *
     * Returns the cached value from the last update_stats() call.
     *
     * @return Total contamination across all cells.
     */
    uint32_t get_total_contamination() const;

    /**
     * @brief Get the count of tiles with contamination above a threshold.
     *
     * Returns the cached value from the last update_stats() call.
     *
     * @param threshold Minimum contamination level to count (default 128).
     * @return Number of tiles at or above the threshold.
     */
    uint32_t get_toxic_tiles(uint8_t threshold = 128) const;

    /**
     * @brief Recalculate cached aggregate statistics.
     *
     * Updates total_contamination and toxic_tiles from current buffer.
     */
    void update_stats();

    /**
     * @brief Get raw pointer to level data from current buffer (for overlays).
     *
     * Note: Since cells are stored as ContaminationCell structs, this extracts
     * levels into a separate contiguous buffer for overlay rendering.
     *
     * @return Pointer to contiguous uint8_t level data.
     */
    const uint8_t* get_level_data() const;

    /**
     * @brief Reset both buffers to zero.
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
    uint16_t m_width;                                ///< Grid width in tiles
    uint16_t m_height;                               ///< Grid height in tiles
    std::vector<ContaminationCell> m_grid;            ///< Current tick buffer
    std::vector<ContaminationCell> m_previous_grid;   ///< Previous tick buffer

    uint32_t m_total_contamination = 0;              ///< Sum of all contamination levels
    uint32_t m_toxic_tiles = 0;                      ///< Count of tiles above threshold

    mutable std::vector<uint8_t> m_level_cache;      ///< Cache for get_level_data()
    mutable bool m_level_cache_dirty = true;         ///< Whether cache needs rebuild

    /**
     * @brief Calculate the linear index for a coordinate pair.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Linear index into the buffer vectors.
     * @note Does NOT perform bounds checking.
     */
    size_t index(int32_t x, int32_t y) const;
};

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_CONTAMINATIONGRID_H
