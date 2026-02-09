/**
 * @file BondRepayment.h
 * @brief Detailed bond repayment processing with events (Ticket E11-017)
 *
 * Extends the basic bond payment processing from BudgetCycle.h with
 * per-bond detail tracking and BondPaidOffEvent emission when bonds mature.
 *
 * Bond payment per phase per bond:
 * - principal_payment = bond.principal / bond.term_phases
 * - interest_payment = (bond.remaining_principal * bond.interest_rate_basis_points) / (10000 * 12)
 */

#ifndef SIMS3000_ECONOMY_BONDREPAYMENT_H
#define SIMS3000_ECONOMY_BONDREPAYMENT_H

#include <cstdint>
#include <vector>
#include <sims3000/economy/CreditAdvance.h>

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

/**
 * @struct BondPaidOffEvent
 * @brief Event emitted when a bond completes all payments and matures.
 */
struct BondPaidOffEvent {
    uint8_t player_id;              ///< Player who paid off the bond
    int64_t principal;              ///< Original principal of the bond
    int64_t total_interest_paid;    ///< Total interest paid over the life of the bond
    bool was_emergency;             ///< Whether this was an emergency bond
};

// ---------------------------------------------------------------------------
// Detailed payment tracking
// ---------------------------------------------------------------------------

/**
 * @struct DetailedBondPayment
 * @brief Payment breakdown for a single bond in one phase.
 */
struct DetailedBondPayment {
    size_t bond_index;              ///< Index of the bond in the active_bonds vector
    int64_t principal_payment;      ///< Principal portion of this phase's payment
    int64_t interest_payment;       ///< Interest portion of this phase's payment
    int64_t total_payment;          ///< principal_payment + interest_payment
    bool is_final_payment;          ///< Bond matures this phase
};

/**
 * @struct DetailedBondPaymentResult
 * @brief Aggregated result of detailed bond payment processing.
 */
struct DetailedBondPaymentResult {
    std::vector<DetailedBondPayment> payments;  ///< Per-bond payment details
    int64_t total_principal_paid;               ///< Sum of all principal payments
    int64_t total_interest_paid;                ///< Sum of all interest payments
    int64_t total_payment;                      ///< total_principal_paid + total_interest_paid
    std::vector<BondPaidOffEvent> matured_events; ///< Events for bonds that matured
};

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

/**
 * @brief Calculate detailed payments for all active bonds (pure function).
 *
 * Does not modify the bonds. Returns per-bond payment breakdowns and
 * identifies which bonds will mature this phase.
 *
 * @param bonds The active bonds to calculate payments for.
 * @param player_id Player ID for matured event data.
 * @return DetailedBondPaymentResult with per-bond details and totals.
 */
DetailedBondPaymentResult calculate_detailed_bond_payments(
    const std::vector<CreditAdvance>& bonds, uint8_t player_id);

/**
 * @brief Process payments: update bonds, remove matured, return events.
 *
 * For each bond:
 * - Deducts principal_payment from remaining_principal
 * - Decrements phases_remaining
 * - Removes bonds where phases_remaining reaches 0
 * - Emits BondPaidOffEvent for matured bonds
 *
 * @param bonds The active bonds (modified in place; matured bonds removed).
 * @param player_id Player ID for matured event data.
 * @return DetailedBondPaymentResult with per-bond details and events.
 */
DetailedBondPaymentResult process_detailed_bond_payments(
    std::vector<CreditAdvance>& bonds, uint8_t player_id);

/**
 * @brief Get total outstanding debt across all active bonds.
 *
 * @param bonds The active bonds to sum.
 * @return Total remaining principal across all bonds.
 */
int64_t get_total_debt(const std::vector<CreditAdvance>& bonds);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_BONDREPAYMENT_H
