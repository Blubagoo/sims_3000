/**
 * @file DeficitHandling.cpp
 * @brief Implementation of deficit warning and handling (Ticket E11-015)
 */

#include "sims3000/economy/DeficitHandling.h"
#include "sims3000/economy/CreditAdvance.h"

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// check_deficit
// ---------------------------------------------------------------------------
DeficitCheckResult check_deficit(const TreasuryState& treasury, uint8_t player_id) {
    DeficitCheckResult result;
    result.should_warn = false;
    result.should_offer_bond = false;

    // Initialize event data
    result.warning_event.player_id = player_id;
    result.warning_event.balance = treasury.balance;

    result.bond_event.player_id = player_id;
    result.bond_event.bond_principal = BOND_EMERGENCY.principal;

    // Check warning threshold
    if (treasury.balance < constants::DEFICIT_WARNING_THRESHOLD &&
        !treasury.deficit_warning_sent) {
        result.should_warn = true;
    }

    // Check emergency bond threshold
    if (treasury.balance < constants::EMERGENCY_BOND_THRESHOLD &&
        !treasury.emergency_bond_active) {
        result.should_offer_bond = true;
    }

    return result;
}

// ---------------------------------------------------------------------------
// apply_deficit_state
// ---------------------------------------------------------------------------
void apply_deficit_state(TreasuryState& treasury, const DeficitCheckResult& result) {
    if (result.should_warn) {
        treasury.deficit_warning_sent = true;
    }
    if (result.should_offer_bond) {
        treasury.emergency_bond_active = true;
    }
}

// ---------------------------------------------------------------------------
// check_deficit_recovery
// ---------------------------------------------------------------------------
void check_deficit_recovery(TreasuryState& treasury) {
    if (treasury.balance >= 0) {
        treasury.deficit_warning_sent = false;
        treasury.emergency_bond_active = false;
    }
}

} // namespace economy
} // namespace sims3000
