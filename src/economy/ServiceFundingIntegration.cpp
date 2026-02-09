/**
 * @file ServiceFundingIntegration.cpp
 * @brief Implementation of service funding integration (Ticket E11-014)
 */

#include "sims3000/economy/ServiceFundingIntegration.h"
#include "sims3000/economy/FundingLevel.h"

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// calculate_funded_effectiveness
// ---------------------------------------------------------------------------
ServiceFundingResult calculate_funded_effectiveness(
    uint8_t service_type,
    float base_effectiveness,
    uint8_t funding_level) {

    ServiceFundingResult result;
    result.service_type = service_type;
    result.funding_level = funding_level;
    result.effectiveness_factor = calculate_effectiveness(funding_level);
    result.base_effectiveness = base_effectiveness;
    result.final_effectiveness = base_effectiveness * result.effectiveness_factor;
    return result;
}

// ---------------------------------------------------------------------------
// calculate_all_funded_effectiveness
// ---------------------------------------------------------------------------
AllServicesFundingResult calculate_all_funded_effectiveness(
    const TreasuryState& treasury,
    float base_effectiveness) {

    AllServicesFundingResult result;

    for (uint8_t i = 0; i < 4; ++i) {
        const uint8_t level = get_funding_level(treasury, i);
        result.services[i] = calculate_funded_effectiveness(i, base_effectiveness, level);
    }

    return result;
}

} // namespace economy
} // namespace sims3000
