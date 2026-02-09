/**
 * @file ServiceFundingIntegration.h
 * @brief Bridge module for service effectiveness with economy funding (Ticket E11-014)
 *
 * Computes the effective service effectiveness given an economy funding level.
 * Uses calculate_effectiveness() from FundingLevel.h and per-service funding
 * levels stored in TreasuryState.
 *
 * No system dependencies -- operates on TreasuryState via FundingLevel functions.
 */

#ifndef SIMS3000_ECONOMY_SERVICEFUNDINGINTEGRATION_H
#define SIMS3000_ECONOMY_SERVICEFUNDINGINTEGRATION_H

#include <cstdint>
#include <sims3000/economy/TreasuryState.h>

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Result types
// ---------------------------------------------------------------------------

/**
 * @struct ServiceFundingResult
 * @brief Result of calculating funding-adjusted effectiveness for a single service.
 */
struct ServiceFundingResult {
    uint8_t service_type;           ///< Service type (0-3)
    uint8_t funding_level;          ///< Raw funding % (0-150)
    float effectiveness_factor;     ///< From calculate_effectiveness()
    float base_effectiveness;       ///< Input base effectiveness
    float final_effectiveness;      ///< base * effectiveness_factor
};

/**
 * @struct AllServicesFundingResult
 * @brief Result of calculating funding-adjusted effectiveness for all 4 service types.
 */
struct AllServicesFundingResult {
    ServiceFundingResult services[4]; ///< One per service type (0=Enforcer, 1=HazardResponse, 2=Medical, 3=Education)
};

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

/**
 * @brief Calculate the funding-adjusted effectiveness for a single service.
 *
 * Logic:
 * - Get effectiveness_factor from calculate_effectiveness(funding_level)
 * - final_effectiveness = base_effectiveness * effectiveness_factor
 *
 * @param service_type Service type (0=Enforcer, 1=HazardResponse, 2=Medical, 3=Education).
 * @param base_effectiveness Input base effectiveness (typically 1.0).
 * @param funding_level Raw funding percentage (0-150).
 * @return ServiceFundingResult with all fields populated.
 */
ServiceFundingResult calculate_funded_effectiveness(
    uint8_t service_type,
    float base_effectiveness,
    uint8_t funding_level);

/**
 * @brief Calculate funding-adjusted effectiveness for all 4 service types.
 *
 * Reads per-service funding levels from the TreasuryState and computes
 * effectiveness for each.
 *
 * @param treasury Treasury state to read funding levels from.
 * @param base_effectiveness Default base effectiveness for all services (default 1.0).
 * @return AllServicesFundingResult with results for all 4 service types.
 */
AllServicesFundingResult calculate_all_funded_effectiveness(
    const TreasuryState& treasury,
    float base_effectiveness = 1.0f);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_SERVICEFUNDINGINTEGRATION_H
