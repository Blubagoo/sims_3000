/**
 * @file DemandStats.h
 * @brief IStatQueryable implementation for demand statistics (Ticket E10-048)
 *
 * Provides stat query functions for DemandData, exposing:
 * - Raw demand values for habitation, exchange, fabrication (-100 to +100)
 * - Demand caps (maximum growth capacity) for each zone type
 *
 * These stats can be used by UI systems, analytics, and other systems
 * that need to query demand state.
 */

#ifndef SIMS3000_DEMAND_DEMANDSTATS_H
#define SIMS3000_DEMAND_DEMANDSTATS_H

#include <sims3000/demand/DemandData.h>
#include <cstdint>

namespace sims3000 {
namespace demand {

// ============================================================================
// Stat IDs
// ============================================================================

/// Habitation (residential) zone demand (-100 to +100)
constexpr uint16_t STAT_HABITATION_DEMAND = 300;

/// Exchange (commercial) zone demand (-100 to +100)
constexpr uint16_t STAT_EXCHANGE_DEMAND = 301;

/// Fabrication (industrial) zone demand (-100 to +100)
constexpr uint16_t STAT_FABRICATION_DEMAND = 302;

/// Habitation zone demand cap (max growth capacity)
constexpr uint16_t STAT_HABITATION_CAP = 303;

/// Exchange zone demand cap (max growth capacity)
constexpr uint16_t STAT_EXCHANGE_CAP = 304;

/// Fabrication zone demand cap (max growth capacity)
constexpr uint16_t STAT_FABRICATION_CAP = 305;

// ============================================================================
// Functions
// ============================================================================

/**
 * @brief Get a demand statistic by ID.
 *
 * Returns the requested statistic from DemandData. For invalid stat IDs,
 * returns 0.0f.
 *
 * @param data DemandData to query.
 * @param stat_id Stat ID (one of STAT_* constants).
 * @return Stat value as float. Returns 0.0f for invalid IDs.
 */
float get_demand_stat(const DemandData& data, uint16_t stat_id);

/**
 * @brief Get the human-readable name of a demand stat.
 *
 * @param stat_id Stat ID (one of STAT_* constants).
 * @return Null-terminated string name. Returns "Unknown" for invalid IDs.
 */
const char* get_demand_stat_name(uint16_t stat_id);

/**
 * @brief Check if a stat ID is valid for demand stats.
 *
 * @param stat_id Stat ID to check.
 * @return true if stat_id is a valid demand stat ID.
 */
bool is_valid_demand_stat(uint16_t stat_id);

} // namespace demand
} // namespace sims3000

#endif // SIMS3000_DEMAND_DEMANDSTATS_H
