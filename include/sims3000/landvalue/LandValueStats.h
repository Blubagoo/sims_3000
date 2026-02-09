/**
 * @file LandValueStats.h
 * @brief IStatQueryable implementation for land value statistics (Ticket E10-106)
 *
 * Provides stat query functions for LandValueGrid, exposing:
 * - Average land value across all tiles
 * - Maximum and minimum land values
 * - Count of high-value tiles (above 192)
 * - Count of low-value tiles (below 64)
 *
 * These stats can be used by UI systems, analytics, and economic systems.
 */

#ifndef SIMS3000_LANDVALUE_LANDVALUESTATS_H
#define SIMS3000_LANDVALUE_LANDVALUESTATS_H

#include <sims3000/landvalue/LandValueGrid.h>
#include <cstdint>

namespace sims3000 {
namespace landvalue {

// ============================================================================
// Stat IDs
// ============================================================================

/// Average land value across all tiles (0-255)
constexpr uint16_t STAT_AVERAGE_LAND_VALUE = 600;

/// Maximum land value in the grid (0-255)
constexpr uint16_t STAT_MAX_LAND_VALUE = 601;

/// Minimum land value in the grid (0-255)
constexpr uint16_t STAT_MIN_LAND_VALUE = 602;

/// Number of tiles with high land value (value > 192)
constexpr uint16_t STAT_HIGH_VALUE_TILES = 603;

/// Number of tiles with low land value (value < 64)
constexpr uint16_t STAT_LOW_VALUE_TILES = 604;

// ============================================================================
// Constants
// ============================================================================

/// Threshold for high-value tiles
constexpr uint8_t HIGH_VALUE_THRESHOLD = 192;

/// Threshold for low-value tiles
constexpr uint8_t LOW_VALUE_THRESHOLD = 64;

// ============================================================================
// Functions
// ============================================================================

/**
 * @brief Get a land value statistic by ID.
 *
 * Computes the requested statistic from the LandValueGrid. For invalid
 * stat IDs, returns 0.0f.
 *
 * Note: Some stats (average, max, min, counts) require iterating over
 * the entire grid, so they may be moderately expensive. Consider caching
 * results if querying frequently.
 *
 * @param grid LandValueGrid to query.
 * @param stat_id Stat ID (one of STAT_* constants).
 * @return Stat value as float. Returns 0.0f for invalid IDs.
 */
float get_landvalue_stat(const LandValueGrid& grid, uint16_t stat_id);

/**
 * @brief Get the human-readable name of a land value stat.
 *
 * @param stat_id Stat ID (one of STAT_* constants).
 * @return Null-terminated string name. Returns "Unknown" for invalid IDs.
 */
const char* get_landvalue_stat_name(uint16_t stat_id);

/**
 * @brief Check if a stat ID is valid for land value stats.
 *
 * @param stat_id Stat ID to check.
 * @return true if stat_id is a valid land value stat ID.
 */
bool is_valid_landvalue_stat(uint16_t stat_id);

} // namespace landvalue
} // namespace sims3000

#endif // SIMS3000_LANDVALUE_LANDVALUESTATS_H
