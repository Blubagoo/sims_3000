/**
 * @file BudgetCycle.h
 * @brief Budget cycle processing -- combines income and expenses (Ticket E11-012)
 *
 * Provides the main budget processing logic that runs each budget phase:
 * - Calculates net change (income - expenses)
 * - Updates treasury balance
 * - Applies income/expense breakdowns to treasury fields
 * - Processes bond payments (principal + interest)
 * - Emits BudgetCycleCompletedEvent via return value
 */

#ifndef SIMS3000_ECONOMY_BUDGETCYCLE_H
#define SIMS3000_ECONOMY_BUDGETCYCLE_H

#include <cstdint>
#include <vector>
#include <sims3000/economy/CreditAdvance.h>
#include <sims3000/economy/RevenueTracking.h>
#include <sims3000/economy/ExpenseTracking.h>
#include <sims3000/economy/TreasuryState.h>

namespace sims3000 {
namespace economy {

// ============================================================================
// Budget Cycle Event
// ============================================================================

/**
 * @struct BudgetCycleCompletedEvent
 * @brief Event emitted after each budget cycle completes.
 */
struct BudgetCycleCompletedEvent {
    uint8_t player_id;        ///< Player who completed the budget cycle
    int64_t income;           ///< Total income for this phase
    int64_t expenses;         ///< Total expenses for this phase
    int64_t balance_after;    ///< Treasury balance after processing
    int64_t net_change;       ///< income - expenses
};

// ============================================================================
// Budget Cycle Input / Result
// ============================================================================

/**
 * @struct BudgetCycleInput
 * @brief Combined income and expense breakdowns for one budget phase.
 */
struct BudgetCycleInput {
    IncomeBreakdown income;     ///< Income breakdown for this phase
    ExpenseBreakdown expenses;  ///< Expense breakdown for this phase
};

/**
 * @struct BudgetCycleResult
 * @brief Result of processing a complete budget cycle.
 */
struct BudgetCycleResult {
    int64_t net_change;          ///< income.total - expenses.total
    int64_t new_balance;         ///< treasury.balance after update
    bool is_deficit;             ///< true if new_balance < 0
    BudgetCycleCompletedEvent event;  ///< Event data for notification
};

/**
 * @brief Process a complete budget cycle for a player.
 *
 * Steps:
 * 1. Calculates net_change = income.total - expenses.total
 * 2. Updates treasury.balance += net_change
 * 3. Updates treasury.last_income and treasury.last_expense
 * 4. Applies income/expense breakdowns to treasury category fields
 * 5. Returns BudgetCycleResult with event data
 *
 * @param treasury The player's treasury state (modified in place).
 * @param input Combined income and expense data.
 * @param player_id The player ID for the event.
 * @return BudgetCycleResult with net change, new balance, and event.
 */
BudgetCycleResult process_budget_cycle(
    TreasuryState& treasury,
    const BudgetCycleInput& input,
    uint8_t player_id);

// ============================================================================
// Bond Payment Processing
// ============================================================================

/**
 * @struct BondPaymentResult
 * @brief Result of bond payment calculation/processing.
 */
struct BondPaymentResult {
    int64_t total_payment;    ///< Total payment (principal + interest)
    int64_t principal_paid;   ///< Total principal portion paid
    int64_t interest_paid;    ///< Total interest portion paid
    uint32_t bonds_matured;   ///< Count of bonds that completed this phase
};

/**
 * @brief Calculate bond payments without modifying bonds (pure function).
 *
 * Per bond per phase:
 * - principal_payment = bond.principal / bond.term_phases
 * - interest_payment = (bond.remaining_principal * bond.interest_rate_basis_points) / (10000 * 12)
 * - total = principal_payment + interest_payment
 *
 * A bond is considered maturing if phases_remaining <= 1.
 *
 * @param bonds The active bonds to calculate payments for.
 * @return BondPaymentResult with totals and maturity count.
 */
BondPaymentResult calculate_bond_payments(const std::vector<CreditAdvance>& bonds);

/**
 * @brief Process bond payments: deduct from bonds and remove matured ones.
 *
 * For each bond:
 * - Deducts principal_payment from remaining_principal
 * - Decrements phases_remaining
 * - Removes bonds where phases_remaining reaches 0
 *
 * @param bonds The active bonds (modified in place; matured bonds removed).
 * @return BondPaymentResult with totals and maturity count.
 */
BondPaymentResult process_bond_payments(std::vector<CreditAdvance>& bonds);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_BUDGETCYCLE_H
