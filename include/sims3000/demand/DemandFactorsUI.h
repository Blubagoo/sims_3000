/**
 * @file DemandFactorsUI.h
 * @brief Helper functions for UI demand factor queries (Ticket E10-047).
 *
 * Provides utilities for accessing and interpreting DemandFactors from
 * DemandData. These functions enable the UI to display demand breakdowns
 * and explain to players why demand is high or low for each zone type.
 *
 * @see E10-047
 */

#ifndef SIMS3000_DEMAND_DEMANDFACTORSUI_H
#define SIMS3000_DEMAND_DEMANDFACTORSUI_H

#include <sims3000/demand/DemandData.h>
#include <cstdint>

namespace sims3000 {
namespace demand {

// Zone type constants (matching canon from ZoneTypes.h)
constexpr uint8_t ZONE_HABITATION = 0;   ///< Residential zones
constexpr uint8_t ZONE_EXCHANGE = 1;     ///< Commercial zones
constexpr uint8_t ZONE_FABRICATION = 2;  ///< Industrial zones

/**
 * @brief Get demand factors for a specific zone type from DemandData.
 *
 * Returns a reference to the appropriate DemandFactors struct based on
 * the zone type. Use this to access factor breakdowns for UI display.
 *
 * @param data DemandData containing all zone demand information.
 * @param zone_type Zone type (ZONE_HABITATION, ZONE_EXCHANGE, ZONE_FABRICATION).
 * @return Reference to the DemandFactors for the specified zone type.
 *         If zone_type is invalid, returns habitation_factors.
 */
const DemandFactors& get_demand_factors(const DemandData& data, uint8_t zone_type);

/**
 * @brief Get the dominant (most impactful) factor name for a zone type.
 *
 * Identifies the factor with the largest absolute value, indicating which
 * factor is most responsible for current demand state (positive or negative).
 *
 * @param factors DemandFactors to analyze.
 * @return Name of the dominant factor (e.g., "population", "employment").
 *         Returns "none" if all factors are zero.
 */
const char* get_dominant_factor_name(const DemandFactors& factors);

/**
 * @brief Get a text description of demand state based on demand value.
 *
 * Converts raw demand value (-100 to +100) into a human-readable description.
 *
 * Thresholds:
 * - >= 75: "Strong Growth"
 * - >= 25: "Growth"
 * - >= 10: "Weak Growth"
 * - > -10: "Stagnant"
 * - > -25: "Weak Decline"
 * - > -75: "Decline"
 * - <= -75: "Strong Decline"
 *
 * @param demand_value Raw demand value (-100 to +100).
 * @return Human-readable description string.
 */
const char* get_demand_description(int8_t demand_value);

/**
 * @brief Sum all factors to get total factor contribution.
 *
 * Returns the sum of all six factor values. Useful for verifying factor
 * calculations or displaying total factor impact.
 *
 * @param factors DemandFactors to sum.
 * @return Sum of all factors (may exceed int8_t range, hence int16_t).
 */
int16_t sum_factors(const DemandFactors& factors);

/**
 * @brief Check if demand is bottlenecked by a specific factor.
 *
 * Returns true if the named factor is negative and has the largest absolute
 * value, indicating it's the primary bottleneck preventing demand growth.
 *
 * Factor names: "population", "employment", "services", "tribute",
 *               "transport", "contamination"
 *
 * @param factors DemandFactors to check.
 * @param factor_name Name of the factor to check (case-sensitive).
 * @return true if the factor is negative and dominant, false otherwise.
 */
bool is_bottlenecked_by(const DemandFactors& factors, const char* factor_name);

} // namespace demand
} // namespace sims3000

#endif // SIMS3000_DEMAND_DEMANDFACTORSUI_H
