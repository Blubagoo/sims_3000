/**
 * @file ExpenseTracking.cpp
 * @brief Implementation of expense breakdown and expense history (Ticket E11-011)
 */

#include "sims3000/economy/ExpenseTracking.h"

namespace sims3000 {
namespace economy {

// ============================================================================
// build_expense_breakdown
// ============================================================================

ExpenseBreakdown build_expense_breakdown(
    const InfrastructureMaintenanceResult& infra,
    const ServiceMaintenanceSummary& services,
    int64_t energy_maintenance,
    int64_t bond_payments,
    int64_t ordinance_costs) {

    ExpenseBreakdown breakdown;
    breakdown.infrastructure_maintenance = infra.total;
    breakdown.service_maintenance = services.total;
    breakdown.energy_maintenance = energy_maintenance;
    breakdown.bond_payments = bond_payments;
    breakdown.ordinance_costs = ordinance_costs;
    breakdown.total = infra.total
                    + services.total
                    + energy_maintenance
                    + bond_payments
                    + ordinance_costs;
    return breakdown;
}

// ============================================================================
// ExpenseHistory
// ============================================================================

void ExpenseHistory::record(int64_t expense) {
    phases[current_index] = expense;
    current_index = (current_index + 1) % HISTORY_SIZE;
    if (count < HISTORY_SIZE) {
        ++count;
    }
}

int64_t ExpenseHistory::get_average() const {
    if (count == 0) {
        return 0;
    }
    int64_t sum = 0;
    for (size_t i = 0; i < count; ++i) {
        sum += phases[i];
    }
    return sum / static_cast<int64_t>(count);
}

int64_t ExpenseHistory::get_trend() const {
    if (count < 2) {
        return 0;
    }

    size_t half = count / 2;
    size_t older_count = count - half;

    int64_t recent_sum = 0;
    int64_t older_sum = 0;

    for (size_t i = 0; i < count; ++i) {
        size_t idx = (current_index + HISTORY_SIZE - 1 - i) % HISTORY_SIZE;
        if (i < half) {
            recent_sum += phases[idx];
        } else {
            older_sum += phases[idx];
        }
    }

    int64_t recent_avg = recent_sum / static_cast<int64_t>(half);
    int64_t older_avg = older_sum / static_cast<int64_t>(older_count);

    return recent_avg - older_avg;
}

// ============================================================================
// apply_expenses_to_treasury
// ============================================================================

void apply_expenses_to_treasury(TreasuryState& treasury, const ExpenseBreakdown& expenses) {
    treasury.infrastructure_maintenance = expenses.infrastructure_maintenance;
    treasury.service_maintenance = expenses.service_maintenance;
    treasury.energy_maintenance = expenses.energy_maintenance;
    treasury.bond_payments = expenses.bond_payments;
    treasury.ordinance_costs = expenses.ordinance_costs;
    treasury.last_expense = expenses.total;
}

} // namespace economy
} // namespace sims3000
