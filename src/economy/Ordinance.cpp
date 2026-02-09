/**
 * @file Ordinance.cpp
 * @brief Implementation of ordinance framework (Ticket E11-021)
 */

#include "sims3000/economy/Ordinance.h"

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Ordinance config storage (static, returned by reference)
// ---------------------------------------------------------------------------
static const OrdinanceConfig s_ordinance_configs[] = {
    ORDINANCE_ENHANCED_PATROL,       // OrdinanceType::EnhancedPatrol = 0
    ORDINANCE_INDUSTRIAL_SCRUBBERS,  // OrdinanceType::IndustrialScrubbers = 1
    ORDINANCE_FREE_TRANSIT           // OrdinanceType::FreeTransit = 2
};

// ---------------------------------------------------------------------------
// get_ordinance_config
// ---------------------------------------------------------------------------

const OrdinanceConfig& get_ordinance_config(OrdinanceType type) {
    return s_ordinance_configs[static_cast<uint8_t>(type)];
}

// ---------------------------------------------------------------------------
// OrdinanceState methods
// ---------------------------------------------------------------------------

void OrdinanceState::enable(OrdinanceType type) {
    active[static_cast<uint8_t>(type)] = true;
}

void OrdinanceState::disable(OrdinanceType type) {
    active[static_cast<uint8_t>(type)] = false;
}

bool OrdinanceState::is_active(OrdinanceType type) const {
    return active[static_cast<uint8_t>(type)];
}

int64_t OrdinanceState::get_total_cost() const {
    int64_t total = 0;
    for (uint8_t i = 0; i < ORDINANCE_TYPE_COUNT; ++i) {
        if (active[i]) {
            total += s_ordinance_configs[i].cost_per_phase;
        }
    }
    return total;
}

} // namespace economy
} // namespace sims3000
