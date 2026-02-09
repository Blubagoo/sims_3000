/**
 * @file EmergencyBond.cpp
 * @brief Implementation of emergency bond auto-issuance (Ticket E11-018)
 */

#include "sims3000/economy/EmergencyBond.h"
#include "sims3000/economy/DeficitHandling.h"

namespace sims3000 {
namespace economy {

EmergencyBondResult check_and_issue_emergency_bond(
    TreasuryState& treasury,
    uint8_t player_id,
    bool auto_bonds_enabled) {

    EmergencyBondResult result;
    result.issued = false;
    result.event = EmergencyBondIssuedEvent{};

    // Check all conditions
    if (treasury.balance >= constants::EMERGENCY_BOND_THRESHOLD) {
        return result;
    }
    if (treasury.emergency_bond_active) {
        return result;
    }
    if (!auto_bonds_enabled) {
        return result;
    }

    // Record balance before
    int64_t balance_before = treasury.balance;

    // Create CreditAdvance from BOND_EMERGENCY config
    CreditAdvance bond;
    bond.principal = BOND_EMERGENCY.principal;
    bond.remaining_principal = BOND_EMERGENCY.principal;
    bond.interest_rate_basis_points = BOND_EMERGENCY.interest_rate;
    bond.term_phases = BOND_EMERGENCY.term_phases;
    bond.phases_remaining = BOND_EMERGENCY.term_phases;
    bond.is_emergency = true;

    // Add principal to treasury balance
    treasury.balance += bond.principal;

    // Push to active bonds
    treasury.active_bonds.push_back(bond);

    // Set emergency bond active flag
    treasury.emergency_bond_active = true;

    // Build result
    result.issued = true;
    result.event.player_id = player_id;
    result.event.principal = bond.principal;
    result.event.balance_before = balance_before;
    result.event.balance_after = treasury.balance;

    return result;
}

} // namespace economy
} // namespace sims3000
