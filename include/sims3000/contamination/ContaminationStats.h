/**
 * @file ContaminationStats.h
 * @brief IStatQueryable interface for contamination statistics (Ticket E10-089)
 *
 * Provides stat query functions for ContaminationGrid, exposing aggregate
 * contamination metrics for the statistics system.
 *
 * Stat IDs:
 * - 500: Total contamination (sum of all levels)
 * - 501: Average contamination (total / cell count)
 * - 502: Toxic tiles (count above 128 threshold)
 * - 503: Max contamination (highest single cell level)
 * - 504: Industrial total (tiles with Industrial dominant type and level > 0)
 * - 505: Traffic total (tiles with Traffic dominant type and level > 0)
 * - 506: Energy total (tiles with Energy dominant type and level > 0)
 * - 507: Terrain total (tiles with Terrain dominant type and level > 0)
 */

#ifndef SIMS3000_CONTAMINATION_CONTAMINATIONSTATS_H
#define SIMS3000_CONTAMINATION_CONTAMINATIONSTATS_H

#include <cstdint>

namespace sims3000 {
namespace contamination {

// Forward declaration
class ContaminationGrid;

// Stat ID constants
constexpr uint16_t STAT_TOTAL_CONTAMINATION = 500;
constexpr uint16_t STAT_AVERAGE_CONTAMINATION = 501;
constexpr uint16_t STAT_TOXIC_TILES = 502;
constexpr uint16_t STAT_MAX_CONTAMINATION = 503;
constexpr uint16_t STAT_INDUSTRIAL_TOTAL = 504;
constexpr uint16_t STAT_TRAFFIC_TOTAL = 505;
constexpr uint16_t STAT_ENERGY_TOTAL = 506;
constexpr uint16_t STAT_TERRAIN_TOTAL = 507;

/**
 * @brief Get a contamination statistic by ID.
 *
 * Returns the requested statistic value from the contamination grid.
 *
 * @param grid The contamination grid to query.
 * @param stat_id Stat ID (500-507).
 * @return The statistic value as a float. Returns 0.0f for invalid stat_id.
 */
float get_contamination_stat(const ContaminationGrid& grid, uint16_t stat_id);

/**
 * @brief Get the contamination level at a specific coordinate.
 *
 * Convenience function for querying individual cell contamination.
 *
 * @param grid The contamination grid to query.
 * @param x X coordinate (column).
 * @param y Y coordinate (row).
 * @return Contamination level 0-255. Returns 0 for out-of-bounds.
 */
uint8_t get_contamination_at(const ContaminationGrid& grid, int32_t x, int32_t y);

/**
 * @brief Get the human-readable name for a contamination stat ID.
 *
 * @param stat_id Stat ID (500-507).
 * @return Null-terminated string name, or "Unknown" for invalid IDs.
 */
const char* get_contamination_stat_name(uint16_t stat_id);

/**
 * @brief Check if a stat ID is a valid contamination stat.
 *
 * @param stat_id Stat ID to check.
 * @return true if stat_id is in the range 500-507.
 */
bool is_valid_contamination_stat(uint16_t stat_id);

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_CONTAMINATIONSTATS_H
