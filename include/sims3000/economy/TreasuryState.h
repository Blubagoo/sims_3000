/**
 * @file TreasuryState.h
 * @brief Per-player treasury state data structure (Ticket E11-002)
 *
 * Defines TreasuryState which tracks a player's financial state
 * including balance, income/expense breakdown, tribute rates,
 * service funding levels, and active bonds.
 */

#ifndef SIMS3000_ECONOMY_TREASURYSTATE_H
#define SIMS3000_ECONOMY_TREASURYSTATE_H

#include <cstdint>
#include <vector>
#include <sims3000/economy/CreditAdvance.h>

namespace sims3000 {
namespace economy {

/**
 * @struct TreasuryState
 * @brief Complete financial state for a single player.
 *
 * Tracks current balance, last-phase income/expense totals,
 * detailed income and expense breakdowns, per-zone tribute rates,
 * per-service funding levels, and active credit advances (bonds).
 *
 * Starting balance is 20000 credits (canonical).
 * Default tribute rates are 7% for all zone types.
 * Default service funding levels are 100%.
 */
struct TreasuryState {
    // --- Balance ---
    int64_t balance = 20000;                    ///< Current credit balance (starts at 20000)

    // --- Last-phase totals ---
    int64_t last_income = 0;                    ///< Total income from last phase
    int64_t last_expense = 0;                   ///< Total expense from last phase

    // --- Income breakdown ---
    int64_t habitation_tribute = 0;             ///< Tribute from habitation zones
    int64_t exchange_tribute = 0;               ///< Tribute from exchange zones
    int64_t fabrication_tribute = 0;            ///< Tribute from fabrication zones
    int64_t other_income = 0;                   ///< Miscellaneous income

    // --- Expense breakdown ---
    int64_t infrastructure_maintenance = 0;     ///< Road/utility upkeep
    int64_t service_maintenance = 0;            ///< Service building upkeep
    int64_t energy_maintenance = 0;             ///< Energy system upkeep
    int64_t bond_payments = 0;                  ///< Credit advance repayments
    int64_t ordinance_costs = 0;                ///< Active ordinance costs

    // --- Per-zone tribute rates (0-20%) ---
    uint8_t tribute_rate_habitation = 7;        ///< Habitation tribute rate (%)
    uint8_t tribute_rate_exchange = 7;          ///< Exchange tribute rate (%)
    uint8_t tribute_rate_fabrication = 7;       ///< Fabrication tribute rate (%)

    // --- Per-service funding levels (0-150%) ---
    uint8_t funding_enforcer = 100;             ///< Enforcer service funding (%)
    uint8_t funding_hazard_response = 100;      ///< Hazard response funding (%)
    uint8_t funding_medical = 100;              ///< Medical service funding (%)
    uint8_t funding_education = 100;            ///< Education service funding (%)

    // --- Phase tracking ---
    uint8_t last_processed_phase = 0;           ///< Last phase that was processed

    // --- Flags ---
    bool deficit_warning_sent = false;          ///< Whether deficit warning was issued
    bool emergency_bond_active = false;         ///< Whether an emergency bond is active

    // --- Active bonds ---
    std::vector<CreditAdvance> active_bonds;    ///< Currently active credit advances
};

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_TREASURYSTATE_H
