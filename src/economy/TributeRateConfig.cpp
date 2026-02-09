/**
 * @file TributeRateConfig.cpp
 * @brief Implementation of per-zone tribute rate utilities (Ticket E11-006)
 */

#include "sims3000/economy/TributeRateConfig.h"

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// clamp_tribute_rate
// ---------------------------------------------------------------------------
uint8_t clamp_tribute_rate(uint8_t rate) {
    if (rate > constants::MAX_TRIBUTE_RATE) {
        return constants::MAX_TRIBUTE_RATE;
    }
    return rate;
}

// ---------------------------------------------------------------------------
// get_tribute_rate
// ---------------------------------------------------------------------------
uint8_t get_tribute_rate(const TreasuryState& treasury, ZoneBuildingType zone_type) {
    switch (zone_type) {
        case ZoneBuildingType::Habitation:  return treasury.tribute_rate_habitation;
        case ZoneBuildingType::Exchange:    return treasury.tribute_rate_exchange;
        case ZoneBuildingType::Fabrication: return treasury.tribute_rate_fabrication;
    }
    // Should never be reached for valid enum values; return default as safety net.
    return constants::DEFAULT_TRIBUTE_RATE;
}

// ---------------------------------------------------------------------------
// set_tribute_rate
// ---------------------------------------------------------------------------
TributeRateChangedEvent set_tribute_rate(TreasuryState& treasury,
                                         ZoneBuildingType zone_type,
                                         uint8_t rate,
                                         uint8_t player_id) {
    const uint8_t clamped = clamp_tribute_rate(rate);
    const uint8_t old_rate = get_tribute_rate(treasury, zone_type);

    switch (zone_type) {
        case ZoneBuildingType::Habitation:
            treasury.tribute_rate_habitation = clamped;
            break;
        case ZoneBuildingType::Exchange:
            treasury.tribute_rate_exchange = clamped;
            break;
        case ZoneBuildingType::Fabrication:
            treasury.tribute_rate_fabrication = clamped;
            break;
    }

    TributeRateChangedEvent event;
    event.player_id = player_id;
    event.zone_type = zone_type;
    event.old_rate  = old_rate;
    event.new_rate  = clamped;
    return event;
}

// ---------------------------------------------------------------------------
// get_average_tribute_rate
// ---------------------------------------------------------------------------
float get_average_tribute_rate(const TreasuryState& treasury) {
    const float sum = static_cast<float>(treasury.tribute_rate_habitation)
                    + static_cast<float>(treasury.tribute_rate_exchange)
                    + static_cast<float>(treasury.tribute_rate_fabrication);
    return sum / 3.0f;
}

} // namespace economy
} // namespace sims3000
