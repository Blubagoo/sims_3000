/**
 * @file FundingLevel.cpp
 * @brief Implementation of funding level storage and API (Ticket E11-013)
 */

#include "sims3000/economy/FundingLevel.h"

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// clamp_funding_level
// ---------------------------------------------------------------------------
uint8_t clamp_funding_level(uint8_t level) {
    if (level > constants::MAX_FUNDING_LEVEL) {
        return constants::MAX_FUNDING_LEVEL;
    }
    return level;
}

// ---------------------------------------------------------------------------
// get_funding_level
// ---------------------------------------------------------------------------
uint8_t get_funding_level(const TreasuryState& treasury, uint8_t service_type) {
    switch (service_type) {
        case 0: return treasury.funding_enforcer;          // Enforcer
        case 1: return treasury.funding_hazard_response;   // HazardResponse
        case 2: return treasury.funding_medical;           // Medical
        case 3: return treasury.funding_education;         // Education
        default: return constants::DEFAULT_FUNDING_LEVEL;
    }
}

// ---------------------------------------------------------------------------
// set_funding_level
// ---------------------------------------------------------------------------
FundingLevelChangedEvent set_funding_level(TreasuryState& treasury,
                                           uint8_t service_type,
                                           uint8_t level,
                                           uint8_t player_id) {
    const uint8_t clamped = clamp_funding_level(level);
    const uint8_t old_level = get_funding_level(treasury, service_type);

    switch (service_type) {
        case 0: treasury.funding_enforcer        = clamped; break;
        case 1: treasury.funding_hazard_response  = clamped; break;
        case 2: treasury.funding_medical          = clamped; break;
        case 3: treasury.funding_education        = clamped; break;
        default: break; // unknown service type, no-op
    }

    FundingLevelChangedEvent event;
    event.player_id    = player_id;
    event.service_type = service_type;
    event.old_level    = old_level;
    event.new_level    = clamped;
    return event;
}

// ---------------------------------------------------------------------------
// calculate_effectiveness
// ---------------------------------------------------------------------------
float calculate_effectiveness(uint8_t funding_level) {
    // Clamp to max
    const uint8_t level = clamp_funding_level(funding_level);

    // Piecewise linear interpolation between key points:
    // (0, 0.0), (25, 0.40), (50, 0.65), (75, 0.85), (100, 1.0), (150, 1.10)

    if (level <= 25) {
        // 0 -> 0.0, 25 -> 0.40
        // slope = 0.40 / 25 = 0.016
        return static_cast<float>(level) * (0.40f / 25.0f);
    }
    if (level <= 50) {
        // 25 -> 0.40, 50 -> 0.65
        // slope = 0.25 / 25 = 0.01
        return 0.40f + static_cast<float>(level - 25) * (0.25f / 25.0f);
    }
    if (level <= 75) {
        // 50 -> 0.65, 75 -> 0.85
        // slope = 0.20 / 25 = 0.008
        return 0.65f + static_cast<float>(level - 50) * (0.20f / 25.0f);
    }
    if (level <= 100) {
        // 75 -> 0.85, 100 -> 1.0
        // slope = 0.15 / 25 = 0.006
        return 0.85f + static_cast<float>(level - 75) * (0.15f / 25.0f);
    }
    // 100 -> 1.0, 150 -> 1.10
    // slope = 0.10 / 50 = 0.002
    return 1.0f + static_cast<float>(level - 100) * (0.10f / 50.0f);
}

} // namespace economy
} // namespace sims3000
