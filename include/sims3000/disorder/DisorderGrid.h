/**
 * @file DisorderGrid.h
 * @brief Dense 2D grid for tracking disorder levels per tile with double-buffering.
 *
 * DisorderGrid uses double-buffered storage for circular dependency resolution
 * with LandValue. Systems read from the previous tick's buffer while writing
 * to the current tick's buffer, avoiding read-write conflicts.
 *
 * Memory budget: 1 byte/cell * 2 buffers = 2 bytes/cell.
 * - 128x128: ~32KB
 * - 256x256: ~128KB
 * - 512x512: ~512KB
 *
 * All public methods perform bounds checking. Out-of-bounds reads return 0,
 * out-of-bounds writes are no-ops.
 *
 * @see E10-060
 */

#ifndef SIMS3000_DISORDER_DISORDERGRID_H
#define SIMS3000_DISORDER_DISORDERGRID_H

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace disorder {

/**
 * @struct DisorderCell
 * @brief Single cell in the disorder grid.
 */
struct DisorderCell {
    uint8_t level = 0; ///< 0-255 disorder level
};

static_assert(sizeof(DisorderCell) == 1, "DisorderCell must be 1 byte");

/**
 * @class DisorderGrid
 * @brief Dense 2D double-buffered grid storing disorder levels for all tiles.
 *
 * Row-major layout: index = y * width + x
 *
 * Double-buffering protocol:
 * 1. At the start of each tick, call swap_buffers()
 * 2. Systems read from get_level_previous_tick() (previous frame data)
 * 3. Systems write to set_level() / add_disorder() / apply_suppression() (current frame)
 */
class DisorderGrid {
public:
    /**
     * @brief Construct a disorder grid with the specified dimensions.
     *
     * Both buffers are initialized to 0 (no disorder).
     *
     * @param width Grid width in tiles.
     * @param height Grid height in tiles.
     */
    DisorderGrid(uint16_t width, uint16_t height);

    /** @brief Get grid width in tiles. */
    uint16_t get_width() const;

    /** @brief Get grid height in tiles. */
    uint16_t get_height() const;

    /**
     * @brief Get the disorder level for a cell in the current tick buffer.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Disorder level 0-255. Returns 0 for out-of-bounds.
     */
    uint8_t get_level(int32_t x, int32_t y) const;

    /**
     * @brief Set the disorder level for a cell in the current tick buffer.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param level Disorder level 0-255.
     * @note No-op for out-of-bounds coordinates.
     */
    void set_level(int32_t x, int32_t y, uint8_t level);

    /**
     * @brief Add disorder to a cell with saturating arithmetic.
     *
     * The result is clamped to 255 (no wrap-around).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param amount Amount to add.
     * @note No-op for out-of-bounds coordinates.
     */
    void add_disorder(int32_t x, int32_t y, uint8_t amount);

    /**
     * @brief Subtract disorder from a cell with saturating arithmetic.
     *
     * The result is clamped to 0 (no wrap-around).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param amount Amount to subtract.
     * @note No-op for out-of-bounds coordinates.
     */
    void apply_suppression(int32_t x, int32_t y, uint8_t amount);

    /**
     * @brief Get the disorder level for a cell from the previous tick buffer.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Disorder level 0-255. Returns 0 for out-of-bounds.
     */
    uint8_t get_level_previous_tick(int32_t x, int32_t y) const;

    /**
     * @brief Swap the current and previous buffers.
     *
     * Call this at the start of each simulation tick. Uses std::swap
     * on the underlying vectors for O(1) pointer swap.
     */
    void swap_buffers();

    /**
     * @brief Get the sum of all disorder levels across the grid.
     *
     * Returns the cached value from the last update_stats() call.
     *
     * @return Total disorder across all cells.
     */
    uint32_t get_total_disorder() const;

    /**
     * @brief Get the count of tiles with disorder above a threshold.
     *
     * Returns the cached value from the last update_stats() call.
     *
     * @param threshold Minimum disorder level to count (default 128).
     * @return Number of tiles at or above the threshold.
     */
    uint32_t get_high_disorder_tiles(uint8_t threshold = 128) const;

    /**
     * @brief Recalculate cached aggregate statistics.
     *
     * Updates total_disorder and high_disorder_tiles from current buffer.
     */
    void update_stats();

    /**
     * @brief Get raw pointer to current buffer data (for overlay rendering).
     * @return Pointer to the first element of the current buffer.
     */
    const uint8_t* get_raw_data() const;

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
    uint16_t m_width;                    ///< Grid width in tiles
    uint16_t m_height;                   ///< Grid height in tiles
    std::vector<uint8_t> m_grid;         ///< Current tick buffer
    std::vector<uint8_t> m_previous_grid;///< Previous tick buffer

    // Cached stats
    uint32_t m_total_disorder = 0;       ///< Sum of all disorder levels
    uint32_t m_high_disorder_tiles = 0;  ///< Count of tiles above threshold

    /**
     * @brief Calculate the linear index for a coordinate pair.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Linear index into the buffer vectors.
     * @note Does NOT perform bounds checking.
     */
    size_t index(int32_t x, int32_t y) const;
};

} // namespace disorder
} // namespace sims3000

#endif // SIMS3000_DISORDER_DISORDERGRID_H
