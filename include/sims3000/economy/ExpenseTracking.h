/**
 * @file ExpenseTracking.h
 * @brief Expense breakdown and expense history tracking (Ticket E11-011)
 *
 * Provides ExpenseBreakdown for categorized expense tracking,
 * ExpenseHistory for circular-buffer phase history (last 12 phases),
 * and functions to build breakdowns from maintenance results and
 * apply them to TreasuryState.
 */

#ifndef SIMS3000_ECONOMY_EXPENSETRACKING_H
#define SIMS3000_ECONOMY_EXPENSETRACKING_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <sims3000/economy/InfrastructureMaintenance.h>
#include <sims3000/economy/ServiceMaintenance.h>
#include <sims3000/economy/TreasuryState.h>

namespace sims3000 {
namespace economy {

// ============================================================================
// Expense Breakdown
// ============================================================================

/**
 * @struct ExpenseBreakdown
 * @brief Categorized expense totals for a single budget phase.
 */
struct ExpenseBreakdown {
    int64_t infrastructure_maintenance = 0;  ///< Road/utility upkeep
    int64_t service_maintenance = 0;         ///< Service building upkeep
    int64_t energy_maintenance = 0;          ///< Energy system upkeep
    int64_t bond_payments = 0;               ///< Credit advance repayments
    int64_t ordinance_costs = 0;             ///< Active ordinance costs
    int64_t total = 0;                       ///< Sum of all expense categories
};

/**
 * @brief Build an ExpenseBreakdown from component maintenance results.
 *
 * @param infra Infrastructure maintenance aggregate result.
 * @param services Service maintenance summary.
 * @param energy_maintenance Energy system maintenance cost.
 * @param bond_payments Total bond payment amount for this phase.
 * @param ordinance_costs Total ordinance costs for this phase.
 * @return ExpenseBreakdown with categorized totals.
 */
ExpenseBreakdown build_expense_breakdown(
    const InfrastructureMaintenanceResult& infra,
    const ServiceMaintenanceSummary& services,
    int64_t energy_maintenance,
    int64_t bond_payments,
    int64_t ordinance_costs);

// ============================================================================
// Expense History (Circular Buffer)
// ============================================================================

/**
 * @struct ExpenseHistory
 * @brief Tracks the last 12 phases of total expenses for trend analysis.
 *
 * Uses a circular buffer. Same pattern as IncomeHistory.
 */
struct ExpenseHistory {
    static constexpr size_t HISTORY_SIZE = 12;

    std::array<int64_t, HISTORY_SIZE> phases{};  ///< Circular buffer of expense values
    size_t current_index = 0;                     ///< Next write position
    size_t count = 0;                             ///< Number of recorded entries (max HISTORY_SIZE)

    /**
     * @brief Record a new expense value, advancing the circular buffer.
     * @param expense Total expense for the completed phase.
     */
    void record(int64_t expense);

    /**
     * @brief Get the average expense across all recorded phases.
     * @return Average expense, or 0 if no phases recorded.
     */
    int64_t get_average() const;

    /**
     * @brief Get the expense trend (positive = growing, negative = shrinking).
     *
     * Compares the average of the most recent half of recorded entries
     * against the older half. Returns 0 if fewer than 2 entries.
     *
     * @return Trend value (recent_avg - older_avg).
     */
    int64_t get_trend() const;
};

// ============================================================================
// Treasury Application
// ============================================================================

/**
 * @brief Apply an ExpenseBreakdown to TreasuryState expense fields.
 *
 * Updates the per-category expense fields and last_expense total.
 * Does NOT modify balance (that is done by the budget cycle).
 *
 * @param treasury The treasury state to update.
 * @param expenses The expense breakdown to apply.
 */
void apply_expenses_to_treasury(TreasuryState& treasury, const ExpenseBreakdown& expenses);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_EXPENSETRACKING_H
