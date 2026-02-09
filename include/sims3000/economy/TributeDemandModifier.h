/**
 * @file TributeDemandModifier.h
 * @brief Tribute rate to demand modifier calculation (Ticket E11-019)
 *
 * Standalone calculation module that converts tribute rates into
 * demand modifiers. Used by DemandSystem (or integration code) to
 * determine how tribute rates affect zone growth demand.
 *
 * Tiered system:
 * - Rate 0-3%:   +15 demand bonus
 * - Rate 4-7%:   0 (neutral)
 * - Rate 8-12%:  -4 per % above 7 (-4 to -20)
 * - Rate 13-16%: -20 base - 5 per % above 12 (-25 to -40)
 * - Rate 17-20%: -40 base - 5 per % above 16 (-45 to -60)
 *
 * No system dependencies -- operates directly on TreasuryState.
 */

#ifndef SIMS3000_ECONOMY_TRIBUTEDEMANDMODIFIER_H
#define SIMS3000_ECONOMY_TRIBUTEDEMANDMODIFIER_H

#include <cstdint>
#include <sims3000/economy/TreasuryState.h>

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

/**
 * @brief Calculate demand modifier based on tribute rate.
 *
 * Tiered system:
 * - Rate 0-3%:   +15 demand bonus
 * - Rate 4-7%:   0 (neutral)
 * - Rate 8-12%:  -4 per % above 7 (-4 to -20)
 * - Rate 13-16%: -20 base - 5 per % above 12 (-25 to -40)
 * - Rate 17-20%: -40 base - 5 per % above 16 (-45 to -60)
 *
 * @param tribute_rate Tribute rate in percent (0-20).
 * @return Demand modifier (positive = bonus, negative = penalty).
 */
int calculate_tribute_demand_modifier(uint8_t tribute_rate);

/**
 * @brief Get the tribute demand modifier for a specific zone type.
 *
 * Convenience function that reads the appropriate tribute rate from
 * the treasury state and calculates the demand modifier.
 *
 * @param treasury Treasury state to query.
 * @param zone_type Zone type (0=habitation, 1=exchange, 2=fabrication).
 * @return Demand modifier for the zone type.
 */
int get_zone_tribute_modifier(const TreasuryState& treasury, uint8_t zone_type);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_TRIBUTEDEMANDMODIFIER_H
