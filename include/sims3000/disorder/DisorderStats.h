/**
 * @file DisorderStats.h
 * @brief IStatQueryable interface for disorder statistics (Ticket E10-077)
 *
 * Provides stat queries for the disorder system:
 * - STAT_TOTAL_DISORDER: sum of all disorder levels across the grid
 * - STAT_AVERAGE_DISORDER: average disorder per tile
 * - STAT_HIGH_DISORDER_TILES: count of tiles with disorder >= 128
 * - STAT_MAX_DISORDER: maximum disorder value in the grid
 *
 * Also provides direct position queries via get_disorder_at().
 */

#ifndef SIMS3000_DISORDER_DISORDERSTATS_H
#define SIMS3000_DISORDER_DISORDERSTATS_H

#include <cstdint>

namespace sims3000 {
namespace disorder {

// Forward declaration
class DisorderGrid;

// ============================================================================
// Stat IDs
// ============================================================================

constexpr uint16_t STAT_TOTAL_DISORDER = 400;
constexpr uint16_t STAT_AVERAGE_DISORDER = 401;
constexpr uint16_t STAT_HIGH_DISORDER_TILES = 402;
constexpr uint16_t STAT_MAX_DISORDER = 403;

// ============================================================================
// Stat Query Functions
// ============================================================================

/**
 * @brief Get a stat value from the disorder grid.
 *
 * @param grid The disorder grid to query.
 * @param stat_id The stat ID to retrieve.
 * @return The stat value as a float. Returns 0.0f for invalid stat IDs.
 */
float get_disorder_stat(const DisorderGrid& grid, uint16_t stat_id);

/**
 * @brief Get the human-readable name of a disorder stat.
 *
 * @param stat_id The stat ID.
 * @return Null-terminated string name, or "Unknown" for invalid IDs.
 */
const char* get_disorder_stat_name(uint16_t stat_id);

/**
 * @brief Check if a stat ID is valid.
 *
 * @param stat_id The stat ID to validate.
 * @return true if the stat ID is recognized, false otherwise.
 */
bool is_valid_disorder_stat(uint16_t stat_id);

/**
 * @brief Direct query for disorder level at a specific position.
 *
 * @param grid The disorder grid to query.
 * @param x X coordinate (column).
 * @param y Y coordinate (row).
 * @return Disorder level 0-255. Returns 0 for out-of-bounds.
 */
uint8_t get_disorder_at(const DisorderGrid& grid, int32_t x, int32_t y);

} // namespace disorder
} // namespace sims3000

#endif // SIMS3000_DISORDER_DISORDERSTATS_H
