/**
 * @file BondIssuance.cpp
 * @brief Implementation of bond issuance system (Ticket E11-016)
 */

#include "sims3000/economy/BondIssuance.h"

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Bond config storage (static, returned by reference)
// ---------------------------------------------------------------------------
static const BondConfig s_bond_configs[] = {
    BOND_SMALL,      // BondType::Small     = 0
    BOND_STANDARD,   // BondType::Standard  = 1
    BOND_LARGE,      // BondType::Large     = 2
    BOND_EMERGENCY   // BondType::Emergency = 3
};

// ---------------------------------------------------------------------------
// get_bond_config
// ---------------------------------------------------------------------------
const BondConfig& get_bond_config(BondType type) {
    return s_bond_configs[static_cast<uint8_t>(type)];
}

// ---------------------------------------------------------------------------
// can_issue_bond
// ---------------------------------------------------------------------------
bool can_issue_bond(const TreasuryState& treasury, BondType type, uint32_t population) {
    // Cannot issue emergency bonds manually
    if (type == BondType::Emergency) {
        return false;
    }

    // Must have fewer than MAX_BONDS_PER_PLAYER active bonds
    if (static_cast<int>(treasury.active_bonds.size()) >= MAX_BONDS_PER_PLAYER) {
        return false;
    }

    // Large bond requires population > 5000
    if (type == BondType::Large && population <= constants::LARGE_BOND_POPULATION_REQUIREMENT) {
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// issue_bond
// ---------------------------------------------------------------------------
BondIssuanceResult issue_bond(TreasuryState& treasury, BondType type,
                              uint8_t player_id, uint32_t population) {
    BondIssuanceResult result;
    result.success = false;
    result.bond = CreditAdvance{};
    result.principal_added = 0;

    // Validate
    if (!can_issue_bond(treasury, type, population)) {
        return result;
    }

    // Get config
    const BondConfig& config = get_bond_config(type);

    // Create CreditAdvance from config
    CreditAdvance bond;
    bond.principal = config.principal;
    bond.remaining_principal = config.principal;
    bond.interest_rate_basis_points = config.interest_rate;
    bond.term_phases = config.term_phases;
    bond.phases_remaining = config.term_phases;
    bond.is_emergency = config.is_emergency;

    // Add principal to treasury balance
    treasury.balance += config.principal;

    // Push bond to active_bonds
    treasury.active_bonds.push_back(bond);

    // Build result
    result.success = true;
    result.bond = bond;
    result.principal_added = config.principal;

    return result;
}

} // namespace economy
} // namespace sims3000
