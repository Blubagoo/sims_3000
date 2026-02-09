/**
 * @file BondIssuance.h
 * @brief Bond issuance validation and execution (Ticket E11-016)
 *
 * Provides bond issuance validation (can_issue_bond), bond creation
 * (issue_bond), and bond config lookup (get_bond_config).
 *
 * Validation rules:
 * - Must have fewer than MAX_BONDS_PER_PLAYER (5) active bonds
 * - Large bond requires population > 5000
 * - Emergency bonds cannot be issued manually
 *
 * No system dependencies -- operates directly on TreasuryState and CreditAdvance.
 */

#ifndef SIMS3000_ECONOMY_BONDISSUANCE_H
#define SIMS3000_ECONOMY_BONDISSUANCE_H

#include <cstdint>
#include <sims3000/economy/CreditAdvance.h>
#include <sims3000/economy/TreasuryState.h>

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
namespace constants {

constexpr uint32_t LARGE_BOND_POPULATION_REQUIREMENT = 5000;

} // namespace constants

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

/**
 * @struct BondIssuedEvent
 * @brief Event data emitted when a bond is successfully issued.
 */
struct BondIssuedEvent {
    uint8_t player_id;                          ///< Issuing player
    int64_t principal;                          ///< Bond principal amount
    uint16_t interest_rate_basis_points;        ///< Interest rate in basis points
    BondType bond_type;                         ///< Type of bond issued
};

// ---------------------------------------------------------------------------
// Result types
// ---------------------------------------------------------------------------

/**
 * @struct BondIssuanceResult
 * @brief Result of attempting to issue a bond.
 */
struct BondIssuanceResult {
    bool success;                   ///< Whether the bond was successfully issued
    CreditAdvance bond;             ///< The issued bond (valid if success)
    int64_t principal_added;        ///< Amount added to treasury (valid if success)
};

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

/**
 * @brief Get the bond configuration for a given bond type.
 * @param type The bond type to look up.
 * @return Reference to the corresponding BondConfig.
 */
const BondConfig& get_bond_config(BondType type);

/**
 * @brief Validate if a bond can be issued.
 *
 * Checks:
 * - Active bonds count < MAX_BONDS_PER_PLAYER (5)
 * - Large bond requires population > 5000
 * - Emergency bonds cannot be issued manually
 *
 * @param treasury The player's treasury state (read-only).
 * @param type The bond type to validate.
 * @param population Current city population (used for large bond check).
 * @return true if the bond can be issued, false otherwise.
 */
bool can_issue_bond(const TreasuryState& treasury, BondType type, uint32_t population = 0);

/**
 * @brief Issue a bond: create CreditAdvance, add principal to treasury.
 *
 * If validation fails, returns a result with success=false.
 * If validation passes:
 * - Creates a CreditAdvance from the BondConfig
 * - Adds principal to treasury.balance
 * - Pushes bond to treasury.active_bonds
 *
 * @param treasury The player's treasury state (modified in place).
 * @param type The bond type to issue.
 * @param player_id Player issuing the bond.
 * @param population Current city population (for large bond validation).
 * @return BondIssuanceResult with success flag and bond data.
 */
BondIssuanceResult issue_bond(TreasuryState& treasury, BondType type,
                              uint8_t player_id, uint32_t population = 0);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_BONDISSUANCE_H
