/**
 * @file DeficitHandling.h
 * @brief Deficit warning and handling (Ticket E11-015)
 *
 * Pure calculation module for detecting deficit conditions,
 * generating warning/emergency bond events, and tracking
 * recovery from deficit states.
 *
 * No system dependencies -- operates directly on TreasuryState.
 */

#ifndef SIMS3000_ECONOMY_DEFICITHANDLING_H
#define SIMS3000_ECONOMY_DEFICITHANDLING_H

#include <cstdint>
#include <sims3000/economy/TreasuryState.h>

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
namespace constants {

constexpr int64_t DEFICIT_WARNING_THRESHOLD  = -5000;
constexpr int64_t EMERGENCY_BOND_THRESHOLD   = -10000;

} // namespace constants

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

/**
 * @struct DeficitWarningEvent
 * @brief Event emitted when a player's balance drops below the warning threshold.
 */
struct DeficitWarningEvent {
    uint8_t player_id;   ///< Player whose balance triggered the warning
    int64_t balance;     ///< Current treasury balance at time of warning
};

/**
 * @struct EmergencyBondOfferEvent
 * @brief Event emitted when a player qualifies for an emergency bond.
 */
struct EmergencyBondOfferEvent {
    uint8_t player_id;       ///< Player qualifying for emergency bond
    int64_t bond_principal;  ///< Suggested emergency bond principal
};

// ---------------------------------------------------------------------------
// Deficit Check Result
// ---------------------------------------------------------------------------

/**
 * @struct DeficitCheckResult
 * @brief Result of checking a player's deficit status.
 */
struct DeficitCheckResult {
    bool should_warn;                          ///< balance < -5000 and warning not yet sent
    bool should_offer_bond;                    ///< balance < -10000 and no emergency bond active
    DeficitWarningEvent warning_event;         ///< Warning event data (valid if should_warn)
    EmergencyBondOfferEvent bond_event;        ///< Bond offer event data (valid if should_offer_bond)
};

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

/**
 * @brief Check deficit status and determine what actions to take.
 *
 * Logic:
 * - Warning at balance < -5000: set should_warn if !treasury.deficit_warning_sent
 * - Emergency bond at balance < -10000: set should_offer_bond if !treasury.emergency_bond_active
 *
 * @param treasury The player's treasury state (read-only).
 * @param player_id Player ID for event data.
 * @return DeficitCheckResult with flags and event data.
 */
DeficitCheckResult check_deficit(const TreasuryState& treasury, uint8_t player_id);

/**
 * @brief Apply deficit state changes to treasury based on check result.
 *
 * Sets deficit_warning_sent and/or emergency_bond_active flags
 * based on the check result.
 *
 * @param treasury The player's treasury state (modified in place).
 * @param result The result from check_deficit.
 */
void apply_deficit_state(TreasuryState& treasury, const DeficitCheckResult& result);

/**
 * @brief Reset warning flags when balance recovers.
 *
 * If balance >= 0, resets deficit_warning_sent and emergency_bond_active.
 *
 * @param treasury The player's treasury state (modified in place).
 */
void check_deficit_recovery(TreasuryState& treasury);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_DEFICITHANDLING_H
