/**
 * @file BudgetCycle.cpp
 * @brief Implementation of budget cycle processing (Ticket E11-012)
 */

#include "sims3000/economy/BudgetCycle.h"
#include <algorithm>

namespace sims3000 {
namespace economy {

// ============================================================================
// process_budget_cycle
// ============================================================================

BudgetCycleResult process_budget_cycle(
    TreasuryState& treasury,
    const BudgetCycleInput& input,
    uint8_t player_id) {

    BudgetCycleResult result;

    // 1. Calculate net change
    result.net_change = input.income.total - input.expenses.total;

    // 2. Update treasury balance
    treasury.balance += result.net_change;
    result.new_balance = treasury.balance;

    // 3. Update last-phase totals
    treasury.last_income = input.income.total;
    treasury.last_expense = input.expenses.total;

    // 4. Apply income breakdown to treasury fields
    apply_income_to_treasury(treasury, input.income);

    // 5. Apply expense breakdown to treasury fields
    apply_expenses_to_treasury(treasury, input.expenses);

    // 6. Determine deficit status
    result.is_deficit = result.new_balance < 0;

    // 7. Build event
    result.event.player_id = player_id;
    result.event.income = input.income.total;
    result.event.expenses = input.expenses.total;
    result.event.balance_after = result.new_balance;
    result.event.net_change = result.net_change;

    return result;
}

// ============================================================================
// Bond payment helpers
// ============================================================================

namespace {

/**
 * @brief Calculate per-bond payment amounts.
 *
 * principal_payment = bond.principal / bond.term_phases
 * interest_payment = (bond.remaining_principal * bond.interest_rate_basis_points) / (10000 * 12)
 */
struct SingleBondPayment {
    int64_t principal_payment;
    int64_t interest_payment;
    int64_t total;
    bool will_mature; // phases_remaining <= 1
};

SingleBondPayment calculate_single_bond_payment(const CreditAdvance& bond) {
    SingleBondPayment pay;

    if (bond.term_phases == 0) {
        pay.principal_payment = 0;
        pay.interest_payment = 0;
        pay.total = 0;
        pay.will_mature = true;
        return pay;
    }

    pay.principal_payment = bond.principal / bond.term_phases;
    pay.interest_payment = (bond.remaining_principal
                           * static_cast<int64_t>(bond.interest_rate_basis_points))
                          / (10000LL * 12LL);
    pay.total = pay.principal_payment + pay.interest_payment;
    pay.will_mature = (bond.phases_remaining <= 1);

    return pay;
}

} // anonymous namespace

// ============================================================================
// calculate_bond_payments (pure, const)
// ============================================================================

BondPaymentResult calculate_bond_payments(const std::vector<CreditAdvance>& bonds) {
    BondPaymentResult result = {0, 0, 0, 0};

    for (const auto& bond : bonds) {
        auto pay = calculate_single_bond_payment(bond);
        result.principal_paid += pay.principal_payment;
        result.interest_paid += pay.interest_payment;
        result.total_payment += pay.total;
        if (pay.will_mature) {
            ++result.bonds_matured;
        }
    }

    return result;
}

// ============================================================================
// process_bond_payments (mutating)
// ============================================================================

BondPaymentResult process_bond_payments(std::vector<CreditAdvance>& bonds) {
    BondPaymentResult result = {0, 0, 0, 0};

    for (auto& bond : bonds) {
        auto pay = calculate_single_bond_payment(bond);
        result.principal_paid += pay.principal_payment;
        result.interest_paid += pay.interest_payment;
        result.total_payment += pay.total;

        // Deduct principal and decrement phases
        bond.remaining_principal -= pay.principal_payment;
        if (bond.phases_remaining > 0) {
            --bond.phases_remaining;
        }
    }

    // Remove matured bonds (phases_remaining == 0)
    auto new_end = std::remove_if(bonds.begin(), bonds.end(),
        [](const CreditAdvance& b) { return b.phases_remaining == 0; });
    result.bonds_matured = static_cast<uint32_t>(std::distance(new_end, bonds.end()));
    bonds.erase(new_end, bonds.end());

    return result;
}

} // namespace economy
} // namespace sims3000
