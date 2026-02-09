/**
 * @file TributeDemandModifier.cpp
 * @brief Implementation of tribute rate to demand modifier (Ticket E11-019)
 */

#include "sims3000/economy/TributeDemandModifier.h"

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// calculate_tribute_demand_modifier
// ---------------------------------------------------------------------------
int calculate_tribute_demand_modifier(uint8_t tribute_rate) {
    // Tier 1: Rate 0-3% -> +15 bonus
    if (tribute_rate <= 3) {
        return 15;
    }

    // Tier 2: Rate 4-7% -> neutral (0)
    if (tribute_rate <= 7) {
        return 0;
    }

    // Tier 3: Rate 8-12% -> -4 per % above 7
    if (tribute_rate <= 12) {
        return -4 * static_cast<int>(tribute_rate - 7);
    }

    // Tier 4: Rate 13-16% -> -20 base - 5 per % above 12
    if (tribute_rate <= 16) {
        return -20 - 5 * static_cast<int>(tribute_rate - 12);
    }

    // Tier 5: Rate 17-20% -> -40 base - 5 per % above 16
    return -40 - 5 * static_cast<int>(tribute_rate - 16);
}

// ---------------------------------------------------------------------------
// get_zone_tribute_modifier
// ---------------------------------------------------------------------------
int get_zone_tribute_modifier(const TreasuryState& treasury, uint8_t zone_type) {
    uint8_t rate = 0;
    switch (zone_type) {
        case 0: rate = treasury.tribute_rate_habitation;  break;
        case 1: rate = treasury.tribute_rate_exchange;    break;
        case 2: rate = treasury.tribute_rate_fabrication; break;
        default: return 0; // unknown zone type
    }
    return calculate_tribute_demand_modifier(rate);
}

} // namespace economy
} // namespace sims3000
