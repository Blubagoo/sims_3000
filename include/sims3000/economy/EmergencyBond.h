/**
 * @file EmergencyBond.h
 * @brief Emergency bond auto-issuance system (Ticket E11-018)
 *
 * Automatically issues an emergency bond when a player's treasury
 * balance drops below EMERGENCY_BOND_THRESHOLD (-10,000) and no
 * emergency bond is currently active.
 *
 * Uses BOND_EMERGENCY config: 25K principal, 15% interest, 12 phases.
 */

#ifndef SIMS3000_ECONOMY_EMERGENCYBOND_H
#define SIMS3000_ECONOMY_EMERGENCYBOND_H

#include <cstdint>
#include <sims3000/economy/CreditAdvance.h>
#include <sims3000/economy/TreasuryState.h>

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

/**
 * @struct EmergencyBondIssuedEvent
 * @brief Event emitted when an emergency bond is auto-issued.
 */
struct EmergencyBondIssuedEvent {
    uint8_t player_id;          ///< Player receiving the emergency bond
    int64_t principal;          ///< Principal amount of the emergency bond
    int64_t balance_before;     ///< Treasury balance before bond issuance
    int64_t balance_after;      ///< Treasury balance after bond issuance
};

// ---------------------------------------------------------------------------
// Result types
// ---------------------------------------------------------------------------

/**
 * @struct EmergencyBondResult
 * @brief Result of checking and potentially issuing an emergency bond.
 */
struct EmergencyBondResult {
    bool issued;                        ///< Whether an emergency bond was issued
    EmergencyBondIssuedEvent event;     ///< Event data (valid if issued)
};

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

/**
 * @brief Check if emergency bond should auto-issue and do it.
 *
 * Conditions for issuance (all must be true):
 * - balance < EMERGENCY_BOND_THRESHOLD (-10,000)
 * - !treasury.emergency_bond_active
 * - auto_bonds_enabled == true
 *
 * If all conditions met:
 * - Creates CreditAdvance from BOND_EMERGENCY config
 * - Adds principal to treasury.balance
 * - Pushes bond to treasury.active_bonds
 * - Sets treasury.emergency_bond_active = true
 *
 * @param treasury The player's treasury state (modified if bond is issued).
 * @param player_id Player ID for the event.
 * @param auto_bonds_enabled Player setting to enable/disable auto-bonds.
 * @return EmergencyBondResult with issued flag and event data.
 */
EmergencyBondResult check_and_issue_emergency_bond(
    TreasuryState& treasury,
    uint8_t player_id,
    bool auto_bonds_enabled = true);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_EMERGENCYBOND_H
